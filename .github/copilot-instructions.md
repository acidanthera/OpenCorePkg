# OpenCorePkg Copilot Instructions

## Project Overview
OpenCorePkg is an EDK II-based UEFI firmware project implementing the OpenCore bootloader with extensive Apple-specific UEFI support libraries. The codebase provides a modular bootloader framework for macOS and other operating systems.

## Architecture
- **Core Application**: `Application/OpenCore/` - Main bootloader entry point using OcMainLib
- **Library Ecosystem**: `Library/Oc*` - 50+ specialized libraries for Apple UEFI functionality (ACPI, kernel patching, secure boot, etc.)
- **Platform Code**: `Platform/` - Hardware-specific implementations
- **Utilities**: `Utilities/` - Build tools and host utilities (built with `make USE_SHARED_OBJS=1`)
- **Tests**: `Tests/` - UEFI driver-based unit tests

## Build System
- **EDK II Framework**: Uses standard EDK II DSC/FDF files (`OpenCorePkg.dsc`, `OpenCorePkg.fdf`)
- **Docker Builds**: `docker-compose.yaml` provides containerized EDK2 environment
- **Build Scripts**:
  - `./build_oc.tool` - Main OpenCore build
  - `./build_duet.tool` - Legacy DuetPkg build
  - `./xcbuild.tool` - Xcode toolchain build
- **Utility Builds**: `make -j $(nproc) USE_SHARED_OBJS=1` in `Utilities/` subdirectories

## Configuration System
- **Format**: Property List (.plist) files parsed by OcConfigurationLib
- **Schema**: Defined in `Library/OcConfigurationLib/OcConfigurationLib.c` with validation via `CheckSchema.py`
- **Sample**: `Docs/Sample.plist` - Reference configuration with all options

## Code Patterns
- **Indentation**: 2 spaces (`.editorconfig`)
- **Formatting**: Uncrustify with excludes in `Uncrustify.yml` (third-party code excluded)
- **Library Dependencies**: Libraries specify `LIBRARY_CLASS` in .inf files for linking
- **Error Handling**: Use `EFI_STATUS` returns, `ASSERT` for debug builds
- **Memory Management**: `AllocatePool/FreePool` for dynamic allocation
- **String Handling**: Use `OcStringLib` for Unicode operations

## Development Workflow
1. **Setup**: Use Docker: `docker-compose run --rm build-oc`
2. **Build**: Run `./build_oc.tool` or use Docker compose services
3. **Test**: Build and run UEFI test drivers from `Tests/`
4. **Validate**: Use `ocvalidate` utility for configuration checking
5. **Format**: Run Uncrustify on modified files (excluding listed directories)

## Key Files
- `OpenCorePkg.dsc` - Package description and build configuration
- `Library/OcMainLib/` - Core bootloader logic coordination
- `Library/OcConfigurationLib/` - Plist parsing and validation
- `Docs/Configuration.tex` - Complete configuration reference
- `build_oc.tool` - Primary build script

## Common Tasks
- **Adding Libraries**: Create .inf with `LIBRARY_CLASS`, add to DSC `[LibraryClasses]`
- **Configuration Options**: Add to schema in OcConfigurationLib.c, update Sample.plist
- **UEFI Protocols**: Define GUIDs in .dec files, implement in libraries
- **Cross-Platform**: Support IA32/X64 architectures in .inf `VALID_ARCHITECTURES`

## External Dependencies
- **EDK II**: Base UEFI framework (MdePkg, MdeModulePkg, etc.)
- **Third-party**: zlib, lzss, lzvn, libDER, helix (excluded from formatting)
- **Build Tools**: LzmaCompress, ImageTool, EfiLdrImage for firmware image creation