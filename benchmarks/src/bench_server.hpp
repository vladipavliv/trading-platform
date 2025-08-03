/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#ifndef HFT_BENCHSERVER_HPP
#define HFT_BENCHSERVER_HPP

#include <benchmark/benchmark.h>

#include <iostream>

#include <benchmark/benchmark.h>

#include "adapters/postgres/postgres_adapter.hpp"
#include "commands/server_command.hpp"
#include "config/server_config.hpp"
#include "execution/coordinator.hpp"
#include "server_types.hpp"
#include "storage/storage.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

class ServerFixture : public benchmark::Fixture {
public:
  inline static server::MarketData data;

  inline static UPtr<server::Bus> bus;
  inline static UPtr<server::Coordinator> coordinator;

  inline static size_t orderLimit{0};
  inline static std::vector<server::ServerOrder> orders;

  inline static std::jthread systemThread;
  inline static std::atomic_flag flag{ATOMIC_FLAG_INIT};

  void SetUp(::benchmark::State &state) override;
  void TearDown(const ::benchmark::State &) override;

  static void GlobalSetUp();
  static void GlobalTearDown();

  static void setupBus();
  static void setupCoordinator();

  static void fillMarketData();
  static void fillOrders();
};

} // namespace hft::benchmarks

#endif // HFT_BENCHSERVER_HPP