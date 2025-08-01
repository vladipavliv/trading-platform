/**
 * @author Vladimir Pavliv
 * @date 2025-03-07
 */

#ifndef HFT_SERVER_SERVERCONTROLCENTER_HPP
#define HFT_SERVER_SERVERCONTROLCENTER_HPP

#include "adapters/kafka/kafka_adapter.hpp"
#include "adapters/postgres/postgres_adapter.hpp"
#include "config/server_config.hpp"
#include "console_reader.hpp"
#include "domain_types.hpp"
#include "execution/coordinator.hpp"
#include "network/authenticator.hpp"
#include "network/network_server.hpp"
#include "price_feed.hpp"
#include "server_command.hpp"
#include "server_command_parser.hpp"
#include "server_events.hpp"
#include "server_types.hpp"
#include "storage/storage.hpp"

namespace hft::server {

/**
 * @brief Creates all the components and controls the flow
 */
class ServerControlCenter {
public:
  using ServerConsoleReader = ConsoleReader<ServerCommandParser>;
  using Kafka = KafkaAdapter<ServerCommandParser>;

  ServerControlCenter()
      : marketData_{readMarketData()}, networkServer_{bus_},
        authenticator_{bus_.systemBus, dbAdapter_}, coordinator_{bus_, marketData_},
        consoleReader_{bus_.systemBus}, priceFeed_{bus_, marketData_}, kafka_{bus_.systemBus},
        storage_{dbAdapter_, marketData_} {
    // System bus subscriptions
    bus_.systemBus.subscribe(ServerEvent::Operational, [this] {
      // start the network server only after internal components are fully operational
      LOG_INFO_SYSTEM("Server is ready");
      networkServer_.start();
    });

    // commands
    bus_.systemBus.subscribe(ServerCommand::Shutdown, [this] { stop(); });

    // kafka topics and commands
    kafka_.addConsumeTopic(Config::get<String>("kafka.kafka_server_cmd_topic"));
    kafka_.addProduceTopic<RuntimeMetrics>(Config::get<String>("kafka.kafka_metrics_topic"));
  }

  void start() {
    greetings();
    storage_.loadOrders();

    coordinator_.start();
    kafka_.start();
    bus_.run();
  }

  void stop() {
    kafka_.stop();
    networkServer_.stop();
    coordinator_.stop();
    bus_.stop();
    storage_.saveOrders();

    LOG_INFO_SYSTEM("stonk");
  }

private:
  void greetings() {
    LOG_INFO_SYSTEM("Server go stonks");
    LOG_INFO_SYSTEM("Configuration:");
    ServerConfig::log();
    consoleReader_.printCommands();
    LOG_INFO_SYSTEM("Tickers loaded: {}", marketData_.size());
  }

  // TODO(self) Move to storage
  auto readMarketData() -> MarketData {
    const auto result = dbAdapter_.readTickers();
    if (!result) {
      LOG_ERROR("Failed to load ticker data");
      throw std::runtime_error(utils::toString(result.error()));
    }
    const auto &prices = result.value();
    MarketData data;
    data.reserve(prices.size());
    const auto workers = ServerConfig::cfg.coresApp.size();

    size_t idx{0};
    const size_t tickerPerWorker{prices.size() / workers};
    for (const auto &item : prices) {
      LOG_TRACE("{}: ${}", utils::toString(item.ticker), item.price);
      const size_t workerId = std::min(idx / tickerPerWorker, workers - 1);
      data.emplace(item.ticker, std::make_unique<TickerData>(bus_, workerId, item.price));
      ++idx;
    }
    LOG_INFO("Data loaded for {} tickers", data.size());
    return data;
  }

private:
  PostgresAdapter dbAdapter_;

  const MarketData marketData_;

  Bus bus_;

  NetworkServer networkServer_;
  Authenticator authenticator_;
  Coordinator coordinator_;
  ServerConsoleReader consoleReader_;
  PriceFeed priceFeed_;
  Kafka kafka_;
  Storage storage_;
};

} // namespace hft::server

#endif // HFT_SERVER_SERVERCONTROLCENTER_HPP
