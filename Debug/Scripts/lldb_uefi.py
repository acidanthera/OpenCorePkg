# pylint: disable=no-self-use,undefined-variable,wildcard-import,too-many-statements,too-many-branches
'''
Port of gdb_uefi.py to LLDB.
Refer to gdb_uefi.py for more details.

'''

import array
import binascii
import getopt
import os
import re
import shlex

from collections import OrderedDict
from common_uefi import *
import lldb


class ReloadUefi:
    '''Reload UEFI symbols'''

    #
    # Various constants.
    #

    EINVAL = 0xffffffff
    CV_NB10 = 0x3031424E
    CV_RSDS = 0x53445352
    CV_MTOC = 0x434F544D
    DOS_MAGIC = 0x5A4D
    PE32PLUS_MAGIC = 0x20b
    EST_SIGNATURE = 0x5453595320494249
    DEBUG_GUID = [0x49152E77, 0x1ADA, 0x4764,
                  [0xB7, 0xA2, 0x7A, 0xFE,
                   0xFE, 0xD9, 0x5E, 0x8B]]
    DEBUG_IS_UPDATING = 0x1

    #
    # If the images were built as ELF/MACH-O and then converted to PE,
    # then the base address needs to be offset by PE headers.
    #

    offset_by_headers = False

    def __init__(self, debugger, session_dict):
        self.debugger = debugger
        self.session_dict = session_dict
        self.typetarget = None
        self.activetarget = None

    #
    # Returns SBType for a type.
    #

    def type(self, typename):
        return self.typetarget.FindFirstType(typename)

    #
    # Returns SBType for a pointer to a type.
    #

    def ptype(self, typename):
        return self.typetarget.FindFirstType(typename).GetPointerType()

    #
    # Returns typed SBValue for an address.
    #
    def typed_ptr(self, typename, address):
        target = self.activetarget
        sbdata = lldb.SBData.CreateDataFromInt(address, size=8) ##< size=8 required on 32-bit as well as 64-bit, or resultant pointer cannot be dereferenced.
        return target.CreateValueFromData('ptr', sbdata, typename)

    #
    # Cast pointer in way which works round fragilities of 32-bit handling (simple ptr.Cast(type) works in 64-bit).
    #     
    def cast_ptr(self, type, ptr):
        return self.typed_ptr(type, ptr.GetValueAsUnsigned())

    #
    # Computes CRC32 on an array of data.
    #

    def crc32(self, data):
        return binascii.crc32(data) & 0xFFFFFFFF
    
    #
    # GetChildMemberWithName working round fragilities of 32-bit handling.
    #

    def get_child_member_with_name(self, value, field_name):
        member = value.GetChildMemberWithName(field_name)
        if member.TypeIsPointerType():
            member = self.cast_ptr(member.GetType(), member)
        return member

    #
    # Gets a field from struct as an unsigned value.
    #

    def get_field(self, value, field_name=None, force_bytes=False, force_int=False, single_entry=False):
        if field_name is not None:
            member = self.get_child_member_with_name(value, field_name)
        else:
            member = value

        if (not force_int and member.GetByteSize() > self.typetarget.GetAddressByteSize()) or force_bytes:
            sbdata = member.GetData()
            byte_size = sbdata.GetByteSize()
            data = array.array('B')
            error = lldb.SBError()
            for i in range(0, byte_size):
                data.append(sbdata.GetUnsignedInt8(error, i))
            return data

        if member.TypeIsPointerType():
            if single_entry:
                return member.Dereference().GetValueAsUnsigned()

            # Unfortunately LLDB is unaware of underlying types like CHAR8 when working in PDB mode,
            # so we only have size to trust. DWARF mode is ok, but it is best to stay most compatible.
            member_unsigned = member.GetValueAsUnsigned()
            char_type = member.GetType().GetPointeeType()
            if char_type.GetByteSize() == 1:
                char_size = 1
                char_data = array.array('B')
            elif char_type.GetByteSize() == 2:
                char_size = 2
                char_data = array.array('H')
            else:
                return member_unsigned

            i = 0
            while i < 0x1000:
                c = member.CreateValueFromAddress('ptr', member_unsigned + i * char_size, char_type).GetValueAsUnsigned()
                char_data.append(c)
                if c == 0:
                    break
                i += 1

            return char_data

        return member.GetValueAsUnsigned()

    #
    # Sets a field in a struct to a value, i.e.
    #      value->field_name = data.
    #

    def set_field(self, value, field_name, data):
        member = self.get_child_member_with_name(value, field_name)
        data = lldb.SBData.CreateDataFromInt(data, size=member.GetByteSize())
        error = lldb.SBError()
        member.SetData(data, error)

    #
    # Locates the EFI_SYSTEM_TABLE as per UEFI spec 17.4.
    # Returns base address or -1.
    #

    def search_est(self):
        address = 0
        estp_t = self.ptype('EFI_SYSTEM_TABLE_POINTER')
        while True:
            estp = self.typed_ptr(estp_t, address)
            if self.get_field(estp, 'Signature', force_int=True) == self.EST_SIGNATURE:
                oldcrc = self.get_field(estp, 'Crc32')
                self.set_field(estp, 'Crc32', 0)
                newcrc = self.crc32(self.get_field(estp.Dereference()))
                self.set_field(estp, 'Crc32', oldcrc)
                if newcrc == oldcrc:
                    print(f'EFI_SYSTEM_TABLE_POINTER @ 0x{address:x}')
                    return self.get_child_member_with_name(estp, 'EfiSystemTableBase')

            address += 4 * 2**20
            if address >= 2**32:
                return self.EINVAL

    #
    # Searches for a vendor-specific configuration table (in EST),
    # given a vendor-specific table GUID. GUID is a list like -
    # [32-bit, 16-bit, 16-bit, [8 bytes]]
    #

    def search_config(self, cfg_table, count, guid):
        index = 0
        while index != count:
            # GetChildAtIndex accesses inner structure fields, so we have to use the fugly way.
            cfg_entry = cfg_table.GetValueForExpressionPath(f'[{index}]')
            cfg_guid = self.get_child_member_with_name(cfg_entry, 'VendorGuid')
            if self.get_field(cfg_guid, 'Data1') == guid[0] and \
               self.get_field(cfg_guid, 'Data2') == guid[1] and \
               self.get_field(cfg_guid, 'Data3') == guid[2] and \
               self.get_field(cfg_guid, 'Data4', True).tolist() == guid[3]:
                return self.get_child_member_with_name(cfg_entry, 'VendorTable')
            index += 1
        return self.EINVAL

    #
    # Returns offset of a field within structure. Useful
    # for getting container of a structure.
    #

    def offsetof(self, typename, field):
        t = self.ptype(typename)
        for index in range(0, t.GetNumberOfFields()):
            f = t.GetFieldAtIndex(index)
            if f.GetName() == field:
                return f.GetOffsetInBytes()
        raise RuntimeError(f'Cannot find {field} in {typename} to get offset')

    #
    # Returns sizeof of a type.
    #

    def sizeof(self, typename):
        return self.type(typename).GetByteSize()

    #
    # Returns the EFI_IMAGE_NT_HEADERS32 pointer, given
    # an ImageBase address as a SBValue.
    #

    def pe_headers(self, imagebase):
        dosh_t = self.ptype('EFI_IMAGE_DOS_HEADER')
        head_t = self.ptype('EFI_IMAGE_OPTIONAL_HEADER_UNION')
        dosh = self.typed_ptr(dosh_t, imagebase)
        h_addr = imagebase
        if self.get_field(dosh, 'e_magic') == self.DOS_MAGIC:
            h_addr = h_addr + self.get_field(dosh, 'e_lfanew')
        return self.typed_ptr(head_t, h_addr)

    #
    # Returns a dictionary with PE sections.
    #

    def pe_sections(self, combined, file, _):
        sect_t = self.ptype('EFI_IMAGE_SECTION_HEADER')
        common = self.get_child_member_with_name(combined, 'CommonHeader')
        sections_addr = common.GetLoadAddress() + common.GetByteSize() + self.get_field(file, 'SizeOfOptionalHeader')
        sections = self.typed_ptr(sect_t, sections_addr)
        sects = OrderedDict()
        for i in range(self.get_field(file, 'NumberOfSections')):
            section = sections.GetValueForExpressionPath(f'[{i}]')
            name = self.get_field(section, 'Name', force_bytes=True)
            name = UefiMisc.parse_utf8(name)
            addr = self.get_field(section, 'VirtualAddress')
            if name != '':
                sects[name] = addr
        return sects

    #
    # Returns True if pe_headers refer to a PE32+ image.
    #

    def pe_is_64(self, pe_headers):
        magic = pe_headers.GetValueForExpressionPath('.Pe32.Magic').GetValueAsUnsigned()
        return magic == self.PE32PLUS_MAGIC

    #
    # Returns the PE fileheader.
    #

    def pe_file(self, pe):
        if self.pe_is_64(pe):
            obj = self.get_child_member_with_name(pe, 'Pe32Plus')
        else:
            obj = self.get_child_member_with_name(pe, 'Pe32')
        return self.get_child_member_with_name(obj, 'FileHeader')

    #
    # Returns the PE (not so) optional header.
    #

    def pe_combined(self, pe):
        if self.pe_is_64(pe):
            return self.get_child_member_with_name(pe, 'Pe32Plus')
        else:
            return self.get_child_member_with_name(pe, 'Pe32')

    #
    # Returns the symbol file name for a PE image.
    #

    def pe_parse_debug(self, base):
        pe = self.pe_headers(base)
        combined = self.pe_combined(pe)
        debug_dir_entry = combined.GetValueForExpressionPath('.DataDirectory[6]')
        dep = self.get_field(debug_dir_entry, 'VirtualAddress') + base
        dep = self.typed_ptr(self.ptype('EFI_IMAGE_DEBUG_DIRECTORY_ENTRY'), dep)
        cvp = self.get_field(dep, 'RVA') + base
        # FIXME: UINT32 should be used here instead of unsigned, but LLDB+PDB type system is broken.
        cvv = self.typed_ptr(self.ptype('unsigned'), cvp).Dereference().GetValueAsUnsigned()
        if cvv == self.CV_NB10:
            return cvp + self.sizeof('EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY')
        if cvv == self.CV_RSDS:
            return cvp + self.sizeof('EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY')
        if cvv == self.CV_MTOC:
            return cvp + self.sizeof('EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY')
        if cvv == 0:
            return 0

        return self.EINVAL

    #
    # Prepares lldb symbol load command.
    # Currently supports Mach-O and single-section files.
    #
    def get_sym_cmd(self, filename, orgbase):
        if filename.endswith('.pdb'):
            dll_file = filename.replace('.pdb', '.dll')
            module_cmd = f'target modules add -s {filename} {dll_file}'
        else:
            dll_file = filename
            module_cmd = f'target modules add {filename}'
        map_cmd = f'target modules load -f {dll_file} -s 0x{orgbase:X}'
        return(module_cmd, map_cmd)

    #
    # Parses an EFI_LOADED_IMAGE_PROTOCOL, figuring out the symbol file name.
    # This file name is then appended to list of loaded symbols.
    #
    # TODO: Support TE images.
    #

    def parse_image(self, image, syms):
        base = self.get_field(image, 'ImageBase')
        pe = self.pe_headers(base)
        combined = self.pe_combined(pe)
        sym_address = self.pe_parse_debug(base)

        if sym_address == 0:
            # llvm-objcopy --add-gnu-debuglink=a/x.debug a/x.dll does not update
            # DataDirectory with any data, instead it creates a new section
            # with a /\d+ name containing:
            # - ASCII debug file name (x.debug) padded by 4 bytes
            # - CRC32
            file = self.pe_file(pe)
            sections = self.pe_sections(combined, file, base)
            last_section = next(reversed(sections))
            if re.match(r'^/\d+$', last_section):
                sym_address = sections[last_section]
                sym_ptr = self.typed_ptr(self.ptype('char'), sym_address + base)
                sym_name = UefiMisc.parse_utf8(self.get_field(sym_ptr))
                if not sym_name.endswith('.debug'):
                    sym_name = self.EINVAL
        elif sym_address != self.EINVAL:
            sym_ptr = self.typed_ptr(self.ptype('char'), sym_address)
            sym_name = UefiMisc.parse_utf8(self.get_field(sym_ptr))
        else:
            sym_name = self.EINVAL

        # For ELF and Mach-O-derived images...
        if self.offset_by_headers:
            base = base + self.get_field(combined, 'SizeOfHeaders')
        if sym_name != self.EINVAL:
            self.add_sym(sym_name, base, syms)

    #
    # Add symbol load command with additional processing for correct file location.
    #
   
    def add_sym(self, sym_name, base, syms):
            macho = os.path.isdir(sym_name + '.dSYM')
            if macho:
                real_sym = sym_name
            else:
                sym_name_dbg = re.sub(r'\.dll$', '.debug', sym_name)
                if sym_name_dbg != sym_name and os.path.exists(sym_name_dbg):
                    real_sym = sym_name_dbg
                else:
                    real_sym = None
                    paths = os.getenv('EFI_SYMBOL_PATH', '').split(':')
                    for path in paths:
                        if os.path.exists(os.path.join(path, sym_name)):
                            real_sym = os.path.join(path, sym_name)
                            break

            if real_sym:
                syms.append(self.get_sym_cmd(real_sym, base))
            else:
                print(f'No symbol file {sym_name}')

    #
    # Use debug info from new image loader.
    #

    def use_new_debug_info(self, entry, syms):
        sym_address = self.get_child_member_with_name(entry, 'PdbPath')
        if sym_address:
            sym_ptr = self.cast_ptr(self.ptype('char'), sym_address)
            sym_name = UefiMisc.parse_utf8(self.get_field(sym_ptr))
            debug_base = self.get_field(entry, 'DebugBase')
            self.add_sym(sym_name, debug_base, syms)
        else:
            print('No symbol file')

    #
    # Parses table EFI_DEBUG_IMAGE_INFO structures, builds
    # a list of add-symbol-file commands, and reloads debugger
    # symbols.
    #

    def parse_edii(self, edii, count):
        index = 0
        syms = []
        while index != count:
            # GetChildAtIndex accesses inner structure fields, so we have to use the fugly way again.
            entry = edii.GetValueForExpressionPath(f'[{index}]')
            image_type = self.get_field(entry, 'ImageInfoType', single_entry=True)
            if image_type == 1:
                entry = self.get_child_member_with_name(entry, 'NormalImage')
                self.parse_image(self.get_child_member_with_name(entry, 'LoadedImageProtocolInstance'), syms)
            elif image_type == 2:
                entry = self.get_child_member_with_name(entry, 'NormalImage2')
                self.use_new_debug_info(entry, syms)
            else:
                print(f'Skipping unknown EFI_DEBUG_IMAGE_INFO (ImageInfoType {image_type})')
            index = index + 1
        print('Loading new symbols...')
        for sym in syms:
            print(sym[0])
            self.debugger.HandleCommand(sym[0])
            print(sym[1])
            self.debugger.HandleCommand(sym[1])

    #
    # Parses EFI_DEBUG_IMAGE_INFO_TABLE_HEADER, in order to load
    # image symbols.
    #

    def parse_dh(self, dh):
        dh_t = self.ptype('EFI_DEBUG_IMAGE_INFO_TABLE_HEADER')
        dh = self.cast_ptr(dh_t, dh)
        print(f"DebugImageInfoTable @ 0x{self.get_field(dh, 'EfiDebugImageInfoTable'):x}, 0x{self.get_field(dh, 'TableSize'):x} entries")
        if self.get_field(dh, 'UpdateStatus') & self.DEBUG_IS_UPDATING:
            print('EfiDebugImageInfoTable update in progress, retry later')
            return
        self.parse_edii(self.get_child_member_with_name(dh, 'EfiDebugImageInfoTable'), self.get_field(dh, 'TableSize'))

    #
    # Parses EFI_SYSTEM_TABLE, in order to load image symbols.
    #

    def parse_est(self, est):
        est_t = self.ptype('EFI_SYSTEM_TABLE')
        est = self.cast_ptr(est_t, est)
        print(f"Connected to {UefiMisc.parse_utf16(self.get_field(est, 'FirmwareVendor'))}(Rev. 0x{self.get_field(est, 'FirmwareRevision'):x})")
        print(f"ConfigurationTable @ 0x{self.get_field(est, 'ConfigurationTable'):x}, 0x{self.get_field(est, 'NumberOfTableEntries'):x} entries")
        dh = self.search_config(self.get_child_member_with_name(est, 'ConfigurationTable'), self.get_field(est, 'NumberOfTableEntries'), self.DEBUG_GUID)
        if dh == self.EINVAL:
            print('No EFI_DEBUG_IMAGE_INFO_TABLE_HEADER')
            return
        self.parse_dh(dh)

    #
    # Usage information.
    #

    def get_short_help(self):
        return 'Usage: reload-uefi [-o] [/path/to/GdbSyms.dll]'

    def get_long_help(self):
        return 'Usage: reload-uefi [-o] [/path/to/GdbSyms.dll]'

    def usage(self):
        print(self.get_short_help())

    #
    # Handler for reload-uefi.
    #

    def __call__(self, debugger, command, exe_ctx, result):
        self.debugger = debugger

        category = debugger.GetDefaultCategory()
        format_bool = lldb.SBTypeFormat(lldb.eFormatBoolean)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('BOOLEAN'), format_bool)

        format_hex = lldb.SBTypeFormat(lldb.eFormatHex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('UINT64'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('INT64'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('UINT32'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('INT32'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('UINT16'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('INT16'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('UINT8'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('INT8'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('UINTN'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('INTN'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('CHAR8'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('CHAR16'), format_hex)

        category.AddTypeFormat(lldb.SBTypeNameSpecifier('EFI_PHYSICAL_ADDRESS'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('PHYSICAL_ADDRESS'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('EFI_STATUS'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('EFI_TPL'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('EFI_LBA'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('EFI_BOOT_MODE'), format_hex)
        category.AddTypeFormat(lldb.SBTypeNameSpecifier('EFI_FV_FILETYPE'), format_hex)

        args = shlex.split(command)
        try:
            opts, args = getopt.getopt(args, 'o', ['offset-by-headers'])
        except(getopt.GetoptError) as _:
            self.usage()
            return
        for opt, _ in opts:
            if opt == '-o':
                self.offset_by_headers = True

        self.typetarget = None
        self.activetarget = None

        # FIXME: Use ReadCStringFromMemory.
        # FIXME: Support executing code.
        if len(args) >= 1 and args[0] != '':
            gdb.execute('symbol-file')
            gdb.execute(f'symbol-file {args[0]}')
        else:
            for i in range(0, self.debugger.GetNumTargets()):
                target = self.debugger.GetTargetAtIndex(i)
                target_name = str(target)
                print(f"Target {i} is '{target_name}'")
                if target_name.find('GdbSyms') >= 0:
                    self.typetarget = target
                elif target_name.find('No executable module.') >= 0:
                    self.activetarget = target

        if not self.typetarget:
            print('Cannot find GdbSyms target!')
            return

        if not self.activetarget:
            print('Cannot find target with full memory access!')
            return

        # Force into full memory target.
        self.debugger.SetSelectedTarget(self.activetarget)

        est = self.search_est()
        if est == self.EINVAL:
            print('No EFI_SYSTEM_TABLE...')
            return

        print(f'EFI_SYSTEM_TABLE @ 0x{est.GetValueAsUnsigned():x}')
        self.parse_est(est)
