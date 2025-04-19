#!/bin/bash

KAFKA_BROKER="localhost:9092"
KAFKA_TOPIC="order-timestamps"
KAFKA_GROUP="clickhouse-consumer"
PROTOBUF_SCHEMA="metadata_messages.proto:OrderTimestamp"
CLICKHOUSE_HOST="localhost"
CLICKHOUSE_PORT="9000"
CLICKHOUSE_USER="default"
CLICKHOUSE_PASSWORD="password"

# Create Kafka table in ClickHouse
cat <<EOF | clickhouse-client --host=$CLICKHOUSE_HOST --port=$CLICKHOUSE_PORT --user=$CLICKHOUSE_USER --password=$CLICKHOUSE_PASSWORD
CREATE TABLE IF NOT EXISTS order_timestamps_kafka
(
    order_id UInt64,
    created UInt64,
    fulfilled UInt64,
    notified UInt64
)
ENGINE = Kafka
SETTINGS 
    kafka_broker_list = '$KAFKA_BROKER', 
    kafka_topic_list = '$KAFKA_TOPIC', 
    kafka_group_name = '$KAFKA_GROUP',
    kafka_format = 'Protobuf',
    kafka_num_consumers = 1,
    kafka_max_block_size = 1048576,
    format_schema = '$PROTOBUF_SCHEMA',
    kafka_skip_broken_messages = 1,
    kafka_flush_interval_ms = 500,
    kafka_poll_timeout_ms = 1000,
    kafka_thread_per_consumer = 1;
EOF

# Create destination table to store the processed data
cat <<EOF | clickhouse-client --host=$CLICKHOUSE_HOST --port=$CLICKHOUSE_PORT --user=$CLICKHOUSE_USER --password=$CLICKHOUSE_PASSWORD
CREATE TABLE IF NOT EXISTS order_timestamps
(
    order_id UInt64,
    created UInt64,
    fulfilled UInt64,
    notified UInt64
)
ENGINE = MergeTree()
ORDER BY order_id;
EOF

# Create materialized view to process data from Kafka table and insert it into the destination table
cat <<EOF | clickhouse-client --host=$CLICKHOUSE_HOST --port=$CLICKHOUSE_PORT --user=$CLICKHOUSE_USER --password=$CLICKHOUSE_PASSWORD
CREATE MATERIALIZED VIEW IF NOT EXISTS mv_order_timestamps
TO order_timestamps
AS
SELECT 
    order_id, 
    created, 
    fulfilled,
    notified
FROM order_timestamps_kafka;
EOF

# Inform the user that the setup is complete
echo "ClickHouse Kafka integration and table setup is complete!"
