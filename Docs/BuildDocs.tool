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
latexdiff -s ONLYCHANGEDPAGE PreviousConfiguration.tex ../Configuration.tex \
  > Differences.tex || \
  abort "Unable to differentiate"
latexbuild Differences -interaction=nonstopmode

cd ../Errata || abort "Unable to process annotations"
latexbuild Errata

exit 0
