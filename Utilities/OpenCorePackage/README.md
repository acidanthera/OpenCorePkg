OpenCorePackage
===========

This tool installs OpenCore UEFI and legacy on GPT-formatted disk
### UEFI environment and BIOS-based systems.
- The Main EFI folder is included in the Packages.
- If you want to use OC Legacy make sure you have built DuetPkg first otherwise you will only have an OC UEFI Package. (https://github.com/acidanthera/DuetPkg)
- Build a config.plist base on Docs/SampleCustom.plist to ➤ /OC-EFI/EFIROOTDIR/EFI/OC/config.plist
- Build a Resources/Themes to ➤ /OC-EFI/EFIROOTDIR/EFI/OC/Resources/Image

### Readme [UEFI/Duet Installer Package]()

### Readme [UEFI Only Installer Package]()

### References [Apple Installer Package](https://developer.apple.com/library/archive/documentation/DeveloperTools/Reference/DistributionDefinitionRef/Chapters/Introduction.html#//apple_ref/doc/uid/TP40005370-CH1-SW1)
