#!/bin/bash

package() {
  if [ ! -d "$1" ]; then
    echo "Missing package directory"
    exit 1
  fi

  local ver
  ver=$(grep OPEN_CORE_VERSION ./Include/Acidanthera/OpenCore.h | sed 's/.*"\(.*\)".*/\1/' | grep -E '^[0-9.]+$')
  if [ "$ver" = "" ]; then
    echo "Invalid version $ver"
  fi

  selfdir=$(pwd)
  pushd "$1" || exit 1
  rm -rf tmp || exit 1
  mkdir -p tmp/EFI/BOOT || exit 1
  mkdir -p tmp/EFI/OC/ACPI || exit 1
  mkdir -p tmp/EFI/OC/Bootstrap || exit 1
  mkdir -p tmp/EFI/OC/Drivers || exit 1
  mkdir -p tmp/EFI/OC/Kexts || exit 1
  mkdir -p tmp/EFI/OC/Tools || exit 1
  mkdir -p tmp/EFI/OC/Resources/Audio || exit 1
  mkdir -p tmp/EFI/OC/Resources/Font || exit 1
  mkdir -p tmp/EFI/OC/Resources/Image || exit 1
  mkdir -p tmp/EFI/OC/Resources/Label || exit 1
  mkdir -p tmp/Docs/AcpiSamples || exit 1
  mkdir -p tmp/Utilities || exit 1
  # Mark binaries to be recognisable by OcBootManagementLib.
  dd if="${selfdir}/Library/OcBootManagementLib/BootSignature.bin" \
     of=BOOTx64.efi seek=64 bs=1 count=64 conv=notrunc || exit 1
  dd if="${selfdir}/Library/OcBootManagementLib/BootSignature.bin" \
     of=OpenCore.efi seek=64 bs=1 count=64 conv=notrunc || exit 1
  cp BootKicker.efi tmp/EFI/OC/Tools/ || exit 1
  cp BOOTx64.efi tmp/EFI/BOOT/ || exit 1
  cp BOOTx64.efi tmp/EFI/OC/Bootstrap/Bootstrap.efi || exit 1
  cp ChipTune.efi tmp/EFI/OC/Tools/ || exit 1
  cp CleanNvram.efi tmp/EFI/OC/Tools/ || exit 1
  cp GopStop.efi tmp/EFI/OC/Tools/ || exit 1
  cp HdaCodecDump.efi tmp/EFI/OC/Tools/ || exit 1
  cp HiiDatabase.efi tmp/EFI/OC/Drivers/ || exit 1
  cp KeyTester.efi tmp/EFI/OC/Tools/ || exit 1
  cp MmapDump.efi tmp/EFI/OC/Tools/ || exit 1
  cp ResetSystem.efi tmp/EFI/OC/Tools || exit 1
  cp RtcRw.efi tmp/EFI/OC/Tools || exit 1
  cp NvmExpressDxe.efi tmp/EFI/OC/Drivers/ || exit 1
  cp AudioDxe.efi tmp/EFI/OC/Drivers/ || exit 1
  cp OpenCanopy.efi tmp/EFI/OC/Drivers/ || exit 1
  cp OpenControl.efi tmp/EFI/OC/Tools/ || exit 1
  cp OpenCore.efi tmp/EFI/OC/ || exit 1
  cp OpenRuntime.efi tmp/EFI/OC/Drivers/ || exit 1
  cp OpenUsbKbDxe.efi tmp/EFI/OC/Drivers/ || exit 1
  cp Ps2MouseDxe.efi tmp/EFI/OC/Drivers/ || exit 1
  cp Ps2KeyboardDxe.efi tmp/EFI/OC/Drivers/ || exit 1
  cp UsbMouseDxe.efi tmp/EFI/OC/Drivers/ || exit 1
  cp Shell.efi tmp/EFI/OC/Tools/OpenShell.efi || exit 1
  cp VerifyMsrE2.efi tmp/EFI/OC/Tools/ || exit 1
  cp XhciDxe.efi tmp/EFI/OC/Drivers/ || exit 1
  cp "${selfdir}/Docs/Configuration.pdf" tmp/Docs/ || exit 1
  cp "${selfdir}/Docs/Differences/Differences.pdf" tmp/Docs/ || exit 1
  cp "${selfdir}/Docs/Sample.plist" tmp/Docs/ || exit 1
  cp "${selfdir}/Docs/SampleFull.plist" tmp/Docs/ || exit 1
  cp "${selfdir}/Changelog.md" tmp/Docs/ || exit 1
  cp -r "${selfdir}/Docs/AcpiSamples/" tmp/Docs/AcpiSamples/ || exit 1
  cp -r "${selfdir}/Utilities/LegacyBoot" tmp/Utilities/ || exit 1
  cp -r "${selfdir}/Utilities/CreateVault" tmp/Utilities/ || exit 1
  cp -r "${selfdir}/Utilities/LogoutHook" tmp/Utilities/ || exit 1
  cp -r "${selfdir}/Utilities/macrecovery" tmp/Utilities/ || exit 1
  if [ -d "{selfdir}/Utilities/macserial/bin" ]; then
    cp -r "${selfdir}/Utilities/macserial/bin" tmp/Utilities/macserial || exit 1
  else
    mkdir -p tmp/Utilities/macserial || exit 1
  fi
  cp "${selfdir}/Utilities/macserial/FORMAT.md" tmp/Utilities/macserial/ || exit 1
  cp "${selfdir}/Utilities/macserial/README.md" tmp/Utilities/macserial/ || exit 1
  cp "${selfdir}/Utilities/disklabel/disklabel" tmp/Utilities/ || exit 1
  cp "${selfdir}/Utilities/icnspack/icnspack" tmp/Utilities/ || exit 1
  cp "${selfdir}/Utilities/kpdescribe/kpdescribe.sh" tmp/Utilities/ || exit 1
  pushd tmp || exit 1
  zip -qr -FS ../"OpenCore-${ver}-${2}.zip" ./* || exit 1
  popd || exit 1
  rm -rf tmp || exit 1
  popd || exit 1
}

cd "$(dirname "$0")" || exit 1
if [ "$ARCHS" = "" ]; then
  ARCHS=(X64 IA32)
  export ARCHS
fi
SELFPKG=OpenCorePkg
NO_ARCHIVES=0

export SELFPKG
export NO_ARCHIVES

./Utilities/macserial/build.tool || exit 1

src=$(curl -Lfs https://raw.githubusercontent.com/acidanthera/ocbuild/master/efibuild.sh) && eval "$src" || exit 1

if [ "$BUILD_UTILITIES" = "1" ]; then
  UTILS=(
    "AppleEfiSignTool"
    "EfiResTool"
    "disklabel"
    "icnspack"
    "RsaTool"
  )

  cd Utilities || exit 1
  for util in "${UTILS[@]}"; do
    cd "$util" || exit 1
    make || exit 1
    cd - || exit 1
  done
  cd .. || exit 1
fi

cd Library/OcConfigurationLib || exit 1
./CheckSchema.py OcConfigurationLib.c || exit 1
