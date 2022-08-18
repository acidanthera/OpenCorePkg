# pylint: disable=undefined-variable,import-error,too-many-boolean-expressions
#
# Extract platform data from CCC
# Copyright (c) 2018 vit9696
#

import struct
import ida_bytes


def readaddr(ea):
    a = ida_bytes.get_bytes(ea, 8)
    if a == BADADDR:
        return None
    return struct.unpack("L", a)[0]


def readstr(ea, length=256):
    if ea == BADADDR:
        return None

    string = ""
    while 1:
        c = ida_bytes.get_bytes(ea, 1)
        if c == BADADDR:
            if string == "":
                return None
            return string
        if c == "\0":
            return string

        string += c
        ea += 1
        if len(string) > length:
            return str


base = get_name_ea(0, "_show")
count = 0
print('typedef enum {')
while 1:
    s = readstr(readaddr(base))
    if s is None:
        break
    info = s.split('-', 1)
    print(f"  {info[0].strip().replace(',', '_')}, // {info[1].strip()}")
    base += 8
    count += 1

    name = get_name(base)
    if name is not None and name != "" and name != "_show":
        break
print('} AppleModel;')
print(f'#define APPLE_MODEL_MAX {count}')

base = get_name_ea(0, "_ApplePlatformData")
print('static PLATFORMDATA ApplePlatformData[] = {')
while 1:
    product_name = readstr(readaddr(base + 0))
    firmware_version = readstr(readaddr(base + 8))  # board version
    board_id = readstr(readaddr(base + 16))
    product_family = readstr(readaddr(base + 24))
    system_version = readstr(readaddr(base + 32))  # bios version
    serial_number = readstr(readaddr(base + 40))
    chassis_asset = readstr(readaddr(base + 48))
    smc_revision = struct.unpack("BBBBBB", ida_bytes.get_bytes(base + 56, 6))
    unknown_value = readaddr(base + 64)
    smc_branch = readstr(readaddr(base + 72))
    smc_platform = readstr(readaddr(base + 80))
    smc_config = readaddr(base + 88)

    if product_name is None or firmware_version is None or board_id is None or product_family is None or system_version is None or serial_number is None or chassis_asset is None or smc_branch is None or smc_config is None:
        break

    print(f'  {{ "{product_name}", "{firmware_version}", "{board_id}",')
    print(f'    "{product_family}", "{system_version}", "{serial_number}", "{chassis_asset}",')
    print(f'    {{ 0x{smc_revision[0]:02X}, 0x{smc_revision[1]:02X}, 0x{smc_revision[2]:02X}, 0x{smc_revision[3]:02X}, 0x{smc_revision[4]:02X}, 0x{smc_revision[5]:02X} }}, "{smc_branch}", "{smc_platform}", 0x{smc_config:08X} }},')

    base += 0x60

    name = get_name(base)
    if name is not None and name not in ["", "_ApplePlatformData"]:
        break
print('};')

base = get_name_ea(0, "_ModelCode")
print('static const char *AppleModelCode[] = {')
while 1:
    s = readstr(readaddr(base))
    if s is None:
        break

    print(f'  "{s}",')
    base += 8

    name = get_name(base)
    if name not in [None, '', '_ModelCode']:
        break
print('};')
