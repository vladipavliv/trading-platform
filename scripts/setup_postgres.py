"""
Sets up Postgres by creading db, tables, creating several clients, and generating some tickers
"""

from create_tables import create_db, create_tables
from create_client import create_client
from generate_tickers import generate_tickers

if __name__ == "__main__":
    create_db()
    create_tables()

    create_client("client0", "password0")
    create_client("client1", "password1")

    generate_tickers(1000)
