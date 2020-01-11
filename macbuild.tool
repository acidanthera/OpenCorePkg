#!/bin/bash

package() {
  if [ ! -d "$1" ]; then
    echo "Missing package directory"
    exit 1
  fi

  local ver=$(cat Include/OpenCore.h | grep OPEN_CORE_VERSION | sed 's/.*"\(.*\)".*/\1/' | grep -E '^[0-9.]+$')
  if [ "$ver" = "" ]; then
    echo "Invalid version $ver"
  fi

  selfdir=$(pwd)
  pushd "$1" || exit 1
  rm -rf tmp || exit 1
  mkdir -p tmp/EFI || exit 1
  mkdir -p tmp/EFI/OC || exit 1
  mkdir -p tmp/EFI/OC/ACPI || exit 1
  mkdir -p tmp/EFI/OC/Drivers || exit 1
  mkdir -p tmp/EFI/OC/Kexts || exit 1
  mkdir -p tmp/EFI/OC/Tools || exit 1
  mkdir -p tmp/EFI/BOOT || exit 1
  mkdir -p tmp/Docs/AcpiSamples || exit 1
  mkdir -p tmp/Utilities || exit 1
  cp OpenCore.efi tmp/EFI/OC/ || exit 1
  cp BOOTx64.efi tmp/EFI/BOOT/ || exit 1
  cp FwRuntimeServices.efi tmp/EFI/OC/Drivers/ || exit 1
  cp CleanNvram.efi tmp/EFI/OC/Tools/ || exit 1
  cp VerifyMsrE2.efi tmp/EFI/OC/Tools/ || exit 1
  cp "${selfdir}/Docs/Configuration.pdf" tmp/Docs/ || exit 1
  cp "${selfdir}/Docs/Differences/Differences.pdf" tmp/Docs/ || exit 1
  cp "${selfdir}/Docs/Sample.plist" tmp/Docs/ || exit 1
  cp "${selfdir}/Docs/SampleFull.plist" tmp/Docs/ || exit 1
  cp "${selfdir}/Changelog.md" tmp/Docs/ || exit 1
  cp -r "${selfdir}/Docs/AcpiSamples/" tmp/Docs/AcpiSamples/ || exit 1
  cp -r "${selfdir}/UDK/OcSupportPkg/Utilities/BootInstall" tmp/Utilities/ || exit 1
  cp -r "${selfdir}/UDK/OcSupportPkg/Utilities/CreateVault" tmp/Utilities/ || exit 1
  cp -r "${selfdir}/UDK/OcSupportPkg/Utilities/LogoutHook" tmp/Utilities/ || exit 1
  pushd tmp || exit 1
  zip -qry -FS ../"OpenCore-${ver}-${2}.zip" * || exit 1
  popd || exit 1
  rm -rf tmp || exit 1
  popd || exit 1
}

cd $(dirname "$0")
ARCHS=(X64 IA32)
SELFPKG=OpenCorePkg
DEPNAMES=('EfiPkg' 'OcSupportPkg' 'MacInfoPkg')
DEPURLS=('https://github.com/acidanthera/EfiPkg' 'https://github.com/acidanthera/OcSupportPkg' 'https://github.com/acidanthera/MacInfoPkg')
DEPBRANCHES=('master' 'master' 'master')
src=$(/usr/bin/curl -Lfs https://raw.githubusercontent.com/acidanthera/ocbuild/master/efibuild.sh) && eval "$src" || exit 1
