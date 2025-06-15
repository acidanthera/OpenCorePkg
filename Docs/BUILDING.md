# Building OpenCorePkg

This document outlines the recommended environment and steps required to build OpenCorePkg and its documentation.

## Prerequisites

- macOS with the latest **Xcode** installed. The XCODE5 toolchain is used.
- [**NASM**](https://www.nasm.us)
- [**MTOC** and **mtoc.NEW**](https://github.com/acidanthera/ocbuild/tree/master/external)
- For documentation: a TeX Live installation providing `pdflatex` and `latexdiff`.

## Building OpenCore

From the repository root run:

```bash
./build_oc.tool
```

This invokes the EDK II build system and produces packages in the `Binaries` directory. Additional options are available via `./build_oc.tool --help`.

## Building the Documentation

To generate `Configuration.pdf` and related files run:

```bash
./Docs/BuildDocs.tool
```

Make sure the TeX prerequisites listed above are installed.

## `unamer()` helper

`build_oc.tool` defines a helper called `unamer()` which wraps `uname`. On Windows MSYS or MINGW shells `uname` returns strings like `MINGW64_NT-10.0`. The helper normalises this output to simply `Windows` so the script can detect the host platform reliably.
