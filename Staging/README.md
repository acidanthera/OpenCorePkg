Staging drivers considered experimental and not production ready.

## VBoxHfs
This driver, based on [VBoxHfs](https://www.virtualbox.org/browser/vbox/trunk/src/VBox/Devices/EFI/Firmware/VBoxPkg/VBoxFsDxe) from [VirtualBox OSE](https://www.virtualbox.org) project driver, implements HFS+ support with bless extensions. Commit history can be found in [VBoxFsDxe](https://github.com/nms42/VBoxFsDxe) repository. Note, that unlike other drivers, its source code is licensed under GPLv2.

## AudioDxe
Improved audio driver (currently only Intel HD audio).  
HDMI or other digital outputs don't work.
