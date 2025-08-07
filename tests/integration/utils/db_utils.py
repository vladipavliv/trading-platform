import os
import psycopg2
from psycopg2.extras import RealDictCursor

DB_CONFIG = {
    "dbname": "hft_db",
    "user": "postgres",
    "password": "password",
    "host": os.environ.get("POSTGRES_HOST", "localhost"),
    "port": 5432
}

def read_tickers():
    conn = None
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        with conn.cursor(cursor_factory=RealDictCursor) as cur:
            cur.execute("SELECT ticker, price FROM tickers;")
            rows = cur.fetchall()
            return rows
    except Exception as e:
        print(f"Error reading tickers from DB: {e}")
        return []
    finally:
        if conn:
            conn.close()