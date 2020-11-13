Apple Models
===========

更多关于 Apple 硬件的信息：
([现在数据库状态](https://github.com/acidanthera/OpenCorePkg/blob/master/AppleModels/DataBase.md)).

## 语言
- 简体中文 (当前语言)
- [English](https://github.com/acidanthera/OpenCorePkg/blob/master/AppleModels/README.md)

## 改进数据库

要添加新的硬件，请在 DataBase 目录创建一个文件，然后运行 `./update_generated.py`。该操作应该不会返回任何输出代码。

若要在 macOS 上安装 PyYAML 请使用以下命令:

```
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
sudo -H python get-pip.py
sudo -H pip install pyyaml
```

## 解包固件

若要更新数据库，你可以通过固件镜像文件或者在硬件设备上获得必要的信息。 目前有两个目录包含了固件镜像：generic models 的 `FirmwareUpdate.pkg` 以及支持 T2 芯片型号的 `BridgeOSUpdateCustomer.pkg` （完整的恢复列表可以在 [mesu.apple.com](https://mesu.apple.com/assets/bridgeos/com_apple_bridgeOSIPSW/com_apple_bridgeOSIPSW.xml) 找到。) 

请按照以下步骤操作：

1. 通过填写系统版本来访问合适的升级目录 (例如 [这个](https://swscan.apple.com/content/catalogs/others/index-10.16seed-10.16-10.15-10.14-10.13-10.12-10.11-10.10-10.9-mountainlion-lion-snowleopard-leopard.merged-1.sucatalog.gz) 是 macOS 11.0 beta 版)。
2. 下载最新版本的 `FirmwareUpdate.pkg` 和 `BridgeOSUpdateCustomer.pkg`。
3. 从 `FirmwareUpdate.pkg` 提取 `scap` 文件。
4. 从 `BridgeOSUpdateCustomer.pkg` 提取 `/usr/standalone/firmware/bridgeOSCustomer.bundle/Contents/Resources/UpdateBundle.zip`
  并将其解包。
5. 在解包 `UpdateBundle.zip` 后前往 `boot/Firmware/MacEFI` 目录并解包 im4p 文件。
   你可以使用 [img4](https://github.com/xerub/img4lib), [img4tool](https://github.com/tihmstar/img4tool), 或者我们推荐的 [MacEfiUnpack](https://github.com/acidanthera/OpenCorePkg/blob/master/Utilities/MacEfiUnpack/MacEfiUnpack.py) 来进行操作。 例如， `for i in *.im4p ; do ./MacEfiUnpack.py "$i" ; done`
