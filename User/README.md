Minimalist UEFI in Userspace
============================

This toolset allows one to quickly port UEFI code to userspace
utilities in popular operating systems such as macOS, Windows,
or Linux. The only requirement is a C11-supporting compiler
with the build scripts adopted for modern versions of clang
and gcc.

### Quick start

Create a `Makefile` with the contents similar to an example below:

```
PROJECT = ProjectName
PRODUCT = $(PROJECT)$(SUFFIX)
OBJS    = $(PROJECT).o ExtraObject.o

VPATH   = ../../Library/MyExtraLib:$

include ../../User/Makefile
CFLAGS += -I../../Library/MyExtraLib
```

Build it with the `make` command.

### Flags and arguments

Additional variables are supported to adjust the compilation process.

- `COVERAGE=1` — build with coverage information gathering.
- `DEBUG=1` — build with debugging information.
- `FUZZ=1` — build with fuzzing enabled.
- `FUZZ_JOBS=2` — run with 2 fuzzing jobs (defaults to 4).
- `FUZZ_MEM=1024` - run with 1024 MB fuzzing memory limit (defaults to 4096).
- `SANITIZE=1` — build with LLVM sanitizers enabled.
- `CC=cc` — build with `cc` compiler (e.g. `i686-w64-mingw32-gcc` for Windows).
- `DIST=Target` — build for target `Target` (e.g. `Darwin`, `Linux`, `Windows`).
- `STRIP=strip` — build with `strip` stripping tool (e.g. `i686-w64-mingw32-strip` for Windows).
- `OC_PATH` - path to OpenCorePkg for out-of-tree utilities (defaults to `../..`).
- `UDK_ARCH=Ia32` — build with 32-bit UDK architecture (defaults to `X64`).
- `UDK_PATH=/path/to/UDK` — build with custom UDK path (defaults to `$PACKAGES_PATH`).
- `WERROR=1` — treat compiler warnings as errors.

Example 1. To build for 32-bit Windows (requires MinGW installed) use the following command:

```sh
UDK_ARCH=Ia32 CC=i686-w64-mingw32-gcc STRIP=i686-w64-mingw32-strip DIST=Windows make
```

Example 2. To build with LLVM sanitizers use the following command:

```sh
DEBUG=1 SANITIZE=1 make
```

Example 3. Perform fuzzing and generate coverage report:

```sh
# MacPorts clang is used since Xcode clang has no fuzzing support.
CC=clang-mp-10 FUZZ=1 SANITIZE=1 DEBUG=1 make fuzz
# LCOV is required for running this command.
make clean
COVERAGE=1 DEBUG=1 make coverage
```

### Predefined variables

Most UDK variables are available due to including the original headers.

- To distinguish target platform `MDE_CPU_*` variables can be used.
  For example, `MDE_CPU_IA32` for 32-bit Intel and `MDE_CPU_X64` for 64-bit Intel.
- To distinguish from normal UEFI compilation use `EFIUSER` variable.
- To detect debug build use `EFIUSER_DEBUG` variable.
- To detect sanitizing status use `SANITIZE_TEST`.
- To detect fuzzing status use `FUZZING_TEST`.
- Use `ENTRY_POINT` variable for `main` to automatically disable it for fuzzing.

