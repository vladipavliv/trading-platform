/**
 * @author Vladimir Pavliv
 * @date 2025-08-04
 */

#include <iostream>

#include <gtest/gtest.h>

#include "config/server_config.hpp"
#include "execution/orderbook/flat_order_book.hpp"
#include "utils/data_generator.hpp"
#include "utils/test_utils.hpp"

namespace hft::tests {

using namespace server;
using namespace utils;

namespace {
ClientId cId() { return genId(); }
OrderId oId() { return genId(); }
Timestamp ts() { return getTimestampNs(); }

const OrderAction BUY = OrderAction::Buy;
const OrderAction SELL = OrderAction::Sell;
const Ticker tkr = genTicker();
} // namespace

using OrderBook = FlatOrderBook;

class OrderBookFixture : public ::testing::Test {
public:
  UPtr<OrderBook> book;
  Vector<InternalOrderStatus> statusq;

  void SetUp() override {
    ServerConfig::load("utest_server_config.ini");
    LOG_INIT(ServerConfig::cfg.logOutput);
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

  void addOrder(InternalOrderEvent order) {
    book->add(order, *this);
    book->sendAck(order, *this);
  }
};

template <>
void OrderBookFixture::post<InternalOrderStatus>(CRef<InternalOrderStatus> event) {
  statusq.push_back(event);
}

TEST_F(OrderBookFixture, OrdersWontMatch) {
  statusq.clear();

  uint32_t id = 0;
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 40}, nullptr, tkr, SELL});
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 50}, nullptr, tkr, SELL});
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 60}, nullptr, tkr, SELL});

  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 30}, nullptr, tkr, BUY});
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 20}, nullptr, tkr, BUY});
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 10}, nullptr, tkr, BUY});

  printStatusQ();
  ASSERT_TRUE(statusq.size() == 6);
}

TEST_F(OrderBookFixture, 3Buy3SellMatch) {
  statusq.clear();

  uint32_t id = 0;
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 40}, nullptr, tkr, BUY});
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 50}, nullptr, tkr, BUY});
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 60}, nullptr, tkr, BUY});

  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 30}, nullptr, tkr, SELL});
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 20}, nullptr, tkr, SELL});
  addOrder({{SystemOrderId{++id}, BookOrderId{}, 1, 10}, nullptr, tkr, SELL});

  printStatusQ();
  ASSERT_EQ(statusq.size(), 9);
}

TEST_F(OrderBookFixture, 10Buy1SellMatch) {
  statusq.clear();

  Quantity quantity{1};
  Price price{10};

  for (uint32_t idx = 0; idx < 10; ++idx) {
    addOrder({{SystemOrderId{idx}, BookOrderId{}, idx, price}, nullptr, tkr, BUY});
    quantity += idx;
  }

  addOrder({{SystemOrderId{42}, BookOrderId{}, quantity, price}, nullptr, tkr, SELL});

  printStatusQ();
  ASSERT_EQ(statusq.size(), 21);
}

} // namespace hft::tests
