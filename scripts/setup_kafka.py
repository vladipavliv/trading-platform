"""
Adjusts kafka configuration for better throughput and creates topics
"""

import os
import subprocess

KAFKA_DIR = "/home/vladimir/src/kafka_2.13-4.0.0"
KAFKA_BIN = f"{KAFKA_DIR}/bin"
KAFKA_PROPERTIES = f"{KAFKA_DIR}/config/producer.properties"

producer_config = {
    "bootstrap.servers": "localhost:9092",
    "acks": "1",
    "batch.size": "1048576",
    "linger.ms": "100",
    "compression.type": "snappy",
    "buffer.memory": "33554432",
    "max.in.flight.requests.per.connection": "1000",
}

topics = ["order-timestamps"]

def update_producer_properties():
    if not os.path.exists(KAFKA_PROPERTIES):
        print(f"Error: {KAFKA_PROPERTIES} does not exist.")
        return

    with open(KAFKA_PROPERTIES, 'r') as file:
        lines = file.readlines()

    updated_lines = []
    existing_keys = set()

    for line in lines:
        if "=" in line:
            key, value = line.split("=", 1)
            key = key.strip()
            value = value.strip()

            if key in producer_config:
                value = producer_config[key]
                line = f"{key}={value}\n"
            existing_keys.add(key)

        updated_lines.append(line)

    for key, value in producer_config.items():
        if key not in existing_keys:
            updated_lines.append(f"{key}={value}\n")

    with open(KAFKA_PROPERTIES, 'w') as file:
        file.writelines(updated_lines)

    print(f"Producer properties updated successfully in {KAFKA_PROPERTIES}")

def create_kafka_topics():
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
    update_producer_properties()
    create_kafka_topics()
