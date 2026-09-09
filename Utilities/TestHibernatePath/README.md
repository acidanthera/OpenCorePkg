# Hibernation device path regression test

Build and run the actual resolver with mocked EFI Block I/O handles:

```sh
make DEBUG=1 SANITIZE=1 WERROR=1
./TestHibernatePath
```

An optional argument accepts a private raw `boot-image` device-path fixture. It verifies that the complete image suffix survives the replacement byte-for-byte. Do not include machine-specific fixture files in a public patch.

The tests cover Apple/standard NVMe nodes, ambiguous namespaces, PCI-prefix mismatches, partition/removable/absent media, protocol lookup failure, already-working paths, invalid replacement resolution, and unsupported input shapes.
