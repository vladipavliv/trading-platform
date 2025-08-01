FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

# Main dependencies
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libboost-all-dev \
    libspdlog-dev \
    librdkafka-dev \
    libdouble-conversion-dev \
    libiberty-dev \
    binutils-dev \
    libgoogle-glog-dev \
    libpq-dev \ 
    flatbuffers-compiler \
    libflatbuffers-dev \
    protobuf-compiler \
    && rm -rf /var/lib/apt/lists/*

ARG BUILD_TYPE=Release
ENV BUILD_TYPE=${BUILD_TYPE}

# fast_float needed by folly
RUN git clone --branch main --single-branch https://github.com/fastfloat/fast_float.git /fast_float && \
    cd /fast_float && \
    mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf /fast_float

# folly
RUN git clone --branch main --single-branch https://github.com/facebook/folly.git /folly && \
    cd /folly && \
    git submodule update --init --recursive && \
    mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc) && \
    make install && \
    rm -rf /folly

# Build libpqxx from source with C++23 support
RUN git clone --branch master --single-branch https://github.com/jtv/libpqxx.git /libpqxx && \
    cd /libpqxx && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    cd / && rm -rf /libpqxx

WORKDIR /app

COPY . /app

RUN rm -rf build && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. && \
    make -j$(nproc)

# Run tests with CTest
CMD ["ctest", "--output-on-failure"]
