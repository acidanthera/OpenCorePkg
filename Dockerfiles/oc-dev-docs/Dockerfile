ARG OC_DEV_DOCS_BASE=ubuntu:22.04
FROM $OC_DEV_DOCS_BASE

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y make doxygen texlive texlive-latex-extra && \
    rm -rf /var/lib/apt/lists/*
