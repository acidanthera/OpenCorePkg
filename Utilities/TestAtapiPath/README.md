# ATAPI device path regression test

Build and run `OcDevicePathLib` with mocked firmware protocols:

```sh
make DEBUG=1 SANITIZE=1 WERROR=1
./TestAtapiPath
```

The tests cover ATAPI expansion with disk, partition, APFS and file suffixes,
extended ATAPI nodes, multiple instances, removable NVMe disks, and firmware
paths without media metadata. Partition handles are excluded by their device
paths. Working paths remain unchanged, ambiguous matches are rejected, and the
replacement must resolve through Block I/O.

Allocation counters check the original-path cleanup, including an existing
VirtIO expansion. Context tests cover rollback, committed cleanup, a direct
node call without a restore context, and ACPI correction followed by ATAPI
expansion through the generic resolver.

An optional argument accepts a private raw `boot-image` fixture. Machine-specific
fixtures are not included in the repository.

The host environment stubs partition-based short-form expansion; it runs the
actual ATAPI and VirtIO node handlers.
