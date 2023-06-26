#!/bin/bash

LIBRESSL_VERSION="3.5.3"
LIBRESSL_NAME="libressl-${LIBRESSL_VERSION}"
LIBRESSL_ARCHIVE="${LIBRESSL_NAME}.tar.gz"
LIBRESSL_URL="https://cdn.openbsd.org/pub/OpenBSD/LibreSSL/${LIBRESSL_ARCHIVE}"

SRC_DIR=$(dirname "$0")
cd "$SRC_DIR" || exit 1

OUTPUT_PATH="$(pwd)/libressl"
BUILD_DIR="$(pwd)/tmp/${LIBRESSL_NAME}/build"

export CFLAGS="-mmacosx-version-min=10.6 -Wno-unguarded-availability-new"
export LDFLAGS="-mmacosx-version-min=10.6"

abort() {
  echo "ERROR: $1!"
  exit 1
}

unamer() {
  NAME="$(uname)"

  if [ "$(echo "${NAME}" | grep MINGW)" != "" ] || [ "$(echo "${NAME}" | grep MSYS)" != "" ]; then
    echo "Windows"
  else
    echo "${NAME}"
  fi
}

if [ "$(unamer)" != "Darwin" ]; then
  abort "This script is only for Darwin"
fi

# Avoid conflicts with PATH overrides.
ARCH="/usr/bin/arch"
CURL="/usr/bin/curl"
FIND="/usr/bin/find"
MKDIR="/bin/mkdir"
RM="/bin/rm"
SED="/usr/bin/sed"
TAR="/usr/bin/tar"

TOOLS=(
  "${ARCH}"
  "${CURL}"
  "${FIND}"
  "${MKDIR}"
  "${RM}"
  "${SED}"
  "${TAR}"
)
for tool in "${TOOLS[@]}"; do
  if [ ! -x "${tool}" ]; then
    abort "Missing ${tool}"
  fi
done

# shellcheck disable=SC2015
"${RM}" -rf "tmp" && "${MKDIR}" "tmp" || abort "Failed to create temporary directory with code $?"

pushd "tmp" > /dev/null || abort "Failed to cd to temporary directory tmp with code $?"

echo "Downloading LibreSSL ${LIBRESSL_VERSION}..."
"${CURL}" -LfsS -o "${LIBRESSL_ARCHIVE}" "${LIBRESSL_URL}" || abort "Failed to download LibreSSL ${LIBRESSL_VERSION} from ${LIBRESSL_URL} with code $?"

"${TAR}" -xzf "${LIBRESSL_ARCHIVE}" || abort "Failed to extract LibreSSL with code $?"

cd "${LIBRESSL_NAME}" || abort "Failed to cd to ${LIBRESSL_NAME} with code $?"

if [ "$(${ARCH})" = "arm64" ]; then
  # If we are building on arm64 (Apple Silicon), these extra options are required to ensure x86_64 builds.
  EXTRA_OPTS=(--host=arm-apple-darwin --build=x86_64-apple-darwin)
  CFLAGS+=" --target=x86_64-apple-darwin"
  LDFLAGS+=" --target=x86_64-apple-darwin"
else
  EXTRA_OPTS=()
fi

# Monkeypatch to disable strtonum for <11.0 support
"${SED}" -i '' -E 's/strsep strtonum/strsep/g' configure || abort "Failed to monkeypatch strtonum in LibreSSL with code $?"

./configure --disable-dependency-tracking --disable-tests --disable-shared --prefix="${BUILD_DIR}" "${EXTRA_OPTS[@]}" || abort "Failed to configure LibreSSL with code $?"

make -j "$(getconf _NPROCESSORS_ONLN)" || abort "Failed to build LibreSSL with code $?"

make install || abort "Failed to copy LibreSSL build files with code $?"

# shellcheck disable=SC2015
rm -rf "${OUTPUT_PATH}" && mkdir "${OUTPUT_PATH}" || abort "Failed to create output directory ${OUTPUT_PATH} with code $?"

cp -r "${BUILD_DIR}/include" "${BUILD_DIR}/lib" "${OUTPUT_PATH}/" || abort "Failed to copy LibreSSL libraries to output directory with code $?"

popd > /dev/null || abort "Failed to cd to root with code $?"

${RM} -rf "tmp"

echo "Successfully built LibreSSL ${LIBRESSL_VERSION}"
exit 0
