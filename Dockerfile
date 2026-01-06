FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ARG BUILD_TYPE=Release
ARG CXX=/usr/bin/g++

RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config \
    libboost-all-dev libspdlog-dev librdkafka-dev \
    libdouble-conversion-dev libiberty-dev binutils-dev \
    libgoogle-glog-dev libpq-dev libbenchmark-dev \
    flatbuffers-compiler libflatbuffers-dev \
    protobuf-compiler openjdk-17-jdk \
    python3 python3-venv python3-pip \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 https://github.com/fastfloat/fast_float.git /tmp/fast_float && \
    cmake -S /tmp/fast_float -B /tmp/fast_float/build -DCMAKE_BUILD_TYPE=${BUILD_TYPE} && \
    cmake --build /tmp/fast_float/build --target install && \
    rm -rf /tmp/fast_float

RUN git clone --depth 1 --branch 7.9.1 https://github.com/jtv/libpqxx.git /tmp/libpqxx && \
    cmake -S /tmp/libpqxx -B /tmp/libpqxx/build \
    -DCMAKE_CXX_STANDARD=23 \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBUILD_TEST=OFF -DBUILD_DOC=OFF && \
    cmake --build /tmp/libpqxx/build -j$(nproc) --target install && \
    rm -rf /tmp/libpqxx

RUN git clone --depth 1 https://github.com/real-logic/simple-binary-encoding.git /tmp/sbe && \
    cd /tmp/sbe && ./gradlew jar && \
    mv ./sbe-all/build/libs/sbe-all-*-SNAPSHOT.jar /usr/local/bin/sbe-tool.jar && \
    rm -rf /tmp/sbe

WORKDIR /app

COPY requirements.txt .
RUN python3 -m venv /opt/venv && \
    /opt/venv/bin/pip install --upgrade pip && \
    /opt/venv/bin/pip install -r requirements.txt

COPY . .

RUN rm -rf build && \
    cmake -S . -B build \
        -DCMAKE_CXX_COMPILER=${CXX} \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DSERIALIZATION=SBE \
        -DTELEMETRY_ENABLED=ON && \
    cmake --build build -j$(nproc)

FROM ubuntu:24.04

COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /usr/local/bin /usr/local/bin
COPY --from=builder /app/build /app/build
COPY --from=builder /opt/venv /opt/venv

RUN apt-get update && apt-get install -y \
    libgoogle-glog0v6 libdouble-conversion3 libspdlog1.12 \
    librdkafka1 libpq5 openjdk-17-jre-headless \
    && rm -rf /var/lib/apt/lists/* && ldconfig

ENV PATH="/opt/venv/bin:$PATH"
WORKDIR /app
CMD ["./build/server/hft_server"]
