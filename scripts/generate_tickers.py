"""
Generates a bunch of random tickers with prices
"""

import random
import string
import psycopg2
import sys
from postgres_config import DB_CONFIG

def generate_ticker():
    return "".join(random.choices(string.ascii_uppercase, k=4))

def generate_price():
    return random.randint(10, 7500)

def generate_tickers(amount):
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        cursor = conn.cursor()
        
        cursor.execute("DELETE FROM tickers")

        for _ in range(amount):
            ticker = generate_ticker()
            price = generate_price()
            cursor.execute("INSERT INTO tickers (ticker, price) VALUES (%s, %s) ON CONFLICT DO NOTHING", (ticker, price))
        
        conn.commit()
        print(f"Inserted {amount} random tickers.")
    except Exception as e:
        print("Error:", e)
    finally:
        cursor.close()
        conn.close()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 generate_tickers.py 42")
        sys.exit(1)

    try:
        amount = int(sys.argv[1])
        generate_tickers(amount)
    except ValueError:
        print("Error: Amount should be an integer.")
        sys.exit(1)