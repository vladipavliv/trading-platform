/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_SERVER_MARKET_HPP
#define HFT_SERVER_MARKET_HPP

#include <spdlog/spdlog.h>

#include "boost_types.hpp"
#include "db/postgres_adapter.hpp"
#include "market_types.hpp"
#include "order_book.hpp"
#include "price_feed.hpp"
#include "server_types.hpp"
#include "utils/utils.hpp"

namespace hft::server::market {

class Market {
public:
  Market(ServerSink &sink) : mSink{sink}, mPriceFeed{mSink} {}

  void start() {
    mPriceFeed.setCurrentPrices(db::PostgresAdapter::readTickers());

    mSink.dataSink.setHandler<Order>([this](const Order &order) { processOrder(order); });
    mSink.controlSink.setHandler(ServerCommand::PriceFeedStart,
                                 [this](ServerCommand command) { mPriceFeed.start(); });
    mSink.controlSink.setHandler(ServerCommand::PriceFeedStop,
                                 [this](ServerCommand command) { mPriceFeed.stop(); });
  }
  void stop() {}

private:
  void processOrder(const Order &order) {
    spdlog::debug(utils::toString(order));
    OrderStatus status{order.traderId,         order.id,       order.ticker,
                       FulfillmentState::Full, order.quantity, order.price};
    mSink.networkSink.post(status);
  }

private:
  ServerSink &mSink;

  std::vector<PriceUpdate> mPrices;

  OrderBook mBook;
  PriceFeed mPriceFeed;
};

} // namespace hft::server::market

#endif // HFT_SERVER_MARKET_HPP
