#
# Extract platform data from CCC
# Copyright (c) 2018 vit9696
#

import ida_bytes
import struct

def readaddr(ea):
    a = ida_bytes.get_bytes(ea, 8)
    if a == BADADDR:
        return None
    return struct.unpack("L", a)[0]

def readstr(ea, l=256):
    if ea == BADADDR:
        return None

    str = ""
    while 1:
        c = ida_bytes.get_bytes(ea, 1)
        if c == BADADDR:
            if str == "":
                return None
            else:
                return str
        elif c == "\0":
            return str

        str += c
        ea += 1
        if len(str) > l:
            return str

base = get_name_ea(0, "_show")
count = 0
print 'typedef enum {'
while 1:
    s = readstr(readaddr(base))
    if s == None:
        break
    info = s.split('-', 1)
    print '  %s, // %s' % (info[0].strip().replace(',', '_'), info[1].strip())
    base += 8
    count += 1;

    name = get_name(base)
    if name != None and name != "" and name != "_show":
        break
print '} AppleModel;'
print '#define APPLE_MODEL_MAX %d' % count

base = get_name_ea(0, "_ApplePlatformData")
print 'static PLATFORMDATA ApplePlatformData[] = {'
while 1:
    productName     = readstr(readaddr(base+0))
    firmwareVersion = readstr(readaddr(base+8))  # board version
    boardID         = readstr(readaddr(base+16))
    productFamily   = readstr(readaddr(base+24))
    systemVersion   = readstr(readaddr(base+32)) # bios version
    serialNumber    = readstr(readaddr(base+40))
    chassisAsset    = readstr(readaddr(base+48))
    smcRevision     = struct.unpack("BBBBBB", ida_bytes.get_bytes(base+56, 6))
    unknownValue    = readaddr(base+64)
    smcBranch       = readstr(readaddr(base+72))
    smcPlatform     = readstr(readaddr(base+80))
    smcConfig       = readaddr(base+88)

    if productName == None or firmwareVersion == None or boardID == None or productFamily == None or systemVersion == None or serialNumber == None or chassisAsset == None or smcBranch == None or smcConfig == None:
        break

    print '  { "%s", "%s", "%s",' % (productName, firmwareVersion, boardID)
    print '    "%s", "%s", "%s", "%s",' % (productFamily, systemVersion, serialNumber, chassisAsset)
    print '    { 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X }, "%s", "%s", 0x%08X },' % (smcRevision[0], smcRevision[1], smcRevision[2], smcRevision[3], smcRevision[4], smcRevision[5], smcBranch, smcPlatform, smcConfig)

    base += 0x60

    name = get_name(base)
    if name != None and name != "" and name != "_ApplePlatformData":
        break
print '};'

base = get_name_ea(0, "_ModelCode")
print 'static const char *AppleModelCode[] = {'
while 1:
    s = readstr(readaddr(base))
    if s == None:
        break

    print '  "%s",' % s
    base += 8

    name = get_name(base)
    if name != None and name != "" and name != "_ModelCode":
        break
print '};'
