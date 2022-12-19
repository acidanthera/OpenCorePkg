FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y lsb-release wget software-properties-common gnupg build-essential nasm uuid-dev libssl-dev iasl curl git zip && \
    { [ "$(dpkg --print-architecture)" == "amd64" ] || { apt-get install -y gcc-x86-64-linux-gnu && export GCC5_BIN=x86_64-linux-gnu- ; } ; } && echo "export GCC5_BIN=$GCC5_BIN" > ~/.edk2_rc.sh && echo ". ~/.edk2_rc.sh" > /etc/profile.d/edk2-gcc5.sh && \
    wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 13 && rm -f llvm.sh && \
    apt-get purge --auto-remove -y wget software-properties-common gnupg && \
    rm -rf /var/lib/apt/lists/*
ENV PATH="$PATH:/usr/lib/llvm-13/bin"
