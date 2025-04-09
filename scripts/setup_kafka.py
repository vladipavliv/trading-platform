"""
Adjusts kafka configuration for better throughput
Creates topics
"""

import os
import subprocess

KAFKA_DIR = "/home/vladimir/src/kafka_2.13-4.0.0"
KAFKA_BIN = f"{KAFKA_DIR}/bin"
PRODUCER_PROPERTIES = f"{KAFKA_DIR}/config/producer.properties"
BROKER_PROPERTIES = f"{KAFKA_DIR}/config/server.properties"

producer_config = {
    "bootstrap.servers": "localhost:9092",
    "acks": "1",
    "batch.size": "1048576",
    "linger.ms": "100",
    "compression.type": "snappy",
    "buffer.memory": "33554432",
    "max.in.flight.requests.per.connection": "1000",
}

broker_config = {
    "num.network.threads": "8",
    "num.io.threads": "16",
    "socket.send.buffer.bytes": "102400",
    "socket.receive.buffer.bytes": "102400",
    "socket.request.max.bytes": "104857600",
    "log.retention.hours": "168",
    "log.segment.bytes": "1073741824",
    "log.retention.check.interval.ms": "300000",
}

topics = ["order-timestamps", "server-commands"]

def update_properties(path, config_dict, label):
    if not os.path.exists(path):
        print(f"Error: {path} does not exist.")
        return

    with open(path, 'r') as file:
        lines = file.readlines()

    updated_lines = []
    existing_keys = set()

    for line in lines:
        stripped = line.strip()
        if "=" in stripped and not stripped.startswith("#"):
            key, value = map(str.strip, stripped.split("=", 1))

            if key in config_dict:
                line = f"{key}={config_dict[key]}\n"
            existing_keys.add(key)

        updated_lines.append(line)

    for key, value in config_dict.items():
        if key not in existing_keys:
            updated_lines.append(f"{key}={value}\n")

    with open(path, 'w') as file:
        file.writelines(updated_lines)

    print(f"{label} properties updated successfully in {path}")

def create_topics():
    for topic in topics:
        print(f"Creating topic: {topic}")
        command = [
            f"{KAFKA_BIN}/kafka-topics.sh",
            "--create",
            "--bootstrap-server", "localhost:9092",
            "--replication-factor", "1",
            "--partitions", "1",
            "--topic", topic,
            "--if-not-exists"
        ]
        try:
            subprocess.run(command, check=True)
            print(f"Topic '{topic}' created successfully.")
        except subprocess.CalledProcessError as e:
            print(f"Error creating topic '{topic}': {e}")

if __name__ == "__main__":
    update_properties(PRODUCER_PROPERTIES, producer_config, "Producer")
    update_properties(BROKER_PROPERTIES, broker_config, "Broker")
    create_topics()
