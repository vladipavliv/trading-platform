/**
 * @author Vladimir Pavliv
 * @date 2025-08-04
 */

#include <iostream>

#include <gtest/gtest.h>

#include "config/server_config.hpp"
#include "execution/order_book.hpp"

namespace hft::tests {

using namespace server;
using namespace utils;

namespace {
ClientId cId() { return generateConnectionId(); }
OrderId oId() { return generateOrderId(); }
Timestamp ts() { return getTimestamp(); }

const OrderAction BUY = OrderAction::Buy;
const OrderAction SELL = OrderAction::Sell;
const Ticker tkr = generateTicker();
} // namespace

class OrderBookFixture : public ::testing::Test {
public:
  UPtr<OrderBook> book;
  Vector<ServerOrderStatus> statusq;

  inline static std::once_flag initFlag;

  void SetUp() override {
    std::call_once(initFlag, []() {
      if (!Config::isLoaded()) {
        // Config could be also loaded in other benches
        ServerConfig::load("test_server_config.ini");
        LOG_INIT(ServerConfig::cfg.logOutput);
      }
    });
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
};

template <>
void OrderBookFixture::post<ServerOrderStatus>(CRef<ServerOrderStatus> event) {
  statusq.push_back(event);
}

TEST_F(OrderBookFixture, OrderBookLimitReqched) {
  for (size_t idx = 0; idx < ServerConfig::cfg.orderBookLimit; ++idx) {
    ASSERT_TRUE(book->add({0, generateOrder()}, *this));
  }
  ASSERT_FALSE(book->add({0, generateOrder()}, *this));
}

TEST_F(OrderBookFixture, OrdersWontMatch) {
  book->add({cId(), {oId(), ts(), tkr, 1, 40, SELL}}, *this);
  book->add({cId(), {oId(), ts(), tkr, 1, 50, SELL}}, *this);
  book->add({cId(), {oId(), ts(), tkr, 1, 60, SELL}}, *this);

  book->add({cId(), {oId(), ts(), tkr, 1, 30, BUY}}, *this);
  book->add({cId(), {oId(), ts(), tkr, 1, 20, BUY}}, *this);
  book->add({cId(), {oId(), ts(), tkr, 1, 10, BUY}}, *this);

  book->match(*this);

  printStatusQ();
  ASSERT_TRUE(statusq.empty());
}

TEST_F(OrderBookFixture, 3Buy3SellMatch) {
  book->add({cId(), {oId(), ts(), tkr, 1, 40, BUY}}, *this);
  book->add({cId(), {oId(), ts(), tkr, 1, 50, BUY}}, *this);
  book->add({cId(), {oId(), ts(), tkr, 1, 60, BUY}}, *this);

  book->add({cId(), {oId(), ts(), tkr, 1, 30, SELL}}, *this);
  book->add({cId(), {oId(), ts(), tkr, 1, 20, SELL}}, *this);
  book->add({cId(), {oId(), ts(), tkr, 1, 10, SELL}}, *this);

  book->match(*this);

  printStatusQ();
  ASSERT_EQ(statusq.size(), 3);
}

TEST_F(OrderBookFixture, 10Buy1SellMatch) {
  Quantity quantity{0};
  Price price{10};

  for (uint32_t idx = 0; idx < 10; ++idx) {
    book->add({cId(), {oId(), ts(), tkr, idx, price, BUY}}, *this);
    quantity += idx;
  }

  book->add({cId(), {oId(), ts(), tkr, quantity, price, SELL}}, *this);
  book->match(*this);

  printStatusQ();
  ASSERT_EQ(statusq.size(), 10);
}

} // namespace hft::tests
