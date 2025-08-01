"""
Creates postgres db and all the tables
"""

import psycopg2
from psycopg2 import sql
from psycopg2.extensions import ISOLATION_LEVEL_AUTOCOMMIT
from postgres_config import DB_CONFIG

DB_NAME = "hft_db"

CREATE_TABLES_SQL = """
DROP TABLE IF EXISTS orders;
DROP TABLE IF EXISTS tickers;
DROP TABLE IF EXISTS clients;

CREATE TABLE clients (
    client_id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL
);

CREATE TABLE tickers (
    ticker TEXT PRIMARY KEY,
    price INTEGER NOT NULL
);

CREATE TABLE orders (
    client_id BIGINT NOT NULL REFERENCES clients(client_id),
    order_id BIGINT PRIMARY KEY,
    created BIGINT NOT NULL,
    ticker TEXT NOT NULL REFERENCES tickers(ticker),
    quantity INTEGER NOT NULL,
    price INTEGER NOT NULL,
    action INTEGER NOT NULL
);
"""

def create_db():
    try:
        conn = psycopg2.connect(dbname="postgres", user=DB_CONFIG["user"], password=DB_CONFIG["password"], host=DB_CONFIG["host"], port=DB_CONFIG["port"])
        conn.set_isolation_level(ISOLATION_LEVEL_AUTOCOMMIT)

        cur = conn.cursor()
        cur.execute("SELECT 1 FROM pg_database WHERE datname = %s", (DB_NAME,))
        exists = cur.fetchone()

        if not exists:
            cur.execute(sql.SQL("CREATE DATABASE {}").format(sql.Identifier(DB_NAME)))
            print(f"Database '{DB_NAME}' created successfully.")
        else:
            print(f"Database '{DB_NAME}' already exists.")

        cur.close()
        conn.close()
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