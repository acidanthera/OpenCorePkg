#!/bin/bash

imgbuild() {
  local arch="$1"

  echo "Erasing older files..."
  rm -f "${BUILD_DIR}/FV/DUETEFIMAINFV${arch}.z" \
    "${BUILD_DIR}/FV/DxeMain${arch}.z" \
    "${BUILD_DIR}/FV/DxeIpl${arch}.z" \
    "${BUILD_DIR_ARCH}/EfiLoaderRebased.efi" \
    "${BUILD_DIR}/FV/Efildr${arch}" \
    "${BUILD_DIR}/FV/Efildr${arch}Pure" \
    "${BUILD_DIR}/FV/Efildr${arch}Out" \
    "${BUILD_DIR}/FV/EfildrBlockIo${arch}" \
    "${BUILD_DIR}/FV/EfildrBlockIo${arch}Pure" \
    "${BUILD_DIR}/FV/EfildrBlockIo${arch}Out" \
    "${BUILD_DIR_ARCH}/boot" \
    "${BUILD_DIR_ARCH}/boot-blockio"

  echo "Compressing DUETEFIMainFv.FV..."
  LzmaCompress -e -o "${BUILD_DIR}/FV/DUETEFIMAINFV${arch}.z" \
    "${BUILD_DIR}/FV/DUETEFIMAINFV${arch}.Fv" || exit 1

  echo "Compressing DUETEFIMainFvBlockIo.FV..."
  LzmaCompress -e -o "${BUILD_DIR}/FV/DUETEFIMAINFVBLOCKIO${arch}.z" \
    "${BUILD_DIR}/FV/DUETEFIMAINFVBLOCKIO${arch}.Fv" || exit 1

  echo "Compressing DxeCoreUe.raw..."
  ImageTool GenImage -c UE -o "${BUILD_DIR_ARCH}/DxeCoreUe.raw" "${BUILD_DIR_ARCH}/DxeCore.efi"
  LzmaCompress -e -o "${BUILD_DIR}/FV/DxeMain${arch}.z" \
    "${BUILD_DIR_ARCH}/DxeCoreUe.raw" || exit 1

  echo "Compressing DxeIplUe.raw..."
  ImageTool GenImage -c UE -o "${BUILD_DIR_ARCH}/DxeIplUe.raw" "${BUILD_DIR_ARCH}/DxeIpl.efi"
  LzmaCompress -e -o "${BUILD_DIR}/FV/DxeIpl${arch}.z" \
    "${BUILD_DIR_ARCH}/DxeIplUe.raw" || exit 1

  echo "Generating Loader Image..."
  # Reuse DEBUG EfiLdr in NOOPT build to keep within allotted 0x10000-0x20000 space.
  # With this approach, everything after EfiLdr is fully NOOPT, but EfiLdr starts.
  # TODO: Look at moving EFILDR_BASE_SEGMENT (see also kBoot2Segment, BASE_ADDR_32)
  # to make space for NOOPT loader.
  SAFE_LOADER=$(echo "${BUILD_DIR_ARCH}/EfiLoader.efi" | sed -e 's/NOOPT/DEBUG/')
  ImageTool GenImage -c PE -x -b 0x10000 -o "${BUILD_DIR_ARCH}/EfiLoaderRebased.efi" "${SAFE_LOADER}" || exit 1

  "${FV_TOOLS}/EfiLdrImage" -o "${BUILD_DIR}/FV/Efildr${arch}" \
    "${BUILD_DIR_ARCH}/EfiLoaderRebased.efi" "${BUILD_DIR}/FV/DxeIpl${arch}.z" \
    "${BUILD_DIR}/FV/DxeMain${arch}.z" "${BUILD_DIR}/FV/DUETEFIMAINFV${arch}.z" || exit 1

  "${FV_TOOLS}/EfiLdrImage" -o "${BUILD_DIR}/FV/EfildrBlockIo${arch}" \
    "${BUILD_DIR_ARCH}/EfiLoaderRebased.efi" "${BUILD_DIR}/FV/DxeIpl${arch}.z" \
    "${BUILD_DIR}/FV/DxeMain${arch}.z" "${BUILD_DIR}/FV/DUETEFIMAINFVBLOCKIO${arch}.z" || exit 1

  # Calculate page table location for 64-bit builds.
  # Page table must be 4K aligned, bootsectors are 4K each, and 0x20000 is base address.
  if [ "${arch}" = "X64" ]; then
    if [ "$(uname)" = "Darwin" ]; then
      EL_SIZE=$(stat -f "%z" "${BUILD_DIR}/FV/Efildr${arch}")
      EL_SIZE_BLOCKIO=$(stat -f "%z" "${BUILD_DIR}/FV/EfildrBlockIo${arch}")
    else
      EL_SIZE=$(stat --printf="%s\n" "${BUILD_DIR}/FV/Efildr${arch}")
      EL_SIZE_BLOCKIO=$(stat --printf="%s\n" "${BUILD_DIR}/FV/EfildrBlockIo${arch}")
    fi
    PAGE_TABLE_OFF=$( printf "0x%x" $(( (EL_SIZE + 0x2000 + 0xFFF) & ~0xFFF )) )
    PAGE_TABLE=$( printf "0x%x" $(( PAGE_TABLE_OFF + 0x20000 )) )
    PAGE_TABLE_OFF_BLOCKIO=$( printf "0x%x" $(( (EL_SIZE_BLOCKIO + 0x2000 + 0xFFF) & ~0xFFF )) )
    PAGE_TABLE_BLOCKIO=$( printf "0x%x" $(( PAGE_TABLE_OFF_BLOCKIO + 0x20000 )) )

    export PAGE_TABLE_OFF
    export PAGE_TABLE
    export PAGE_TABLE_OFF_BLOCKIO
    export PAGE_TABLE_BLOCKIO

    BOOTSECTOR_SUFFIX="_${PAGE_TABLE}"
    BOOTSECTOR_SUFFIX_BLOCKIO="_${PAGE_TABLE_BLOCKIO}"
  else
    BOOTSECTOR_SUFFIX=""
    BOOTSECTOR_SUFFIX_BLOCKIO=""
  fi

  # Build bootsectors.
  mkdir -p "${BOOTSECTORS}" || exit 1
  cd "${BOOTSECTORS}"/.. || exit 1
  make "${arch}" || exit 1
  cd - || exit 1

  # Concatenate bootsector into the resulting image.
  cat "${BOOTSECTORS}/Start${arch}${BOOTSECTOR_SUFFIX}.com" "${BOOTSECTORS}/Efi${arch}.com" \
    "${BUILD_DIR}/FV/Efildr${arch}" > "${BUILD_DIR}/FV/Efildr${arch}Pure" || exit 1
  cat "${BOOTSECTORS}/Start${arch}${BOOTSECTOR_SUFFIX_BLOCKIO}.com" "${BOOTSECTORS}/Efi${arch}.com" \
    "${BUILD_DIR}/FV/EfildrBlockIo${arch}" > "${BUILD_DIR}/FV/EfildrBlockIo${arch}Pure" || exit 1

  # Append page table and skip empty data in 64-bit mode.
  if [ "${arch}" = "X64" ]; then
    "${FV_TOOLS}/GenPage" "${BUILD_DIR}/FV/Efildr${arch}Pure" \
      -b "${PAGE_TABLE}" -f "${PAGE_TABLE_OFF}" \
      -o "${BUILD_DIR}/FV/Efildr${arch}Out" || exit 1
    "${FV_TOOLS}/GenPage" "${BUILD_DIR}/FV/EfildrBlockIo${arch}Pure" \
      -b "${PAGE_TABLE_BLOCKIO}" -f "${PAGE_TABLE_OFF_BLOCKIO}" \
      -o "${BUILD_DIR}/FV/EfildrBlockIo${arch}Out" || exit 1

    dd if="${BUILD_DIR}/FV/Efildr${arch}Out" of="${BUILD_DIR_ARCH}/boot" bs=512 skip=1 || exit 1
    dd if="${BUILD_DIR}/FV/EfildrBlockIo${arch}Out" of="${BUILD_DIR_ARCH}/boot-blockio" bs=512 skip=1 || exit 1
  else
    cp "${BUILD_DIR}/FV/Efildr${arch}Pure" "${BUILD_DIR_ARCH}/boot" || exit 1
    cp "${BUILD_DIR}/FV/EfildrBlockIo${arch}Pure" "${BUILD_DIR_ARCH}/boot-blockio" || exit 1
  fi
}

