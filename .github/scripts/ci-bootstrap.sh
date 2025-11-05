#!/bin/bash

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

if [ "$(unamer)" = "Darwin" ]; then
  XCODE_DIR="/Applications/Xcode_VERSION.app/Contents/Developer"

  # In GitHub Actions:
  # env:
  #  PROJECT_TYPE: "UEFI"

  XCODE_VERSION="16.4.0"

  case "${PROJECT_TYPE}" in 
    UEFI)
      BUILD_DEVELOPER_DIR="${XCODE_DIR/VERSION/${XCODE_VERSION}}"
      ANALYZE_DEVELOPER_DIR="${XCODE_DIR/VERSION/${XCODE_VERSION}}"
      COVERITY_DEVELOPER_DIR="${XCODE_DIR/VERSION/${XCODE_VERSION}}"
      ;;
    KEXT | TOOL)
      BUILD_DEVELOPER_DIR="${XCODE_DIR/VERSION/${XCODE_VERSION}}"
      ANALYZE_DEVELOPER_DIR="${XCODE_DIR/VERSION/${XCODE_VERSION}}"
      COVERITY_DEVELOPER_DIR="${XCODE_DIR/VERSION/${XCODE_VERSION}}"
      ;;
    *)
      abort "Invalid project type"
      ;;
  esac

  SELECTED_DEVELOPER_DIR="${JOB_TYPE}_DEVELOPER_DIR"

  export BUILD_DEVELOPER_DIR
  export ANALYZE_DEVELOPER_DIR
  export COVERITY_DEVELOPER_DIR
  export SELECTED_DEVELOPER_DIR

  if [ -z "
${!SELECTED_DEVELOPER_DIR}" ]; then
    abort "Invalid or missing job type"
  fi

  echo "DEVELOPER_DIR=${!SELECTED_DEVELOPER_DIR}" >> "$GITHUB_ENV"

  # Since GITHUB_ENV doesn't affect the current step, need to export DEVELOPER_DIR for subsequent commands.
  export DEVELOPER_DIR="${!SELECTED_DEVELOPER_DIR}"

  if [ -n "${ACID32}" ]; then
    echo "OVERRIDE_PYTHON3=${DEVELOPER_DIR}/usr/bin/python3" >> "$GITHUB_ENV"
    export OVERRIDE_PYTHON3="${DEVELOPER_DIR}/usr/bin/python3"
    src=$(curl -Lfs https://raw.githubusercontent.com/acidanthera/ocbuild/master/clang32-bootstrap.sh) && eval "$src" || exit 1
  fi
fi

colored_text() {
  echo -e "\033[0;36m${1}\033[0m"
}

# Print runner details
colored_text "OS version"
if [ "$(unamer)" = "Darwin" ]; then
  sw_vers
elif [ "$(unamer)" = "Windows" ]; then
  wmic os get caption, version
else
  lsb_release -a
fi

colored_text "git version"
git --version

colored_text "bash version"
bash --version

colored_text "curl version"
curl --version

if [ "$(unamer)" = "Darwin" ]; then
  colored_text "clang version"
  clang --version

  if [ -n "${ACID32}" ]; then
    colored_text "clang32 version"
    ./clang32/clang-12 --version
  fi 

  colored_text "Xcode version"
  xcode-select --print-path
else
  colored_text "gcc version"
  gcc --version
fi

if [ "$(unamer)" = "Windows" ]; then
  colored_text "VS latest version"
  echo "$(vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productSemanticVersion)"
fi
