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

RUN apt-get update && apt-get install -y python3 python3-venv python3-pip \
    && rm -rf /var/lib/apt/lists/*

# OpenJDK (needed for SBE)
RUN apt-get update && apt-get install -y openjdk-17-jdk && rm -rf /var/lib/apt/lists/*

# SBE
RUN git clone --branch master --single-branch https://github.com/real-logic/simple-binary-encoding.git /sbe && \
    cd /sbe && ./gradlew jar && \
    cp ./sbe-all/build/libs/sbe-all-*-SNAPSHOT.jar /usr/local/bin/sbe-tool.jar && \
    rm -rf /sbe

WORKDIR /app

COPY requirements.txt /app/

RUN python3 -m venv venv && \
    . venv/bin/activate && \
    pip install --upgrade pip && \
    pip install -r requirements.txt

COPY . /app

RUN rm -rf build && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_BENCHMARKS=ON -DBUILD_TESTS=ON .. && \
    cmake --build . -- -j$(nproc)

CMD ["ctest", "--output-on-failure"]
