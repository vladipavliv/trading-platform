#!/usr/bin/env python3
"""
Generates a bunch of random tickers with prices and saves to JSON
"""

import random
import string
import json
import sys
import argparse
from pathlib import Path

def generate_ticker():
    return "".join(random.choices(string.ascii_uppercase, k=4))

def generate_price():
    return random.randint(10, 7500)

def generate_tickers(amount, output_file):
    try:
        # Создаем директорию, если её нет
        Path(output_file).parent.mkdir(parents=True, exist_ok=True)
        
        tickers = []
        for _ in range(amount):
            ticker = generate_ticker()
            price = generate_price()
            tickers.append({"ticker": ticker, "price": price})
        
        with open(output_file, 'w') as f:
            json.dump({"tickers": tickers}, f, indent=2)
        
        print(f"Generated {amount} random tickers and saved to {output_file}")
        return True
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return False

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate random tickers JSON")
    parser.add_argument("amount", type=int, help="Number of tickers to generate")
    parser.add_argument("-o", "--output", default="data.json",
                        help="Output file path (default: data.json)")
    
    args = parser.parse_args()
    
    if args.amount <= 0:
        print("Error: Amount must be positive", file=sys.stderr)
        sys.exit(1)
    
    success = generate_tickers(args.amount, args.output)
    sys.exit(0 if success else 1)