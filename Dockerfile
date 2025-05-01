FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

# Main dependencies
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libspdlog-dev \
    librdkafka-dev \
    libdouble-conversion-dev \
    binutils-dev \
    libgoogle-glog-dev \
    libpqxx-dev \
    flatbuffers-compiler \
    libflatbuffers-dev \
    protobuf-compiler \
    && rm -rf /var/lib/apt/lists/*

ARG GITHUB_TOKEN

# fast_float needed by folly
RUN git clone https://x-access-token:${GITHUB_TOKEN}@github.com/fastfloat/fast_float.git /fast_float && \
    cd /fast_float && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install && \
    rm -rf /fast_float

# folly
RUN git clone --branch main --single-branch https://x-access-token:${GITHUB_TOKEN}@github.com/facebook/folly.git /folly && \
    cd /folly && \
    git submodule update --init --recursive && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install && \
    rm -rf /folly

WORKDIR /app

COPY . /app

RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Run tests with CTest
CMD ["ctest", "--output-on-failure"]
