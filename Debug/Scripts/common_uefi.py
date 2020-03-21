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
        data = array.array (type)
        try:
            while value[index] != 0:
                # TODO: add more ASCII symbols?
                v = value[index]
                if v == 0x0A: # \n
                    data.append(0x5C)
                    data.append(0x6E)
                elif v == 0x0D: # \r
                    data.append(0x5C)
                    data.append(0x72)
                elif v == 0x09: # \t
                    data.append(0x5C)
                    data.append(0x74)
                elif v == 0x22: # "
                    data.append(0x5C)
                    data.append(0x22)
                elif v == 0x5C: # \
                    data.append(0x5C)
                    data.append(0x5C)
                else:
                    data.append (v)
                index = index + 1
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
            if err == 1:
                suffix = 'LOAD_ERROR'
            elif err == 2:
                suffix = 'INVALID_PARAMETER'
            elif err == 3:
                suffix = 'UNSUPPORTED'
            elif err == 4:
                suffix = 'BAD_BUFFER_SIZE'
            elif err == 5:
                suffix = 'BUFFER_TOO_SMALL'
            elif err == 6:
                suffix = 'NOT_READY'
            elif err == 7:
                suffix = 'DEVICE_ERROR'
            elif err == 8:
                suffix = 'WRITE_PROTECTED'
            elif err == 9:
                suffix = 'OUT_OF_RESOURCES'
            elif err == 10:
                suffix = 'VOLUME_CORRUPTED'
            elif err == 11:
                suffix = 'VOLUME_FULL'
            elif err == 12:
                suffix = 'NO_MEDIA'
            elif err == 13:
                suffix = 'MEDIA_CHANGED'
            elif err == 14:
                suffix = 'NOT_FOUND'
            elif err == 15:
                suffix = 'ACCESS_DENIED'
            elif err == 16:
                suffix = 'NO_RESPONSE'
            elif err == 17:
                suffix = 'NO_MAPPING'
            elif err == 18:
                suffix = 'TIMEOUT'
            elif err == 19:
                suffix = 'NOT_STARTED'
            elif err == 20:
                suffix = 'ALREADY_STARTED'
            elif err == 21:
                suffix = 'ABORTED'
            elif err == 22:
                suffix = 'ICMP_ERROR'
            elif err == 23:
                suffix = 'TFTP_ERROR'
            elif err == 24:
                suffix = 'PROTOCOL_ERROR'
            elif err == 25:
                suffix = 'INCOMPATIBLE_VERSION'
            elif err == 26:
                suffix = 'SECURITY_VIOLATION'
            elif err == 27:
                suffix = 'CRC_ERROR'
            elif err == 28:
                suffix = 'END_OF_MEDIA'
            elif err == 31:
                suffix = 'END_OF_FILE'
            elif err == 32:
                suffix = 'INVALID_LANGUAGE'
            elif err == 33:
                suffix = 'COMPROMISED_DATA'
            elif err == 35:
                suffix = 'HTTP_ERROR'
            elif efi and err == 100:
                suffix = 'NETWORK_UNREACHABLE'
            elif efi and err == 101:
                suffix = 'HOST_UNREACHABLE'
            elif efi and err == 102:
                suffix = 'PROTOCOL_UNREACHABLE'
            elif efi and err == 103:
                suffix = 'PORT_UNREACHABLE'
            elif efi and err == 104:
                suffix = 'CONNECTION_FIN'
            elif efi and err == 105:
                suffix = 'CONNECTION_RESET'
            elif efi and err == 106:
                suffix = 'CONNECTION_REFUSED'
        else:
            if val == 0:
                suffix = 'SUCCESS'
            elif val == 1:
                suffix = 'WARN_UNKNOWN_GLYPH'
            elif val == 2:
                suffix = 'WARN_DELETE_FAILURE'
            elif val == 3:
                suffix = 'WARN_WRITE_FAILURE'
            elif val == 4:
                suffix = 'WARN_BUFFER_TOO_SMALL'
            elif val == 5:
                suffix = 'WARN_STALE_DATA'
            elif val == 6:
                suffix = 'WARN_FILE_SYSTEM'
        if suffix != '':
            return ('EFI_' if efi else 'RETURN_') + suffix
        return hex(val)

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
