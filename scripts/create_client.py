"""
Creates a new client
TODO(self) encrypt password
"""

import psycopg2
import sys
from postgres_config import DB_CONFIG

def create_CLIENT(name, password):
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        cursor = conn.cursor()

        # Insert the new client
        insert_query = """
        INSERT INTO clients (name, password) 
        VALUES (%s, %s)
        ON CONFLICT (name) DO NOTHING;
        """
        cursor.execute(insert_query, (name, password))
        
        # Commit the transaction
        conn.commit()
        
        print(f"Client '{name}' inserted successfully.")

    except Exception as e:
        print(f"Error: {e}")
    
    finally:
        cursor.close()
        conn.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 create_client.py <name> <password>")
        sys.exit(1)

    name = sys.argv[1]
    password = sys.argv[2]

    create_CLIENT(name, password)
