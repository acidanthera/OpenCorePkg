# Contributing

Thank you for your interest in OpenCorePkg. This document covers the basics of building the project, running tests, and submitting pull requests.

## Building

OpenCorePkg uses the EDK II build system. A macOS host with Xcode and the tools listed in [Docs/BUILDING.md](Docs/BUILDING.md) is recommended.

From the repository root run:

```bash
./build_oc.tool
```

This script builds OpenCorePkg and the utility tools. Extra options are available via `./build_oc.tool --help`.

## Running tests

The `Tests` directory contains several EDK II applications. To build a specific test run:

```bash
make -C Tests/<TestName>
```

The tests are EFI binaries that can be executed in a UEFI environment or virtual machine. Some tests are also built automatically when running `./build_oc.tool`.

## Submitting pull requests

1. Fork the repository and create a topic branch for your changes.
2. Ensure the project builds and tests run without errors.
3. Open a pull request against the `master` branch describing your changes in detail.
4. By contributing you agree to follow our [Code of Conduct](CODE_OF_CONDUCT.md).

