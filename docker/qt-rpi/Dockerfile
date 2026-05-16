FROM debian:trixie

ARG DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    file \
    g++-aarch64-linux-gnu \
    gcc-aarch64-linux-gnu \
    git \
    libclang-dev \
    libgl1-mesa-dev \
    libssl-dev \
    ninja-build \
    openssh-client \
    patch \
    patchelf \
    perl \
    pkg-config \
    python3 \
    rsync \
    tar \
    xz-utils \
    zstd \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work
