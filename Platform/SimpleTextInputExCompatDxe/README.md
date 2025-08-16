# SimpleTextInputExCompatDxe
UEFI DXE driver to provide SimpleTextInputEx protocol compatibility for EFI 1.1 systems.

## Description
This DXE driver provides SimpleTextInputEx protocol support for EFI 1.1 systems that lack native support for this protocol. The driver is specifically designed to enable OpenShell text editor functionality on older UEFI implementations.

The driver creates a compatibility layer that:
- Implements the SimpleTextInputEx protocol interface
- Provides proper function key mappings (F1-F11) for OpenShell
- Handles control character detection and mapping (Ctrl+A through Ctrl+Z)
- Supports extended key combinations and special characters
- Maintains compatibility with existing SimpleTextInput protocol implementations

## Key Features
- **EFI 1.1 Compatibility**: Works with older UEFI firmware that only supports SimpleTextInput protocol
- **OpenShell Integration**: Specifically tested and optimized for OpenShell text editor
- **Function Key Support**: Complete F1-F11 function key mapping
- **Control Character Handling**: Proper Ctrl+key combinations for text editing
- **Non-intrusive**: Does not interfere with existing text input functionality

## Build
This is a standard EDK2-compatible DXE driver. Add it to your package's DSC file to include in the build process.

For OpenCore integration:
```
[Components]
  OpenCorePkg/Platform/SimpleTextInputExCompatDxe/SimpleTextInputExCompat.inf
```

## Usage
1. **Installation**: Load the driver through OpenCore or integrate into firmware
2. **Automatic Operation**: The driver automatically detects EFI 1.1 systems and installs the compatibility protocol
3. **OpenShell Compatibility**: Launch OpenShell - the text editor will now have full keyboard support

The driver operates transparently in the background and requires no user interaction once loaded.

## Technical Details
- **Module Type**: DXE_DRIVER
- **Protocols Consumed**: SimpleTextInput
- **Protocols Produced**: SimpleTextInputEx
- **Target Architecture**: X64, IA32, AARCH64
- **Debug Prefix**: STX (for OpenCore debug output)

## License
BSD 3-Clause License - see LICENSE file for details.
