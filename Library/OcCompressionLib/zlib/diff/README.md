zlib for OpenCorePkg
============================

OcCompressionLib would not be possible without the original [zlib](https://github.com/madler/zlib) project written by [Mark Adler](https://github.com/madler).

This is a modified version of zlib for the smooth integration into OpenCore bootloader. The current version is adapted based on [zlib 1.2.13](https://github.com/madler/zlib/releases/tag/v1.2.13).

Only the header or source code files listed in the in `OcCompressionLib.inf`, section `[Sources]`, will be needed, together with the following modifications.

- ***inflate.c***

OpenCore adds references to its own macro `OC_INFLATE_VERIFY_DATA` for optimum performance.

- ***zconf.h***

OpenCore's configuration header. This file should be left as is as long as it does not break compilation.

- ***zlib_uefi.c***

UEFI implementation for zlib. This file is an addition from OpenCore and should be left as is.

- ***zutil.c***

Simplified version, with `z_errmsg` only. This file should be left as is as long as it does not break compilation.

- ***zutil.h***

Additions of memory functions aliasing UEFI counterparts.
