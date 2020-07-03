#!/bin/bash

imgbuild() {
  echo "Erasing older files..."
  rm -f "${BUILD_DIR}/FV/DUETEFIMAINFV${TARGETARCH}.z" \
    "${BUILD_DIR}/FV/DxeMain${TARGETARCH}.z" \
    "${BUILD_DIR}/FV/DxeIpl${TARGETARCH}.z" \
    "${BUILD_DIR_ARCH}/EfiLoaderRebased.efi" \
    "${BUILD_DIR}/FV/Efildr${TARGETARCH}" \
    "${BUILD_DIR}/FV/Efildr${TARGETARCH}Pure" \
    "${BUILD_DIR}/FV/Efildr${TARGETARCH}Out" \
    "${BUILD_DIR_ARCH}/boot"

  echo "Compressing DUETEFIMainFv.FV..."
  LzmaCompress -e -o "${BUILD_DIR}/FV/DUETEFIMAINFV${TARGETARCH}.z" \
    "${BUILD_DIR}/FV/DUETEFIMAINFV${TARGETARCH}.Fv" || exit 1

  echo "Compressing DxeCore.efi..."
  LzmaCompress -e -o "${BUILD_DIR}/FV/DxeMain${TARGETARCH}.z" \
    "${BUILD_DIR_ARCH}/DxeCore.efi" || exit 1

  echo "Compressing DxeIpl.efi..."
  LzmaCompress -e -o "${BUILD_DIR}/FV/DxeIpl${TARGETARCH}.z" \
    "$BUILD_DIR_ARCH/DxeIpl.efi" || exit 1

  echo "Generating Loader Image..."

  GenFw --rebase 0x10000 -o "${BUILD_DIR_ARCH}/EfiLoaderRebased.efi" \
    "${BUILD_DIR_ARCH}/EfiLoader.efi" || exit 1
  "${FV_TOOLS}/EfiLdrImage" -o "${BUILD_DIR}/FV/Efildr${TARGETARCH}" \
    "${BUILD_DIR_ARCH}/EfiLoaderRebased.efi" "${BUILD_DIR}/FV/DxeIpl${TARGETARCH}.z" \
    "${BUILD_DIR}/FV/DxeMain${TARGETARCH}.z" "${BUILD_DIR}/FV/DUETEFIMAINFV${TARGETARCH}.z" || exit 1

  # Calculate page table location for 64-bit builds.
  # Page table must be 4K aligned, bootsectors are 4K each, and 0x20000 is base address.
  if [ "${TARGETARCH}" = "X64" ]; then
    if [ "$(uname)" = "Darwin" ]; then
      EL_SIZE=$(stat -f "%z" "${BUILD_DIR}/FV/Efildr${TARGETARCH}")
    else
      EL_SIZE=$(stat --printf="%s\n" "${BUILD_DIR}/FV/Efildr${TARGETARCH}")
    fi
    PAGE_TABLE_OFF=$( printf "0x%x" $(( (EL_SIZE + 0x2000 + 0xFFF) & ~0xFFF )) )
    PAGE_TABLE=$( printf "0x%x" $(( PAGE_TABLE_OFF + 0x20000 )) )

    export PAGE_TABLE_OFF
    export PAGE_TABLE

    BOOTSECTOR_SUFFIX="_${PAGE_TABLE}"
  else
    BOOTSECTOR_SUFFIX=""
  fi

  # Build bootsectors.
  mkdir -p "${BOOTSECTORS}" || exit 1
  cd "${BOOTSECTORS}"/.. || exit 1
  make || exit 1
  cd - || exit 1

  # Concatenate bootsector into the resulting image.
  cat "${BOOTSECTORS}/Start${TARGETARCH}${BOOTSECTOR_SUFFIX}.com" "${BOOTSECTORS}/Efi${TARGETARCH}.com" \
    "${BUILD_DIR}/FV/Efildr${TARGETARCH}" > "${BUILD_DIR}/FV/Efildr${TARGETARCH}Pure" || exit 1

  # Append page table and skip empty data in 64-bit mode.
  if [ "${TARGETARCH}" = "X64" ]; then
    "${FV_TOOLS}/GenPage" "${BUILD_DIR}/FV/Efildr${TARGETARCH}Pure" \
      -b "${PAGE_TABLE}" -f "${PAGE_TABLE_OFF}" \
      -o "${BUILD_DIR}/FV/Efildr${TARGETARCH}Out" || exit 1

    dd if="${BUILD_DIR}/FV/Efildr${TARGETARCH}Out" of="${BUILD_DIR_ARCH}/boot" bs=512 skip=1 || exit 1
  else
    cp "${BUILD_DIR}/FV/Efildr${TARGETARCH}Pure" "${BUILD_DIR_ARCH}/boot" || exit 1
  fi
}

package() {
  if [ ! -d "$1" ]; then
    echo "Missing package directory $1"
    exit 1
  fi
  
  if [ ! -d "$1"/../FV ]; then
    echo "Missing FV directory $1/../FV"
    exit 1
  fi

  pushd "$1" || exit 1
  cd -P . || exit 1
  BUILD_DIR_ARCH=$(pwd)
  cd -P .. || exit 1
  BUILD_DIR=$(pwd)
  popd || exit 1

  imgbuild
}

cd "$(dirname "$0")" || exit 1

BOOTSECTORS="$(pwd)/Legacy/BootSector/bin"
UNAME="$(uname)"
if [ "$(echo "${UNAME}" | grep MINGW)" != "" ] || [ "$(echo "${UNAME}" | grep MSYS)" != "" ]; then
  UNAME="Windows"
fi
FV_TOOLS="$(pwd)/Utilities/BaseTools/bin.${UNAME}"

if [ ! -d "${FV_TOOLS}" ]; then
  echo "ERROR: You need to compile BaseTools for your platform!"
  exit 1
fi

if [ "${TARGETARCH}" = "" ]; then
  TARGETARCH="X64"
fi

if [ "${TARGET}" = "" ]; then
  TARGET="RELEASE"
fi

if [ "${INTREE}" != "" ]; then
  # In-tree compilation is merely for packing.
  cd .. || exit 1

  build -a "${TARGETARCH}" -b "${TARGET}" -t XCODE5 -p OpenCorePkg/OpenDuetPkg.dsc || exit 1
  
  BUILD_DIR="${WORKSPACE}/Build/OpenDuetPkg/${TARGET}_XCODE5"
  BUILD_DIR_ARCH="${BUILD_DIR}/${TARGETARCH}"
  imgbuild
else
  TARGETS=(DEBUG RELEASE)
  if [ "$ARCHS" = "" ]; then
    ARCHS=(X64 IA32)
    export ARCHS
  fi
  SELFPKG_DIR="OpenCorePkg"
  SELFPKG=OpenDuetPkg
  NO_ARCHIVES=1

  export TARGETS
  export SELFPKG_DIR
  export SELFPKG
  export NO_ARCHIVES

  src=$(curl -Lfs https://raw.githubusercontent.com/acidanthera/ocbuild/master/efibuild.sh) && eval "$src" || exit 1
fi
