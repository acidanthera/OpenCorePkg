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
- `FUZZ_MEM=1024` - run with 1024 MB fuzzing memory limit (defaults to 4096)*.
- `SANITIZE=1` — build with LLVM sanitizers enabled.
- `CC=cc` — build with `cc` compiler (e.g. `i686-w64-mingw32-gcc` for Windows).
- `DIST=Target` — build for target `Target` (e.g. `Darwin`, `Linux`, `Windows`).
- `STRIP=strip` — build with `strip` stripping tool (e.g. `i686-w64-mingw32-strip` for Windows).
- `OC_PATH` - path to OpenCorePkg for out-of-tree utilities (defaults to `../..`).
- `UDK_ARCH=Ia32` — build with 32-bit UDK architecture (defaults to `X64`).
- `UDK_PATH=/path/to/UDK` — build with custom UDK path (defaults to `$PACKAGES_PATH`).
- `WERROR=1` — treat compiler warnings as errors.
- `SYDR=1` — change `$(SUFFIX)` to store compilation results for Sydr DSE in a directory distinct from the default one.
- `USE_SHARED_OBJS=1` to speed up build if building multiple tools sequentially
   (not suitable for multiple builds in parallel, as this causes race conditions). 

**Note 1**: If your program uses `UserBaseMemoryLib` and calls custom allocation functions, be sure that besides `FUZZ_MEM` limit you correctly set limit `mPoolAllocationSizeLimit` which defaults to the 512 MB in cases if your code could allocate more than this limit at single AllocatePool.

To set up your limit, use `SetPoolAllocationSizeLimit` routine like shown in the example below:

```
...
#include <UserMemory.h>

int main(int argc, char *argv[]) {
  SetPoolAllocationSizeLimit (SINGLE_ALLOCATION_MEMORY_LIMIT);
  /* your code goes here */
  return 0;
}
```

**Note 2**: Projects which will sometimes be built using `USE_SHARED_OBJS=1` (e.g. those
built by `buildutil()` in `build_oc.tool`) must NOT:

- Modify `SHARED_OBJS` or `SHARED_CFLAGS`.
- Use features such as `FUZZ=1`, `SANITIZE=1`, `COVERAGE=1`, `DEBUG=1` *in their default
  builds*, since these affect the flags used for all object files including shared ones.
  Not obeying this rule would result in different utilities expecting the same shared
  object files to be built with different flags, which is not supported.
  - However these flags can still safely be specified with the `make` command for these
    projects when building without `USE_SHARED_OBJS=1`; e.g. when fuzzing or debugging.
  - Any of these flags can also safely be used in the default builds of projects which are not
    built using `USE_SHARED_OBJS=1`, such as UDK/BaseTools ImageTool and MicroTool.

### Example 1

To build 32-bit version of utility on macOS (use High Sierra 10.13 or below):

```sh
UDK_ARCH=Ia32 make
```

### Example 2

To build for 32-bit Windows (requires MinGW installed) use the following command:

```sh
UDK_ARCH=Ia32 CC=i686-w64-mingw32-gcc STRIP=i686-w64-mingw32-strip DIST=Windows make
```

### Example 3

To build with LLVM sanitizers use the following command:

```sh
DEBUG=1 SANITIZE=1 make
```

### Example 4

Perform fuzzing and generate coverage report:

```sh
# MacPorts clang is used since Xcode clang has no fuzzing support.
CC=clang-mp-10 FUZZ=1 SANITIZE=1 DEBUG=1 make fuzz
# LCOV is required for running this command.
make clean
COVERAGE=1 DEBUG=1 make coverage
```

**Note**: fuzzing corpus is saved in `FUZZDICT`.

### Example 5

Perform fuzzing with the help of [Sydr](https://www.ispras.ru/en/technologies/crusher/) tool (path to which should be in `$PATH`):

```sh
CC=clang DEBUG=1 FUZZ=1 SANITIZE=1 make
CC=clang DEBUG=1 SYDR=1 make sydr-fuzz
# Optionally check for security predicates.
CC=clang DEBUG=1 SYDR=1 make sydr-fuzz-security
# Import Sydr inputs to FUZZDICT.
CC=clang DEBUG=1 SYDR=1 make sydr-fuzz-import
make clean
COVERAGE=1 DEBUG=1 make sydr-import-check
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
