#!/bin/bash

abort() {
  echo "ERROR: $1!"
  exit 1
}

cd "$(dirname "$0")" || abort "Wrong directory"

if [ "$(which latexdiff)" = "" ]; then
  abort "latexdiff is missing, check your TeX Live installation"
fi

if [ "$(which pdflatex)" = "" ]; then
  abort "pdflatex is missing, check your TeX Live installation"
fi

rm -f *.aux *.log *.out *.pdf *.toc

pdflatex -draftmode Configuration.tex || \
  abort "Unable to create configuration pdf"
pdflatex -draftmode Configuration.tex || \
  abort "Unable to create configuration pdf with TOC"
pdflatex Configuration.tex || \
  abort "Unable to create configuration pdf with TOC"

cd Differences || abort "Unable to process annotations"

rm -f *.aux *.log *.out *.pdf *.toc

latexdiff -s ONLYCHANGEDPAGE PreviousConfiguration.tex ../Configuration.tex \
  > Differences.tex || \
  abort "Unable to differentiate"

pdflatex -draftmode -interaction=nonstopmode Differences
pdflatex -draftmode -interaction=nonstopmode Differences
pdflatex -interaction=nonstopmode Differences
exit 0
