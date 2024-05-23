This is an i386/x86_64 fat binary version of `nvramdump`, built for macOS
10.6+.

It is bundled into the OpenCore `Utilities/LogoutHook` release folder in
favour of the CI-built x86_64/arm64 10.9+ compatible version, since 32-bit
support and 10.6+ support is relevant to users of this utility.

Future updates to `nvramdump.c` will require this to be rebuilt manually,
which can be done on an Xcode version which supports 10.6+ and i386 with
`FATBIN32=1 make`.
