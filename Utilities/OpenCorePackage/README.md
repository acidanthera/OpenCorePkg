OpenCorePackage
===========

This tool installs OpenCore UEFI and legacy on GPT-formatted disk
### UEFI environment and BIOS-based systems.
- The Main EFI folder is included in the Packages.
- If you want to use OC Legacy make sure you have built DuetPkg first otherwise you will only have an OC UEFI Package. [DuetPkg](https://github.com/acidanthera/DuetPkg)

##### Credit: [Acidanthera OpenCore](https://github.com/acidanthera/OpenCorePkg),  [JrCs, partutil](https://github.com/JrCs), [MountESP Team Clover](https://github.com/CloverHackyColor/CloverBootloader), [chris1111 Package](https://github.com/chris1111)


#### Build
```bash
Run Package.command
```

- Build a config.plist base on Docs/SampleCustom.plist to ➤ /OC-EFI/EFIROOTDIR/EFI/OC/config.plist
- Build a Resources/Themes to ➤ /OC-EFI/EFIROOTDIR/EFI/OC/Resources/Image



#### Readme [UEFI/Duet Installer Package](https://github.com/acidanthera/OpenCorePkg/master/Utilities/OpenCorePackage/Docs/ReadMe%20UEFI:Duet.pdf)

#### Readme [UEFI Only Installer Package](https://github.com/acidanthera/OpenCorePkg/master/Utilities/OpenCorePackage/Docs/ReadMe%20UEFI%20Only.pdf)

#### References [Apple Installer Package](https://developer.apple.com/library/archive/documentation/DeveloperTools/Reference/DistributionDefinitionRef/Chapters/Introduction.html#//apple_ref/doc/uid/TP40005370-CH1-SW1)