package() {
  if [ ! -d "$1" ]; then
    echo "Missing package directory $1 at $(pwd)"
    exit 1
  fi

  if [ ! -d "$1"/../FV ]; then
    echo "Missing FV directory $1/../FV at $(pwd)"
    exit 1
  fi

  pushd "$1" || exit 1

  # Switch to parent directory.
  pushd .. || exit 1
  BUILD_DIR=$(pwd)

  for arch in "${ARCHS[@]}"; do
    pushd "${arch}" || exit 1
    BUILD_DIR_ARCH=$(pwd)
    imgbuild "${arch}"
    popd || exit 1
  done

  popd || exit 1
  popd || exit 1
}

cd "$(dirname "$0")" || exit 1

BOOTSECTORS="$(pwd)/Legacy/BootSector/bin"
UNAME="$(uname)"
if [ "$(echo "${UNAME}" | grep MINGW)" != "" ] || [ "$(echo "${UNAME}" | grep MSYS)" != "" ]; then
  UNAME="Windows"
fi

FV_TOOLS_BUILDDIR="$(pwd)/Utilities/BaseTools"
FV_TOOLS="$(pwd)/Utilities/BaseTools/bin.${UNAME}"

echo "Compiling BaseTools for your platform..."
if [ "$UNAME" != "Windows" ]; then
  make -C "$FV_TOOLS_BUILDDIR" || exit 1
