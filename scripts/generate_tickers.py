import random
import string
import psycopg2

DB_CONFIG = {
    "dbname": "hft_db",
    "user": "postgres",
    "password": "password",
    "host": "localhost",
    "port": 5432
}

def generate_ticker():
    return "".join(random.choices(string.ascii_uppercase, k=4))

def generate_price():
    return random.randint(10, 10000)

NUM_TICKERS = 1000

def insert_tickers():
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        cursor = conn.cursor()
        
        for _ in range(NUM_TICKERS):
            ticker = generate_ticker()
            price = generate_price()
            cursor.execute("INSERT INTO tickers (ticker, price) VALUES (%s, %s) ON CONFLICT DO NOTHING", (ticker, price))
        
        conn.commit()
        print(f"Inserted {NUM_TICKERS} random tickers.")
    except Exception as e:
        print("Error:", e)
    finally:
        cursor.close()
        conn.close()

if __name__ == "__main__":
    insert_tickers()