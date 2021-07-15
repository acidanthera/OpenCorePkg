#!/bin/bash

buildutil() {
  UTILS=(
    "AppleEfiSignTool"
    "ACPIe"
    "EfiResTool"
    "LogoutHook"
    "acdtinfo"
    "disklabel"
    "icnspack"
    "macserial"
    "ocpasswordgen"
    "ocvalidate"
    "TestBmf"
    "TestCpuFrequency"
    "TestDiskImage"
    "TestHelloWorld"
    "TestImg4"
    "TestKextInject"
    "TestMacho"
    "TestMp3"
    "TestPeCoff"
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
    if [ "$(which x86_64-linux-musl-gcc)" != "" ]; then
      echo "Building ${util} for Linux..."
      STATIC=1 SUFFIX=.linux UDK_ARCH=X64 CC=x86_64-linux-musl-gcc STRIP=x86_64-linux-musl-strip DIST=Linux make clean || exit 1
      STATIC=1 SUFFIX=.linux UDK_ARCH=X64 CC=x86_64-linux-musl-gcc STRIP=x86_64-linux-musl-strip DIST=Linux make -j "$cores" || exit 1
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
  ver=$(grep OPEN_CORE_VERSION ./Include/Acidanthera/Library/OcMainLib.h | sed 's/.*"\(.*\)".*/\1/' | grep -E '^[0-9.]+$')
  if [ "$ver" = "" ]; then
    echo "Invalid version $ver"
    ver="UNKNOWN"
  fi

  selfdir=$(pwd)
  pushd "$1" || exit 1
  rm -rf tmp || exit 1

  dirs=(
    "tmp/Docs/AcpiSamples"
    "tmp/Utilities"
    )
  for dir in "${dirs[@]}"; do
    mkdir -p "${dir}" || exit 1
  done

  efidirs=(
    "EFI/BOOT"
    "EFI/OC/ACPI"
    "EFI/OC/Drivers"
    "EFI/OC/Kexts"
    "EFI/OC/Tools"
    "EFI/OC/Resources/Audio"
    "EFI/OC/Resources/Font"
    "EFI/OC/Resources/Image"
    "EFI/OC/Resources/Label"
    )

  # Switch to parent architecture directory (i.e. Build/X64 -> Build).
  local dstdir
  dstdir="$(pwd)/tmp"
  pushd .. || exit 1

  for arch in "${ARCHS[@]}"; do
    for dir in "${efidirs[@]}"; do
      mkdir -p "${dstdir}/${arch}/${dir}" || exit 1
    done

    # Mark binaries to be recognisable by OcBootManagementLib.
    bootsig="${selfdir}/Library/OcBootManagementLib/BootSignature.bin"
    efiOCBMs=(
      "Bootstrap.efi"
      "OpenCore.efi"
      )
    for efiOCBM in "${efiOCBMs[@]}"; do
      dd if="${bootsig}" \
         of="${arch}/${efiOCBM}" seek=64 bs=1 count=64 conv=notrunc || exit 1
    done

    # copy OpenCore main program.
    ocflavour="${selfdir}/Library/OcBootManagementLib/.contentFlavour"
    cp "${arch}/OpenCore.efi" "${dstdir}/${arch}/EFI/OC" || exit 1
    cp "${ocflavour}" "${dstdir}/${arch}/EFI/OC" || exit 1

    local suffix="${arch}"
    if [ "${suffix}" = "X64" ]; then
      suffix="x64"
    fi
    cp "${arch}/Bootstrap.efi" "${dstdir}/${arch}/EFI/BOOT/BOOT${suffix}.efi" || exit 1
    cp "${ocflavour}" "${dstdir}/${arch}/EFI/BOOT" || exit 1

    efiTools=(
      "BootKicker.efi"
      "ChipTune.efi"
      "CleanNvram.efi"
      "CsrUtil.efi"
      "GopStop.efi"
      "KeyTester.efi"
      "MmapDump.efi"
      "ResetSystem.efi"
      "RtcRw.efi"
      "OpenControl.efi"
      "ControlMsrE2.efi"
      )
    for efiTool in "${efiTools[@]}"; do
      cp "${arch}/${efiTool}" "${dstdir}/${arch}/EFI/OC/Tools"/ || exit 1
    done

    # Special case: OpenShell.efi
    cp "${arch}/Shell.efi" "${dstdir}/${arch}/EFI/OC/Tools/OpenShell.efi" || exit 1

    efiDrivers=(
      "HiiDatabase.efi"
      "NvmExpressDxe.efi"
      "AudioDxe.efi"
      "CrScreenshotDxe.efi"
      "OpenCanopy.efi"
      "OpenPartitionDxe.efi"
      "OpenRuntime.efi"
      "OpenUsbKbDxe.efi"
      "Ps2MouseDxe.efi"
      "Ps2KeyboardDxe.efi"
      "UsbMouseDxe.efi"
      "OpenHfsPlus.efi"
      "XhciDxe.efi"
      )
    for efiDriver in "${efiDrivers[@]}"; do
      cp "${arch}/${efiDriver}" "${dstdir}/${arch}/EFI/OC/Drivers"/ || exit 1
    done
  done

  docs=(
    "Configuration.pdf"
    "Differences/Differences.pdf"
    "Sample.plist"
    "SampleCustom.plist"
    )
  for doc in "${docs[@]}"; do
    cp "${selfdir}/Docs/${doc}" "${dstdir}/Docs"/ || exit 1
  done
  cp "${selfdir}/Changelog.md" "${dstdir}/Docs"/ || exit 1
  cp -r "${selfdir}/Docs/AcpiSamples/"* "${dstdir}/Docs/AcpiSamples"/ || exit 1

  mkdir -p "${dstdir}/Docs/AcpiSamples/Binaries" || exit 1
  cd "${dstdir}/Docs/AcpiSamples/Source" || exit 1
  for i in *.dsl ; do
    iasl "$i" || exit 1
  done
  mv ./*.aml "${dstdir}/Docs/AcpiSamples/Binaries" || exit 1
  cd - || exit 1

  utilScpts=(
    "LegacyBoot"
    "CreateVault"
    "macrecovery"
    "kpdescribe"
    )
  for utilScpt in "${utilScpts[@]}"; do
    cp -r "${selfdir}/Utilities/${utilScpt}" "${dstdir}/Utilities"/ || exit 1
  done

  buildutil || exit 1

  # Copy LogoutHook.
  mkdir -p "${dstdir}/Utilities/LogoutHook" || exit 1
  logoutFiles=(
    "LogoutHook.command"
    "README.md"
    "nvramdump"
    )
  for file in "${logoutFiles[@]}"; do
    cp "${selfdir}/Utilities/LogoutHook/${file}" "${dstdir}/Utilities/LogoutHook"/ || exit 1
  done

  # Copy OpenDuetPkg booter.
  for arch in "${ARCHS[@]}"; do
    local tgt
    local booter
    tgt="$(basename "$(pwd)")"
    booter="$(pwd)/../../OpenDuetPkg/${tgt}/${arch}/boot"

    if [ -f "${booter}" ]; then
      echo "Copying OpenDuetPkg boot file from ${booter}..."
      cp "${booter}" "${dstdir}/Utilities/LegacyBoot/boot${arch}" || exit 1
    else
      echo "Failed to find OpenDuetPkg at ${booter}!"
    fi
  done

  utils=(
    "ACPIe"
    "acdtinfo"
    "macserial"
    "ocpasswordgen"
    "ocvalidate"
    "disklabel"
    "icnspack"
    )
  for util in "${utils[@]}"; do
    dest="${dstdir}/Utilities/${util}"
    mkdir -p "${dest}" || exit 1
    bin="${selfdir}/Utilities/${util}/${util}"
    cp "${bin}" "${dest}" || exit 1
    if [ -f "${bin}.exe" ]; then
      cp "${bin}.exe" "${dest}" || exit 1
    fi
    if [ -f "${bin}.linux" ]; then
      cp "${bin}.linux" "${dest}" || exit 1
    fi
  done
  # additional docs for macserial.
  cp "${selfdir}/Utilities/macserial/FORMAT.md" "${dstdir}/Utilities/macserial"/ || exit 1
  cp "${selfdir}/Utilities/macserial/README.md" "${dstdir}/Utilities/macserial"/ || exit 1
  # additional docs for ocvalidate.
  cp "${selfdir}/Utilities/ocvalidate/README.md" "${dstdir}/Utilities/ocvalidate"/ || exit 1

  pushd "${dstdir}" || exit 1
  zip -qr -FS ../"OpenCore-${ver}-${2}.zip" ./* || exit 1
  popd || exit 1
  rm -rf "${dstdir}" || exit 1

  popd || exit 1
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
