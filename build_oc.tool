#!/bin/bash

buildutil() {
  UTILS=(
    "AppleEfiSignTool"
    "EfiResTool"
    "disklabel"
    "icnspack"
    "macserial"
    "ocvalidate"
    "TestBmf"
    "TestDiskImage"
    "TestHelloWorld"
    "TestImg4"
    "TestKextInject"
    "TestMacho"
    "TestRsaPreprocess"
    "TestSmbios"
  )

  if [ "$HAS_OPENSSL_BUILD" = "1" ]; then
    UTILS+=("RsaTool")
  fi

  local cores
  cores=$(getconf _NPROCESSORS_ONLN)

  pushd "${selfdir}/Utilities" || exit 1
  for util in "${UTILS[@]}"; do
    cd "$util" || exit 1
    echo "Building ${util}..."
    make clean || exit 1
    make -j "$cores" || exit 1
    #
    # FIXME: Do not build RsaTool for Win32 without OpenSSL.
    #
    if [ "$util" = "RsaTool" ] && [ "$HAS_OPENSSL_W32BUILD" != "1" ]; then
      continue
    fi

    if [ "$(which i686-w64-mingw32-gcc)" != "" ]; then
      echo "Building ${util} for Windows..."
      UDK_ARCH=Ia32 CC=i686-w64-mingw32-gcc STRIP=i686-w64-mingw32-strip DIST=Windows make clean || exit 1
      UDK_ARCH=Ia32 CC=i686-w64-mingw32-gcc STRIP=i686-w64-mingw32-strip DIST=Windows make -j "$cores" || exit 1
    fi
    cd - || exit 1
  done
  popd || exit 
}

package() {
  if [ ! -d "$1" ]; then
    echo "Missing package directory $1"
    exit 1
  fi

  local ver
  ver=$(grep OPEN_CORE_VERSION ./Include/Acidanthera/OpenCore.h | sed 's/.*"\(.*\)".*/\1/' | grep -E '^[0-9.]+$')
  if [ "$ver" = "" ]; then
    echo "Invalid version $ver"
    ver="UNKNOWN"
  fi

  selfdir=$(pwd)
  pushd "$1" || exit 1
  rm -rf tmp || exit 1

  dirs=(
    "tmp/EFI/BOOT"
    "tmp/EFI/OC/ACPI"
    "tmp/EFI/OC/Bootstrap"
    "tmp/EFI/OC/Drivers"
    "tmp/EFI/OC/Kexts"
    "tmp/EFI/OC/Tools"
    "tmp/EFI/OC/Resources/Audio"
    "tmp/EFI/OC/Resources/Font"
    "tmp/EFI/OC/Resources/Image"
    "tmp/EFI/OC/Resources/Label"
    "tmp/Docs/AcpiSamples"
    "tmp/Utilities"
    )
  for dir in "${dirs[@]}"; do
    mkdir -p "${dir}" || exit 1
  done

  # copy OpenCore main program.
  cp OpenCore.efi tmp/EFI/OC/ || exit 1

  # Mark binaries to be recognisable by OcBootManagementLib.
  bootsig="${selfdir}/Library/OcBootManagementLib/BootSignature.bin"
  efiOCBMs=(
    "BOOTx64.efi"
    "OpenCore.efi"
    )
  for efiOCBM in "${efiOCBMs[@]}"; do
    dd if="${bootsig}" \
       of="${efiOCBM}" seek=64 bs=1 count=64 conv=notrunc || exit 1
  done
  cp BOOTx64.efi tmp/EFI/BOOT/ || exit 1
  cp BOOTx64.efi tmp/EFI/OC/Bootstrap/Bootstrap.efi || exit 1

  efiTools=(
    "BootKicker.efi"
    "ChipTune.efi"
    "CleanNvram.efi"
    "GopStop.efi"
    "HdaCodecDump.efi"
    "KeyTester.efi"
    "MmapDump.efi"
    "ResetSystem.efi"
    "RtcRw.efi"
    "OpenControl.efi"
    "VerifyMsrE2.efi"
    )
  for efiTool in "${efiTools[@]}"; do
    cp "${efiTool}" tmp/EFI/OC/Tools/ || exit 1
  done
  # Special case: OpenShell.efi
  cp Shell.efi tmp/EFI/OC/Tools/OpenShell.efi || exit 1

  efiDrivers=(
    "HiiDatabase.efi"
    "NvmExpressDxe.efi"
    "AudioDxe.efi"
    "CrScreenshotDxe.efi"
    "OpenCanopy.efi"
    "OpenRuntime.efi"
    "OpenUsbKbDxe.efi"
    "Ps2MouseDxe.efi"
    "Ps2KeyboardDxe.efi"
    "UsbMouseDxe.efi"
    "XhciDxe.efi"
    )
  for efiDriver in "${efiDrivers[@]}"; do
    cp "${efiDriver}" tmp/EFI/OC/Drivers/ || exit 1
  done

  docs=(
    "Configuration.pdf"
    "Differences/Differences.pdf"
    "Sample.plist"
    "SampleCustom.plist"
    )
  for doc in "${docs[@]}"; do
    cp "${selfdir}/Docs/${doc}" tmp/Docs/ || exit 1
  done
  cp "${selfdir}/Changelog.md" tmp/Docs/ || exit 1
  cp -r "${selfdir}/Docs/AcpiSamples/" tmp/Docs/AcpiSamples/ || exit 1

  utilScpts=(
    "LegacyBoot"
    "CreateVault"
    "LogoutHook"
    "macrecovery"
    "kpdescribe"
    )
  for utilScpt in "${utilScpts[@]}"; do
    cp -r "${selfdir}/Utilities/${utilScpt}" tmp/Utilities/ || exit 1
  done

  # Copy OpenDuetPkg booter.
  local arch
  local tgt
  local booter
  arch="$(basename "$(pwd)")"
  tgt="$(basename "$(dirname "$(pwd)")")"
  booter="$(pwd)/../../../OpenDuetPkg/${tgt}/${arch}/boot"

  if [ -f "${booter}" ]; then
    echo "Copying OpenDuetPkg boot file from ${booter}..."
    cp "${booter}" tmp/Utilities/LegacyBoot/boot || exit 1
  else
    echo "Failed to find OpenDuetPkg at ${booter}!"
  fi

  buildutil || exit 1
  utils=(
    "macserial"
    "ocvalidate"
    "disklabel"
    "icnspack"
    )
  for util in "${utils[@]}"; do
    dest="tmp/Utilities/${util}"
    mkdir -p "${dest}" || exit 1
    bin="${selfdir}/Utilities/${util}/${util}"
    cp "${bin}" "${dest}" || exit 1
    binEXE="${bin}.exe"
    if [ -f "${binEXE}" ]; then
      cp "${binEXE}" "${dest}" || exit 1
    fi
  done
  # additional docs for macserial.
  cp "${selfdir}/Utilities/macserial/FORMAT.md" tmp/Utilities/macserial/ || exit 1
  cp "${selfdir}/Utilities/macserial/README.md" tmp/Utilities/macserial/ || exit 1

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

src=$(curl -Lfs https://raw.githubusercontent.com/acidanthera/ocbuild/master/efibuild.sh) && eval "$src" || exit 1

cd Library/OcConfigurationLib || exit 1
./CheckSchema.py OcConfigurationLib.c || exit 1
