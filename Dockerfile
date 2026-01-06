FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ARG BUILD_TYPE=Release
ARG CXX=/usr/bin/g++

ARG GITHUB_ACTIONS
ENV GITHUB_ACTIONS=${GITHUB_ACTIONS}

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

ADD https://repo1.maven.org/maven2/uk/co/real-logic/sbe-all/1.34.0/sbe-all-1.34.0.jar /usr/local/bin/sbe-tool.jar
RUN chmod +x /usr/local/bin/sbe-tool.jar

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

RUN find /app -name "__init__.py" -o -name "*.py" | grep hft

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
    libgoogle-glog0v6 libdouble-conversion3 libspdlog1.12 \
    librdkafka1 libpq5 openjdk-17-jre-headless \
    && rm -rf /var/lib/apt/lists/* && ldconfig

ENV PATH="/opt/venv/bin:$PATH"
WORKDIR /app
CMD ["./build/server/hft_server"]
