/**
 * @author Vladimir Pavliv
 * @date 2025-08-04
 */

#include <iostream>

#include <gtest/gtest.h>

#include "config/server_config.hpp"
#include "execution/orderbook/flat_order_book.hpp"
#include "execution/orderbook/price_level_order_book.hpp"
#include "traits.hpp"
#include "utils/data_generator.hpp"
#include "utils/test_utils.hpp"

namespace hft::tests {

using namespace server;
using namespace utils;

namespace {
ClientId cId() { return genId(); }
OrderId oId() { return genId(); }
Timestamp ts() { return getTimestampNs(); }

BookOrderId booId() {
  static uint32_t counter;
  return BookOrderId::make(counter++, 1);
}

SystemOrderId syoId() {
  static uint32_t counter;
  return SystemOrderId::make(counter++, 1);
}

const OrderAction BUY = OrderAction::Buy;
const OrderAction SELL = OrderAction::Sell;
const Ticker tkr = genTicker();
} // namespace

class OrderBookFixture : public ::testing::Test {
public:
  const ServerConfig cfg;
  UPtr<OrderBook> book;
  Vector<InternalOrderStatus> statusq;

  OrderBookFixture() : cfg{"utest_server_config.ini"} {}

  void SetUp() override {
    LOG_INIT(cfg.logOutput);
    book = std::make_unique<OrderBook>();
  }

  void TearDown() override {}

  void printStatusQ() {
    if (statusq.empty()) {
      std::cout << "StatusQ is empty" << std::endl;
      return;
    }
    for (auto &status : statusq) {
      std::cout << toString(status) << std::endl;
    }
  }

  template <typename EventType>
  void post(CRef<EventType>) {}

  void addOrder(InternalOrderEvent order) { book->add(order, *this); }
};

template <>
void OrderBookFixture::post<InternalOrderStatus>(CRef<InternalOrderStatus> event) {
  statusq.push_back(event);
}

auto makeOrder(uint32_t qty, uint32_t price, OrderAction action) -> InternalOrderEvent {
  return {{syoId(), booId(), qty, price}, nullptr, tkr, action};
}

TEST_F(OrderBookFixture, OrdersWontMatch) {
  statusq.clear();

  addOrder(makeOrder(1, 40, SELL));
  addOrder(makeOrder(1, 50, SELL));
  addOrder(makeOrder(1, 60, SELL));

  addOrder(makeOrder(1, 30, BUY));
  addOrder(makeOrder(1, 20, BUY));
  addOrder(makeOrder(1, 10, BUY));

  printStatusQ();
  // 6 ack
  ASSERT_EQ(statusq.size(), 6);
}

TEST_F(OrderBookFixture, 3Buy3SellMatch) {
  statusq.clear();

  addOrder(makeOrder(1, 40, BUY));
  addOrder(makeOrder(1, 50, BUY));
  addOrder(makeOrder(1, 60, BUY));

  addOrder(makeOrder(1, 30, SELL));
  addOrder(makeOrder(1, 20, SELL));
  addOrder(makeOrder(1, 10, SELL));

  printStatusQ();
  // 3 buy ack + 3 full no ack
  ASSERT_EQ(statusq.size(), 6);
}

TEST_F(OrderBookFixture, 10Buy1SellMatch) {
  statusq.clear();

  Quantity quantity{1};
  Price price{10};

  for (uint32_t idx = 1; idx < 10; ++idx) {
    addOrder(makeOrder(idx, price, BUY));
    quantity += idx;
  }

  addOrder(makeOrder(quantity, price, SELL));

  printStatusQ();
  // 9buy ack + 1full no ack
  ASSERT_EQ(statusq.size(), 10);
}

} // namespace hft::tests
