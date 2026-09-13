# ATAPI device path regression test

Build and run the actual `OcDevicePathLib` generic resolver with mocked firmware
protocols:

```sh
make DEBUG=1 SANITIZE=1 WERROR=1
./TestAtapiPath
```

The tests enter through `OcFixAppleBootDevicePath`, including whole-disk paths,
partition/APFS/file suffixes, and a hibernation image path. They verify that the
complete suffix is retained, already working ATA/SATA/NVMe paths stay unchanged,
ambiguous or unsuitable disks are ignored, and a replacement must resolve.
Direct node-API tests also exercise successful replacement, restoration of the
original allocation/cursor, and committed-context cleanup under the sanitizers.
Pool allocation counters verify that each resolver call releases temporary paths
and that no allocations remain after the test suite.

An optional argument accepts a private raw `boot-image` fixture. It is tested
through the same generic API; machine-specific fixtures must not be published.

The host environment stubs unrelated partition-based short-form expansion;
these tests exercise the ATAPI replacement, not that existing expansion code.
