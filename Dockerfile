FROM ubuntu:24.04 AS builder

ARG GITHUB_ACTIONS
ENV GITHUB_ACTIONS=${GITHUB_ACTIONS}

ENV DEBIAN_FRONTEND=noninteractive
ARG BUILD_TYPE=Release
ARG CXX=/usr/bin/g++-13

RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config \
    g++-13 \
    libboost-all-dev libspdlog-dev librdkafka-dev \
    libdouble-conversion-dev libiberty-dev binutils-dev \
    libgoogle-glog-dev libpq-dev libbenchmark-dev \
    flatbuffers-compiler libflatbuffers-dev \
    openjdk-17-jdk \
    python3 python3-venv python3-pip \
    libevent-dev libgflags-dev libssl-dev zlib1g-dev libunwind-dev \
    && rm -rf /var/lib/apt/lists/*

# fast_float (Required by Folly)
RUN git clone --depth 1 https://github.com/fastfloat/fast_float.git /tmp/fast_float && \
    cmake -S /tmp/fast_float -B /tmp/fast_float/build -DCMAKE_BUILD_TYPE=${BUILD_TYPE} && \
    cmake --build /tmp/fast_float/build --target install && \
    rm -rf /tmp/fast_float

# Folly
RUN git clone --depth 1 --branch v2024.01.01.00 https://github.com/facebook/folly.git /tmp/folly && \
    cmake -S /tmp/folly -B /tmp/folly/build \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBUILD_TESTS=OFF && \
    cmake --build /tmp/folly/build -j$(nproc) --target install && \
    rm -rf /tmp/folly

# libpqxx
RUN git clone --depth 1 --branch 7.9.1 https://github.com/jtv/libpqxx.git /tmp/libpqxx && \
    cmake -S /tmp/libpqxx -B /tmp/libpqxx/build \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBUILD_TEST=OFF -DBUILD_DOC=OFF && \
    cmake --build /tmp/libpqxx/build -j$(nproc) --target install && \
    rm -rf /tmp/libpqxx

ADD https://repo1.maven.org/maven2/uk/co/real-logic/sbe-all/1.34.0/sbe-all-1.34.0.jar /usr/local/bin/sbe-tool.jar
RUN chmod +x /usr/local/bin/sbe-tool.jar

WORKDIR /app

COPY requirements.txt .
RUN python3 -m venv /opt/venv && \
    /opt/venv/bin/pip install --upgrade pip && \
    /opt/venv/bin/pip install -r requirements.txt

COPY . .

RUN cmake -S . -B build \
        -DCMAKE_CXX_COMPILER=${CXX} \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DTARGET_ARCH=x86-64-v3 \
        -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG \
        -DSERIALIZATION=FBS \
        -DCOMM_TYPE_SHM=OFF && \
    cmake --build build -j$(nproc)

FROM ubuntu:24.04

COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /usr/local/bin /usr/local/bin
COPY --from=builder /app/build /app/build
COPY --from=builder /opt/venv /opt/venv
COPY --from=builder /app/build/gen/fbs/python/hft /app/hft

COPY scripts /app/scripts
COPY tests /app/tests

RUN apt-get update && apt-get install -y \
    cmake python3 \
    libgoogle-glog0v6 \
    libdouble-conversion3 \
    libspdlog1.12 \
    librdkafka1 \
    libpq5 \
    openjdk-17-jre-headless \
    libevent-2.1-7 \
    libgflags2.2 \
    libunwind8 \
    libssl3 \
    libboost-program-options1.83.0 \
    libboost-system1.83.0 \
    libboost-thread1.83.0 \
    && rm -rf /var/lib/apt/lists/* && ldconfig

ENV PATH="/opt/venv/bin:$PATH"
WORKDIR /app
CMD ["./build/server/hft_server"]
