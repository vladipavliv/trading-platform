/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_BENCHSERVER_HPP
#define HFT_BENCHSERVER_HPP

#include <benchmark/benchmark.h>

#include "adapters/postgres/postgres_adapter.hpp"
#include "commands/server_command.hpp"
#include "config/server_config.hpp"
#include "execution/coordinator.hpp"
#include "server_types.hpp"
#include "storage/storage.hpp"
#include "types.hpp"
#include "types/constants.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

class BM_Sys_ServerFix : public benchmark::Fixture {
public:
  BM_Sys_ServerFix();

  inline static std::once_flag initFlag;
  inline static UPtr<server::MarketData> data;
  inline static UPtr<server::Bus> bus;
  inline static UPtr<server::Coordinator> coordinator;

  inline static size_t tickerCount{10};
  inline static size_t workerCount{1};
  inline static size_t orderLimit{ORDER_BOOK_LIMIT};

  inline static std::vector<Ticker> tickers;
  inline static std::vector<server::ServerOrder> orders;

  inline static std::jthread systemThread;
  inline static std::atomic_flag flag{ATOMIC_FLAG_INIT};

  static void GlobalSetUp();
  static void GlobalTearDown();

  static void setupBus();
  static void setupCoordinator();

  static void fillMarketData();
  static void fillOrders();
  static void cleanupOrders();
};

} // namespace hft::benchmarks

#endif // HFT_BENCHSERVER_HPP
