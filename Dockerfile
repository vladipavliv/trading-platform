FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive
ARG BUILD_TYPE=Release
ENV BUILD_TYPE=${BUILD_TYPE}

# Main dependencies
RUN apt-get update && apt-get install -y \
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
    libbenchmark-dev \
    flatbuffers-compiler \
    libflatbuffers-dev \
    protobuf-compiler \
    && rm -rf /var/lib/apt/lists/*

# fast_float needed by folly
RUN git clone --branch main --single-branch https://github.com/fastfloat/fast_float.git /fast_float && \
    mkdir -p /fast_float/build && cd /fast_float/build && \
    cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} && \
    cmake --build . -- -j$(nproc) && \
    cmake --install . && \
    rm -rf /fast_float

# folly
RUN git clone --branch main --single-branch https://github.com/facebook/folly.git /folly && \
    cd /folly && git submodule update --init --recursive && \
    mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE} && \
    cmake --build . -- -j$(nproc) && \
    cmake --install . && \
    rm -rf /folly

# libpqxx
RUN git clone --branch master --single-branch https://github.com/jtv/libpqxx.git /libpqxx && \
    mkdir -p /libpqxx/build && cd /libpqxx/build && \
    cmake .. -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=${BUILD_TYPE} && \
    cmake --build . -- -j$(nproc) && \
    cmake --install . && ldconfig && \
    rm -rf /libpqxx

WORKDIR /app

COPY . /app

RUN rm -rf build && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_BENCHMARKS=ON -DBUILD_TESTS=ON .. && \
    cmake --build . -- -j$(nproc)

CMD ["ctest", "--output-on-failure"]