else
  CC=gcc DIST=Windows make -C "$FV_TOOLS_BUILDDIR" || exit 1
fi

if [ ! -d "${FV_TOOLS}" ]; then
  echo "ERROR: Something went wrong while compiling BaseTools for your platform!"
  exit 1
fi

if [ "${INTREE}" != "" ]; then
  # In-tree compilation is merely for packing.
  cd .. || exit 1

  if [ "${TARGETARCH}" = "" ]; then
    TARGETARCH="X64"
  fi

  if [ "${TARGET}" = "" ]; then
    TARGET="RELEASE"
  fi

  if [ "${TARGETCHAIN}" = "" ]; then
    TARGETCHAIN="XCODE5"
  fi

  build -a "${TARGETARCH}" -b "${TARGET}" -t "${TARGETCHAIN}" -p OpenCorePkg/OpenDuetPkg.dsc || exit 1
  BUILD_DIR="${WORKSPACE}/Build/OpenDuetPkg/${TARGET}_${TARGETCHAIN}"
  BUILD_DIR_ARCH="${BUILD_DIR}/${TARGETARCH}"
  imgbuild "${TARGETARCH}"
else
  if [ "$TARGETS" = "" ]; then
    TARGETS=(DEBUG RELEASE NOOPT)
    export TARGETS
  fi
  if [ "$ARCHS" = "" ]; then
    ARCHS=(X64 IA32)
    export ARCHS
  fi

  DISCARD_SUBMODULES=OpenCorePkg
  SELFPKG_DIR="OpenCorePkg"
  SELFPKG=OpenDuetPkg
  NO_ARCHIVES=1

  export DISCARD_SUBMODULES
  export SELFPKG_DIR
  export SELFPKG
  export NO_ARCHIVES

  src=$(curl -LfsS https://raw.githubusercontent.com/acidanthera/ocbuild/UserSpace/efibuild.sh) && eval "$src" || exit 1
fi
