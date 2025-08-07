import os 

DB_CONFIG = {
    "dbname": "hft_db",
    "user": "postgres",
    "password": "password",
    "host": os.environ.get("POSTGRES_HOST", "localhost"),
    "port": 5432
}
