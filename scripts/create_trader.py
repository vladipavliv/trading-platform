"""
TODO(self) encrypt password
"""

import psycopg2
import sys

DB_CONFIG = {
    "dbname": "hft_db",
    "user": "postgres",
    "password": "password",
    "host": "localhost",
    "port": 5432
}

def create_trader(name, password):
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        cursor = conn.cursor()

        # Insert the new trader
        insert_query = """
        INSERT INTO traders (name, password) 
        VALUES (%s, %s)
        ON CONFLICT (name) DO NOTHING;
        """
        cursor.execute(insert_query, (name, password))
        
        # Commit the transaction
        conn.commit()
        
        print(f"Trader '{name}' inserted successfully.")

    except Exception as e:
        print(f"Error: {e}")
    
    finally:
        cursor.close()
        conn.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 create_trader.py <name> <password>")
        sys.exit(1)

    trader_name = sys.argv[1]
    trader_password = sys.argv[2]

    create_trader(trader_name, trader_password)
