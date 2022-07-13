# pylint: disable=no-self-use,undefined-variable,too-few-public-methods

'''
Allows loading TianoCore symbols into a GDB session attached to EFI
Firmware.

This is how it works: build GdbSyms - it's a dummy binary that
contains the relevant symbols needed to find and load image symbols.

$ gdb /path/to/GdbSyms.dll
(gdb) target remote ....
(gdb) source Scripts/gdb_uefi.py
(gdb) reload-uefi -o /path/to/GdbSyms.dll

N.B: it was noticed that GDB for certain targets behaves strangely
when run without any binary - like assuming a certain physical
address space size and endianness. To avoid this madness and
seing strange bugs, make sure to pass /path/to/GdbSyms.dll
when starting gdb.

The -o option should be used if you've debugging EFI, where the PE
images were converted from MACH-O or ELF binaries.

'''


import array
import getopt
import binascii
import re
import sys
import os
import subprocess
from common_uefi import UefiMisc

sys.path.append(os.path.dirname(__file__))


__license__ = 'BSD'
__version__ = '1.0.0'
__maintainer__ = 'Andrei Warkentin'
__email__ = 'andrey.warkentin@gmail.com'
__status__ = 'Works'


class ReloadUefi(gdb.Command):

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

    def __init__(self):
        super().__init__('reload-uefi', gdb.COMMAND_OBSCURE)

    #
    # Returns gdb.Type for a type.
    #

    def lookup_type(self, typename):
        return gdb.lookup_type(typename)

    #
    # Returns gdb.Type for a pointer to a type.
    #

    def ptype(self, typename):
        return gdb.lookup_type(typename).pointer()

    #
    # Computes CRC32 on an array of data.
    #

    def crc32(self, data):
        return binascii.crc32(data) & 0xFFFFFFFF

    #
    # Sets a field in a struct to a value, i.e.
    #      value->field_name = data.
    #
    # Newer Py bindings to Gdb provide access to the inferior
    # memory, but not all, so have to do it this awkward way.
    #

    def set_field(self, value, field_name, data):
        gdb.execute(f'set *({str(value[field_name].type)} *) 0x{int(value[field_name].address):x} = 0x{data}')

    #
    # Returns data backing a gdb.Value as an array.
    # Same comment as above regarding newer Py bindings...
    #

    def value_data(self, value, bytes_length=0):
        value_address = gdb.Value(value.address)
        array_t = self.ptype('UINT8')
        value_array = value_address.cast(array_t)
        if bytes_length == 0:
            bytes_length = value.type.sizeof
        data = array.array('B')
        for i in range(0, bytes_length):
            data.append(value_array[i])
        return data

    # Locates the EFI_SYSTEM_TABLE as per UEFI spec 17.4.
    # Returns base address or -1.

    def search_est(self):
        address = 0
        estp_t = self.ptype('EFI_SYSTEM_TABLE_POINTER')
        while True:
            try:
                estp = gdb.Value(address).cast(estp_t)
                if estp['Signature'] == self.EST_SIGNATURE:
                    oldcrc = int(estp['Crc32'])
                    self.set_field(estp, 'Crc32', 0)
                    newcrc = self.crc32(self.value_data(estp.dereference(), 0))
                    self.set_field(estp, 'Crc32', int(oldcrc))
                    if newcrc == oldcrc:
                        print(f'EFI_SYSTEM_TABLE_POINTER @ 0x{address:x}')
                        return estp['EfiSystemTableBase']
            except gdb.MemoryError:
                pass

            address += 4 * 2**20
            if address >= 2**32:
                return self.EINVAL

    #
    # Searches for a vendor-specific configuration table(in EST),
    # given a vendor-specific table GUID. GUID is a list like -
    # [32-bit, 16-bit, 16-bit, [8 bytes]]
    #

    def search_config(self, cfg_table, count, guid):
        index = 0
        while index != count:
            cfg_entry = cfg_table[index]['VendorGuid']
            if cfg_entry['Data1'] == guid[0] and \
                    cfg_entry['Data2'] == guid[1] and \
                    cfg_entry['Data3'] == guid[2] and \
                    self.value_data(cfg_entry['Data4']).tolist() == guid[3]:
                return cfg_table[index]['VendorTable']
            index += 1
        return gdb.Value(self.EINVAL)

    #
    # Returns offset of a field within structure. Useful
    # for getting container of a structure.
    #

    def offsetof(self, typename, field):
        offset = gdb.Value(0).cast(self.ptype(typename))
        return int(offset[field].address)

    #
    # Returns sizeof of a type.
    #

    def sizeof(self, typename):
        return self.type(typename).sizeof

    #
    # Returns the EFI_IMAGE_NT_HEADERS32 pointer, given
    # an ImageBase address as a gdb.Value.
    #

    def pe_headers(self, imagebase):
        dosh_t = self.ptype('EFI_IMAGE_DOS_HEADER')
        head_t = self.ptype('EFI_IMAGE_OPTIONAL_HEADER_UNION')
        dosh = imagebase.cast(dosh_t)
        h_addr = int(imagebase)
        if dosh['e_magic'] == self.DOS_MAGIC:
            h_addr = h_addr + int(dosh['e_lfanew'])
        return gdb.Value(h_addr).cast(head_t)

    #
    # Returns a dictionary with PE sections.
    #

    def pe_sections(self, opt, file, _):
        sect_t = self.ptype('EFI_IMAGE_SECTION_HEADER')
        sections = (opt.address + 1).cast(sect_t)
        sects = {}
        for i in range(file['NumberOfSections']):
            name = UefiMisc.parse_utf8(sections[i]['Name'])
            addr = int(sections[i]['VirtualAddress'])
            if name != '':
                sects[name] = addr
        return sects

    #
    # Returns True if pe_headers refer to a PE32+ image.
    #

    def pe_is_64(self, pe_headers):
        return pe_headers['Pe32']['OptionalHeader']['Magic'] == self.PE32PLUS_MAGIC

    #
    # Returns the PE fileheader.
    #

    def pe_file(self, pe):
        return pe['Pe32Plus']['FileHeader'] if self.pe_is_64(pe) else pe['Pe32']['FileHeader']

    #
    # Returns the PE(not so) optional header.
    #

    def pe_optional(self, pe):
        return pe['Pe32Plus']['OptionalHeader'] if self.pe_is_64(pe) else pe['Pe32']['OptionalHeader']

    #
    # Returns the symbol file name for a PE image.
    #

    def pe_parse_debug(self, pe):
        opt = self.pe_optional(pe)
        debug_dir_entry = opt['DataDirectory'][6]
        dep = debug_dir_entry['VirtualAddress'] + opt['ImageBase']
        dep = dep.cast(self.ptype('EFI_IMAGE_DEBUG_DIRECTORY_ENTRY'))
        cvp = dep.dereference()['RVA'] + opt['ImageBase']
        cvv = cvp.cast(self.ptype('UINT32')).dereference()
        if cvv == self.CV_NB10:
            return cvp + self.sizeof('EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY')
        if cvv == self.CV_RSDS:
            return cvp + self.sizeof('EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY')
        if cvv == self.CV_MTOC:
            return cvp + self.sizeof('EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY')
        return gdb.Value(self.EINVAL)

    #
    # Prepares gdb symbol load command with proper section information.
    # Currently supports Mach-O and single-section files.
    #
    # TODO: Proper ELF support.
    #
    def get_sym_cmd(self, file, orgbase, sections, macho, fallack_base):
        cmd = f'add-symbol-file {file}'

        # Fallback case, no sections, just load .text.
        if not sections.get('.text') or not sections.get('.data'):
            cmd += f' 0x{fallack_base:x}'
            return cmd

        cmd += f" 0x{int(orgbase):x}{sections['.text']}"

        if not macho or not os.path.exists(file):
            # Another fallback, try to load data at least.
            cmd += f" -s .data 0x{int(orgbase) + sections['.data']:x}"
            return cmd

        # 1. Parse Mach-O.
        # FIXME: We should not rely on otool really.
        commands = subprocess.check_output(['otool', '-l', file])
        try:
            lines = commands.decode('utf-8').split('\n')
        except Exception:
            lines = commands.split('\n')
        in_sect = False
        machsections = {}
        for line in lines:
            line = line.strip()
            if line.startswith('Section'):
                in_sect = True
                sectname = None
                segname = None
            elif in_sect:
                if line.startswith('sectname'):
                    sectname = line.split()[1]
                elif line.startswith('segname'):
                    segname = line.split()[1]
                elif line.startswith('addr'):
                    machsections[segname + '.' + sectname] = int(line.split()[1], base=16)
                    in_sect = False

        # 2. Convert section names to gdb sections.
        mapping = {
            '__TEXT.__cstring':         '.cstring',
            '__TEXT.__const':           '.const',
            '__TEXT.__ustring':         '__TEXT.__ustring',
            '__DATA.__const':           '.const_data',
            '__DATA.__data':            '.data',
            '__DATA.__bss':             '.bss',
            '__DATA.__common':          '__DATA.__common',
            # FIXME: These should not be loadable, but gdb still loads them :/
            # '__DWARF.__apple_names':    '__DWARF.__apple_names',
            # '__DWARF.__apple_namespac': '__DWARF.__apple_namespac',
            # '__DWARF.__apple_types':    '__DWARF.__apple_types',
            # '__DWARF.__apple_objc':     '__DWARF.__apple_objc',
        }

        # 3. Rebase.
        for entry in mapping.items():
            if machsections.get(entry):
                cmd += f' -s {mapping[entry]} 0x{int(orgbase) + machsections[entry]:x}'

        return cmd

    #
    # Parses an EFI_LOADED_IMAGE_PROTOCOL, figuring out the symbol file name.
    # This file name is then appended to list of loaded symbols.
    #
    # TODO: Support TE images.
    #

    def parse_image(self, image, syms):
        orgbase = base = image['ImageBase']
        pe = self.pe_headers(base)
        opt = self.pe_optional(pe)
        file = self.pe_file(pe)
        sym_name = self.pe_parse_debug(pe)
        sections = self.pe_sections(opt, file, base)

        # For ELF and Mach-O-derived images...
        if self.offset_by_headers:
            base = base + opt['SizeOfHeaders']
        if sym_name != self.EINVAL:
            sym_name = sym_name.cast(self.ptype('CHAR8')).string()
            sym_name_dbg = re.sub(r'\.dll$', '.debug', sym_name)
            macho = False
            if os.path.isdir(sym_name + '.dSYM'):
                sym_name += '.dSYM/Contents/Resources/DWARF/' + os.path.basename(sym_name)
                macho = True
            elif sym_name_dbg != sym_name and os.path.exists(sym_name_dbg):
                # TODO: implement .elf handling.
                sym_name = sym_name_dbg
            syms.append(self.get_sym_cmd(sym_name, int(orgbase), sections, macho, int(base)))

    #
    # Parses table EFI_DEBUG_IMAGE_INFO structures, builds
    # a list of add-symbol-file commands, and reloads debugger
    # symbols.
    #

    def parse_edii(self, edii, count):
        index = 0
        syms = []
        print(f'Found {count} images...')
        while index != count:
            entry = edii[index]
            if entry['ImageInfoType'].dereference() == 1:
                entry = entry['NormalImage']
                self.parse_image(entry['LoadedImageProtocolInstance'], syms)
            else:
                print(f"Skipping unknown EFI_DEBUG_IMAGE_INFO(Type 0x{entry['ImageInfoType'].dereference():x})")
            index += 1
        gdb.execute('symbol-file')
        print('Loading new symbols...')
        for sym in syms:
            try:
                gdb.execute(sym)
            except(gdb.error) as err:
                print(f'Failed: {err}')

    #
    # Parses EFI_DEBUG_IMAGE_INFO_TABLE_HEADER, in order to load
    # image symbols.
    #

    def parse_dh(self, dh):
        dh_t = self.ptype('EFI_DEBUG_IMAGE_INFO_TABLE_HEADER')
        dh = dh.cast(dh_t)
        print(f"DebugImageInfoTable @ 0x{int(dh['EfiDebugImageInfoTable']):x}, 0x{dh['TableSize']:x} entries")
        if dh['UpdateStatus'] & self.DEBUG_IS_UPDATING:
            print('EfiDebugImageInfoTable update in progress, retry later')
            return
        self.parse_edii(dh['EfiDebugImageInfoTable'], dh['TableSize'])

    #
    # Parses EFI_SYSTEM_TABLE, in order to load image symbols.
    #

    def parse_est(self, est):
        est_t = self.ptype('EFI_SYSTEM_TABLE')
        est = est.cast(est_t)
        print(f"Connected to {UefiMisc.parse_utf16(est['FirmwareVendor'])}(Rev. 0x{int(est['FirmwareRevision']):x})")
        print(f"ConfigurationTable @ 0x{int(est['ConfigurationTable']):x}, 0x{est['NumberOfTableEntries']:x} entries")

        dh = self.search_config(est['ConfigurationTable'], est['NumberOfTableEntries'], self.DEBUG_GUID)
        if dh == self.EINVAL:
            print('No EFI_DEBUG_IMAGE_INFO_TABLE_HEADER')
            return
        self.parse_dh(dh)

    #
    # Usage information.
    #

    def usage(self):
        print('Usage: reload-uefi [-o] [/path/to/GdbSyms.dll]')

    #
    # Handler for reload-uefi.
    #

    def invoke(self, arg, _):
        args = arg.split(' ')
        try:
            opts, args = getopt.getopt(args, 'o', ['offset-by-headers'])
        except(getopt.GetoptError) as _:
            self.usage()
            return
        for opt, _ in opts:
            if opt == '-o':
                self.offset_by_headers = True

        if len(args) >= 1 and args[0] != '':
            gdb.execute('symbol-file')
            gdb.execute(f'symbol-file {args[0]}')
        else:
            # FIXME: gdb.objfiles() loses files after symbol-file execution,
            # so we have to extract GdbSymbs.dll manually.
            lines = gdb.execute('info files', to_string=True).split('\n')
            for line in lines:
                m = re.search("`([^']+)'", line)
                if m:
                    gdb.execute('symbol-file')
                    gdb.execute(f'symbol-file {m.group(1)}')
                    break

        est = self.search_est()
        if est == self.EINVAL:
            print('No EFI_SYSTEM_TABLE...')
            return

        print(f'EFI_SYSTEM_TABLE @ 0x{est}:x')
        self.parse_est(est)


class UefiStringPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return 'NULL' if not self.val else f"L'{UefiMisc.parse_utf16(self.val)}'"


class UefiEfiStatusPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return UefiMisc.parse_status(self.val, True)


class UefiReturnStatusPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return UefiMisc.parse_status(self.val, False)


class UefiGuidPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return UefiMisc.parse_guid(self.val)


def lookup_uefi_type(val):
    if str(val.type) == 'const CHAR16 *' or str(val.type) == 'CHAR16 *':
        return UefiStringPrinter(val)
    if str(val.type) == 'EFI_STATUS':
        return UefiEfiStatusPrinter(val)
    if str(val.type) == 'RETURN_STATUS':
        return UefiReturnStatusPrinter(val)
    if str(val.type) == 'GUID' or str(val.type) == 'EFI_GUID':
        return UefiGuidPrinter(val)
    return None


ReloadUefi()
gdb.pretty_printers.append(lookup_uefi_type)
