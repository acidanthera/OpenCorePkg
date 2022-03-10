#!/bin/bash

abort() {
  echo "ERROR: $1!"
  exit 1
}

prompt() {
  echo "$1"
  read -rp "Enter [Y]es to continue: " v
  if [ "$v" != "Y" ] && [ "$v" != "y" ]; then
    return 0
  fi

  return 1
}

latexbuild() {
  # Perform file cleanup.
  rm -f ./*.aux ./*.log ./*.out ./*.pdf ./*.toc

  # Perform a first pass
  pdflatex -draftmode "$1" "$2" || \
    abort "Unable to create $1 draft"

  # Perform a number of TOC passes.
  while grep 'Rerun to get ' "${1}.log" ; do
    pdflatex -draftmode "$1" "$2" || \
      abort "Unable to create $1 draft with TOC"
  done

  # Create a real PDF.
  pdflatex "$1" "$2" || \
    abort "Unable to create $1 PDF"

  # Perform a number of TOC passes for PDF (usually not needed).
  while grep 'Rerun to get ' "${1}.log" ; do
    pdflatex -draftmode "$1" "$2" || \
      abort "Unable to create $1 PDF with TOC"
  done
}

builddocs() {
  latexbuild Configuration

  cd Differences || abort "Unable to process annotations"
  rm -f ./*.aux ./*.log ./*.out ./*.pdf ./*.toc
  latexdiff --allow-spaces -s ONLYCHANGEDPAGE PreviousConfiguration.tex ../Configuration.tex \
    > Differences.tex || \
    abort "Unable to differentiate"
  latexbuild Differences -interaction=nonstopmode

  cd ../Errata || abort "Unable to process annotations"
  latexbuild Errata

  cd .. || abort "Unable to cd back to Docs directory"

  err=0
  if [ "$(which md5)" != "" ]; then
    HASH=$(md5 Configuration.tex | cut -f4 -d' ')
    err=$?
  elif [ "$(which openssl)" != "" ]; then
    HASH=$(openssl md5 Configuration.tex | cut -f2 -d' ')
    err=$?
  else
    abort "No md5 hasher found!"
  fi

  if [ $err -ne 0 ]; then
    abort "Failed to calculate built configuration hash!"
  fi

  OLDHASH=""
  if [ -f "Configuration.md5" ]; then
    OLDHASH=$(cat "Configuration.md5")
  fi

  echo "$HASH" > "Configuration.md5"
  if [ "$HASH" != "$OLDHASH" ]; then
    echo "Configuration hash ${HASH} is different from ${OLDHASH}."
    echo "You forgot to rebuild documentation (Configuration.pdf)!"
    echo "Please run ./Docs/BuildDocs.tool."
    exit 1
  fi
}

checkver() {
  ocver=$(grep OPEN_CORE_VERSION ../Include/Acidanthera/Library/OcMainLib.h | sed 's/.*"\(.*\)".*/\1/' | grep -E '^[0-9.]+$')
  if [ "$ocver" = "" ]; then
    abort "Invalid OpenCore version"
  fi

  docver=$(grep -w 'Reference Manual' ./Configuration.tex | sed -e 's/(//g' -e 's/)//g' | awk '{print $3}')
  if [ "$docver" = "" ]; then
    abort "Invalid document version"
  fi

  if [ "$ocver" = "$docver" ]; then
    return 0
  fi

  return 1
}

bumpversion() {
  echo "Bumping version from $docver to $ocver"
  cd Differences || abort "Unable to enter Differences directory"
  rm -f PreviousConfiguration.tex
  cp ../Configuration.tex PreviousConfiguration.tex || abort "Failed to copy PreviousConfiguration.tex"
  cd .. || abort "Unable to enter parent directory"
  perl -pi -e "s/Reference Manual \($docver\)/Reference Manual \($ocver\)/g" ./Configuration.tex || abort "Failed to patch Configuration.tex"
}

main() {
  if [ "$(which latexdiff)" = "" ]; then
    abort "latexdiff is missing, check your TeX Live installation"
  fi

  if [ "$(which pdflatex)" = "" ]; then
    abort "pdflatex is missing, check your TeX Live installation"
  fi

  cd "$(dirname "$0")" || abort "Wrong directory"

  checkver
  if [ $? = 1 ]; then
    echo "Current Configuration.tex/pdf version: $docver"
    echo "Current OC Header version: $ocver"
    prompt "Bump version to $ocver?"
    if [ $? = 0 ]; then
      prompt "Still build docs?"
      if [ $? = 0 ]; then
        exit 1
      fi
    else
      bumpversion
    fi
  fi

  builddocs
}

main "$@"

exit 0
