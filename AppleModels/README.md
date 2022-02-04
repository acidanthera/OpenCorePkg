Apple Models
===========

Various information about Apple hardware
([current database status](https://github.com/acidanthera/OpenCorePkg/blob/master/AppleModels/DataBase.md)).

## Improving database

To add a new hardware board, please create a file in DataBase
directory, and then run `./update_generated.py`. It should not
output anything and return zero code.

To install PyYAML on macOS use the following commands:

```
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
sudo -H python get-pip.py
sudo -H pip install pyyaml
```

## Unpacking firmware

To update the database you can either get the information from firmware images or from
running hardware. There currently are two places for firmware images: `FirmwareUpdate.pkg`
for generic models and `BridgeOSUpdateCustomer.pkg` for T2 models (the entire restore list
is available at [mesu.apple.com](https://mesu.apple.com/assets/bridgeos/com_apple_bridgeOSIPSW/com_apple_bridgeOSIPSW.xml))
To use them do as follows:

1. Visit suitable update catalogue by filling the OS versions (e.g. [this](https://swscan.apple.com/content/catalogs/others/index-12seed-12-10.16-10.15-10.14-10.13-10.12-10.11-10.10-10.9-mountainlion-lion-snowleopard-leopard.merged-1.sucatalog.gz) one for macOS 12 beta).  
Note: This catalogue not include `FirmwareUpdate.pkg` for macOS 11+. You can extract  firmware images for generic models from `Install macOS Big Sur.app` (or newer).
2. Download most recent `FirmwareUpdate.pkg` and `BridgeOSUpdateCustomer.pkg`.
3. Extract `scap` files from `FirmwareUpdate.pkg` files and use them as is.
4. Extract `/usr/standalone/firmware/bridgeOSCustomer.bundle/Contents/Resources/UpdateBundle.zip`
   file from `BridgeOSUpdateCustomer.pkg` and unpack it.
5. After unpacking `UpdateBundle.zip` go to `boot/Firmware/MacEFI` directory and unpack im4p files.
   You can use [img4](https://github.com/xerub/img4lib), [img4tool](https://github.com/tihmstar/img4tool), or our dedicated [MacEfiUnpack](https://github.com/acidanthera/OpenCorePkg/blob/master/Utilities/MacEfiUnpack/MacEfiUnpack.py). For example, `for i in *.im4p ; do ./MacEfiUnpack.py "$i" ; done`
