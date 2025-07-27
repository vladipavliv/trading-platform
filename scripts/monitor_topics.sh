~/src/kafka_2.13-4.0.0/bin/kafka-console-consumer.sh \
  --bootstrap-server localhost:9092 \
  --include '^(server-commands|client-commands|runtime-metrics|order-timestamps)$' \
  --property print.topic=true
