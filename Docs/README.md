# OpenCore Documentation

This directory contains reference manuals, sample configuration files, and other useful resources.

## Major documents

- **Configuration.pdf** – comprehensive reference manual describing all OpenCore configuration options.
- **Differences/Differences.pdf** – highlights configuration changes between releases.
- **Errata/Errata.pdf** – errata sheet with clarifications and known issues in the manual.
- **Kexts.md** – list of commonly used kernel extensions.
- **Flavours.md** – overview of the OpenCore content flavour naming scheme used for icons.
- **Libraries.md** – brief description of the libraries included in this package.
- **FORUMS.md** – links to community forums for discussion and support.
- **Sample.plist** – minimal sample configuration file.
- **SampleCustom.plist** – alternative sample configuration with additional settings.
- **AcpiSamples/Source/** – collection of example ACPI tables.
- **Logos/** – official project logos.

## Rebuilding documentation

PDF manuals such as `Configuration.pdf`, `Differences.pdf` and `Errata.pdf` can be rebuilt using:

```bash
./Docs/BuildDocs.tool
```

A complete TeX Live installation with `pdflatex` and `latexdiff` is required.
