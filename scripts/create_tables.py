"""
Creates postgres db and all the tables
"""

import psycopg2
from postgres_config import DB_CONFIG

CREATE_DB_SQL = "CREATE DATABASE IF NOT EXISTS hft_db;"

CREATE_TABLES_SQL = """
CREATE TABLE IF NOT EXISTS traders (
    trader_id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS tickers (
    ticker TEXT PRIMARY KEY,
    price INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS orders (
    order_id BIGINT PRIMARY KEY,
    trader_id BIGINT NOT NULL REFERENCES traders(trader_id),
    ticker TEXT NOT NULL REFERENCES tickers(ticker),
    quantity INTEGER NOT NULL,
    price INTEGER NOT NULL,
    action TEXT NOT NULL
);
"""

def create_db():
    try:
        conn = psycopg2.connect(dbname="postgres", user=DB_CONFIG["user"], password=DB_CONFIG["password"], host=DB_CONFIG["host"], port=DB_CONFIG["port"])
        conn.autocommit = True  # Set autocommit for DB creation
        cur = conn.cursor()

        cur.execute(CREATE_DB_SQL)

        conn.close()

        print("Database 'hft_db' created successfully.")
    except Exception as e:
        print("Error:", e)

def create_tables():
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()

        cur.execute(CREATE_TABLES_SQL)

        conn.commit()
        cur.close()
        conn.close()

        print("Tables created successfully")
    except Exception as e:
        print("Error:", e)

if __name__ == "__main__":
    create_db()
    create_tables()