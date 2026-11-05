FROM nvidia/cuda:12.3.2-cudnn9-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install base prerequisites
RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    make \
    lsb-release \
    wget \
    software-properties-common \
    gnupg \
    zip \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Install LLVM 21
RUN wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh 21 all \
    && rm llvm.sh

# Install compiler tools, libraries, and utilities
RUN apt-get update && apt-get install -y --no-install-recommends \
    llvm \
    clang \
    zlib1g-dev \
    libzstd-dev \
    libeigen3-dev \
    libopencv-dev \
    fish \
    vim \
    && rm -rf /var/lib/apt/lists/*


# Configure fish shell globally (using native fish syntax)
RUN mkdir -p /etc/fish \
    && echo 'set -gx PATH /nsk/bin $PATH' >> /etc/fish/config.fish \
    && echo 'set -gx NEVE_LIBS /nsk/lib' >> /etc/fish/config.fish


# Ensure paths are also set for bash/Docker runtime so 'nsm' works in subsequent RUN steps
ENV PATH="/nsk/bin:${PATH}"
ENV NEVE_LIBS="/nsk/lib"


# Make bash automatically execute fish
RUN echo 'exec fish' >> /root/.bashrc


WORKDIR /root

CMD ["bash"]
