import os
import pytest
import random
import time
import threading
import pickle

from tests.integration.utils import startup, hft_client, serialization
from hft.serialization.gen.fbs.domain import OrderStatus

@pytest.fixture(scope="module")
def server():
    with startup.running_server() as proc:
        yield proc

def generate_and_save_orders(num_orders: int, filename: str, tickers):
    orders = [
        serialization.create_order_message(
            i,
            int(time.time() * 1_000_000),
            random.choice(tickers)['ticker'],
            random.randint(1, 100),
            random.randint(10, 1000),
            i % 2
        )
        for i in range(num_orders)
    ]
    with open(filename, 'wb') as f:
        pickle.dump(orders, f)
    return orders

def load_orders(filename: str):
    with open(filename, 'rb') as f:
        return pickle.load(f)

def test_order(server, tickers, client_name, client_password):
    client = hft_client.HftClient()
    client.connect()
    client.login(client_name, client_password)

    print(f"Sending orders")

    price = 1
    quantity = 1

    orderBuy = serialization.create_order_message(1, 2, tickers[0]['ticker'], quantity, price, 0)
    orderSell = serialization.create_order_message(3, 4, tickers[0]['ticker'], quantity, price, 1)

    client.sendUpstream(orderBuy)
    client.sendUpstream(orderSell)

    order_status_accepted = client.receiveDownstream()
    assert isinstance(order_status_accepted, OrderStatus.OrderStatus), f"Unexpected server response to Order {order_status_accepted}"
    assert order_status_accepted.State() == 0 # accepted

    order_status_accepted = client.receiveDownstream()
    assert isinstance(order_status_accepted, OrderStatus.OrderStatus), f"Unexpected server response to Order {order_status_accepted}"
    assert order_status_accepted.State() == 0 # accepted

    order_status_filled = client.receiveDownstream()
    assert isinstance(order_status_filled, OrderStatus.OrderStatus), f"Unexpected server response to Order {order_status_filled}"
    
    assert order_status_filled.FillPrice() == price
    assert order_status_filled.Quantity() == quantity
    assert order_status_filled.State() == 3 # full
    assert order_status_filled.OrderId() == 3

    print(f"OrderStatus {order_status_filled.State()}")

    client.close()

def spam_orders(num_orders: int, tickers, client_name, client_password):
    try:
        client = hft_client.HftClient()
        client.connect()
        client.login(client_name, client_password)

        filename = f"orders_{client_name}_{num_orders}.pkl"

        if os.path.exists(filename):
            print("Loading orders from file...")
            load_start = time.time()
            orders = load_orders(filename)
            load_duration = time.time() - load_start
            print(f"Loaded {len(orders)} orders in {load_duration:.2f}s")
        else:
            print(f"Generating {num_orders} orders...")
            gen_start = time.time()
            orders = generate_and_save_orders(num_orders, filename, tickers)
            gen_duration = time.time() - gen_start
            print(f"Generated and saved {num_orders} orders in {gen_duration:.2f}s")

        send_start = time.time()
        for order in orders:
            client.sendUpstream(order)
        send_duration = time.time() - send_start

        print(f"Sent {num_orders} orders in {send_duration:.2f}s "
              f"({num_orders/send_duration:.2f} orders/sec)")

        client.close()
    except AssertionError as e:
        print(f"Thread {client_name} exception: {e}")

def test_stress(server, stress_iterations, tickers, client_name, client_password):
    num_clients = 1

    threads = []
    for _ in range(num_clients):
        t = threading.Thread(target=spam_orders, args=(stress_iterations, tickers, client_name, client_password))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()
    
    time.sleep(1)
