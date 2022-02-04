#!/bin/bash

abort() {
  echo "ERROR: $1!"
  exit 1
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

cd "$(dirname "$0")" || abort "Wrong directory"

if [ "$(which latexdiff)" = "" ]; then
  abort "latexdiff is missing, check your TeX Live installation"
fi

if [ "$(which pdflatex)" = "" ]; then
  abort "pdflatex is missing, check your TeX Live installation"
fi

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

if [ -f "Configuration.md5" ]; then
  OLDHASH=$(cat "Configuration.md5")
else
  OLDHASH=""
fi

echo "$HASH" > "Configuration.md5"
if [ "$HASH" != "$OLDHASH" ]; then
  echo "Configuration hash ${HASH} is different from ${OLDHASH}."
  echo "You forgot to rebuild documentation (Configuration.pdf)!"
  echo "Please run ./Docs/BuildDocs.tool."
  exit 1
fi

exit 0
