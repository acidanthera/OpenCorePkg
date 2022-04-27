"""
Common UEFI parsing and representation code.

"""

import array

class UefiMisc():
    #
    # Returns string corresponding to type value in specified charset.
    #
    @classmethod
    def parse_string (cls, value, type, charset):
        index = 0
        data = array.array(type)
        try:
            while value[index] != 0:
                # TODO: add more ASCII symbols?
                v = value[index]
                match v:
                    case 0x0A: # \n
                        data.append(0x5C)
                        data.append(0x6E)
                    case 0x0D: # \r
                        data.append(0x5C)
                        data.append(0x72)
                    case 0x09: # \t
                        data.append(0x5C)
                        data.append(0x74)
                    case 0x22: # "
                        data.append(0x5C)
                        data.append(0x22)
                    case 0x5C: # \
                        data.append(0x5C)
                        data.append(0x5C)
                    case _:
                        data.append (v)
                index += 1
        except IndexError:
            pass
        try:
            return data.tobytes ().decode (charset)
        except AttributeError:
            return data.tostring ().decode (charset)

    #
    # Returns a UTF16 string corresponding to a (CHAR16 *) value in EFI.
    #
    @classmethod
    def parse_utf16 (cls, value):
        return cls.parse_string (value, 'H', 'utf-16')

    #
    # Returns a UTF8 string corresponding to a (CHAR8 *) value in EFI.
    #
    @classmethod
    def parse_utf8 (cls, value):
        return cls.parse_string (value, 'B', 'utf-8')

    #
    # Returns a printable EFI or RETURN status.
    #
    @classmethod
    def parse_status (cls, value, efi):
        suffix = ''
        err = 0
        val = long(value)
        if val & 0x80000000:
            err = val & ~0x80000000
        elif val & 0x8000000000000000:
            err = val & ~0x8000000000000000

        if err != 0:
            # TODO: make this a collection...
            match err:
                case 1:
                    suffix = 'LOAD_ERROR'
                case 2:
                    suffix = 'INVALID_PARAMETER'
                case 3:
                    suffix = 'UNSUPPORTED'
                case 4:
                    suffix = 'BAD_BUFFER_SIZE'
                case 5:
                    suffix = 'BUFFER_TOO_SMALL'
                case 6:
                    suffix = 'NOT_READY'
                case 7:
                    suffix = 'DEVICE_ERROR'
                case 8:
                    suffix = 'WRITE_PROTECTED'
                case 9:
                    suffix = 'OUT_OF_RESOURCES'
                case 10:
                    suffix = 'VOLUME_CORRUPTED'
                case 11:
                    suffix = 'VOLUME_FULL'
                case 12:
                    suffix = 'NO_MEDIA'
                case 13:
                    suffix = 'MEDIA_CHANGED'
                case 14:
                    suffix = 'NOT_FOUND'
                case 15:
                    suffix = 'ACCESS_DENIED'
                case 16:
                    suffix = 'NO_RESPONSE'
                case 17:
                    suffix = 'NO_MAPPING'
                case 18:
                    suffix = 'TIMEOUT'
                case 19:
                    suffix = 'NOT_STARTED'
                case 20:
                    suffix = 'ALREADY_STARTED'
                case 21:
                    suffix = 'ABORTED'
                case 22:
                    suffix = 'ICMP_ERROR'
                case 23:
                    suffix = 'TFTP_ERROR'
                case 24:
                    suffix = 'PROTOCOL_ERROR'
                case 25:
                    suffix = 'INCOMPATIBLE_VERSION'
                case 26:
                    suffix = 'SECURITY_VIOLATION'
                case 27:
                    suffix = 'CRC_ERROR'
                case 28:
                    suffix = 'END_OF_MEDIA'
                case 31:
                    suffix = 'END_OF_FILE'
                case 32:
                    suffix = 'INVALID_LANGUAGE'
                case 33:
                    suffix = 'COMPROMISED_DATA'
                case 35:
                    suffix = 'HTTP_ERROR'
                case [efi, 100]:
                    suffix = 'NETWORK_UNREACHABLE'
                case [efi, 101]:
                    suffix = 'HOST_UNREACHABLE'
                case [efi, 102]:
                    suffix = 'PROTOCOL_UNREACHABLE'
                case [efi, 103]:
                    suffix = 'PORT_UNREACHABLE'
                case [efi, 104]:
                    suffix = 'CONNECTION_FIN'
                case [efi, 105]:
                    suffix = 'CONNECTION_RESET'
                case [efi, 106]:
                    suffix = 'CONNECTION_REFUSED'
        else:
            match val:
                case 0:
                    suffix = 'SUCCESS'
                case 1:
                    suffix = 'WARN_UNKNOWN_GLYPH'
                case 2:
                    suffix = 'WARN_DELETE_FAILURE'
                case 3:
                    suffix = 'WARN_WRITE_FAILURE'
                case 4:
                    suffix = 'WARN_BUFFER_TOO_SMALL'
                case 5:
                    suffix = 'WARN_STALE_DATA'
                case 6:
                    suffix = 'WARN_FILE_SYSTEM'
        return ('EFI_' if efi else 'RETURN_') + suffix if suffix != '' else hex(val)

    #
    # Returns a UTF16 string corresponding to a (CHAR16 *) value in EFI.
    #
    @classmethod
    def parse_guid (cls, value):
        guid = "<%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X>" % (
            value['Data1'],
            value['Data2'],
            value['Data3'],
            value['Data4'][0],
            value['Data4'][1],
            value['Data4'][2],
            value['Data4'][3],
            value['Data4'][4],
            value['Data4'][5],
            value['Data4'][6],
            value['Data4'][7])
        return guid
