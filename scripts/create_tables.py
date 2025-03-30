"""
Creates postgres db and all the tables
"""

import psycopg2

DB_CONFIG = {
    "dbname": "hft_db",
    "user": "postgres",
    "password": "password",
    "host": "localhost",
    "port": 5432
}

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

def create_tables():
    try:
        # Connect to PostgreSQL
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()

        # Execute SQL
        cur.execute(CREATE_TABLES_SQL)

        # Commit and close
        conn.commit()
        cur.close()
        conn.close()

        print("Tables created successfully")
    except Exception as e:
        print("Error:", e)

if __name__ == "__main__":
    create_tables()