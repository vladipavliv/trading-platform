import os
import psycopg2
import json
from pathlib import Path
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


DATA_PATH = os.environ.get("TICKERS_FILE", "data.json")

def read_tickers_json(file_path: str = None) -> list:
    path = DATA_PATH
    
    try:
        # Проверяем, что файл существует
        if not Path(path).exists():
            print(f"Error: File not found: {path}")
            return []
        
        with open(path, 'r') as f:
            data = json.load(f)
        
        # Проверяем, что в JSON есть поле 'tickers'
        tickers = data.get('tickers', [])
        
        # Возвращаем список словарей с нужными полями (как в PostgreSQL)
        result = []
        for item in tickers:
            ticker = item.get('ticker', '')
            price = item.get('price', 0)
            if ticker:
                result.append({"ticker": ticker, "price": price})
        
        return result
    
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in {path}: {e}")
        return []
    except Exception as e:
        print(f"Error reading tickers from JSON: {e}")
        return []