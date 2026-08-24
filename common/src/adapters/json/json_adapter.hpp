/**
 * @author Vladimir Pavliv
 * @date 2026-08-24
 */

#ifndef HFT_COMMON_JSONADAPTER_HPP
#define HFT_COMMON_JSONADAPTER_HPP

#include <nlohmann/json.hpp>

#include "config/config.hpp"
#include "container_types.hpp"
#include "domain_types.hpp"
#include "functional_types.hpp"
#include "primitive_types.hpp"
#include "ptr_types.hpp"

namespace hft::adapters {

using json = nlohmann::json;

class JsonAdapter {
public:
  explicit JsonAdapter(const Config &cfg);

  auto readTickers(bool cache = true) -> Expected<Span<const MarkPrice>>;
  auto checkCredentials(CRef<String> name, CRef<String> password) -> Expected<ClientId>;
  void clean(CRef<String> table);

private:
  bool loadJson();

private:
  String getTickersPath() const;

  const Config &config_;
  const String dataPath_;
  json data_;

  inline static std::vector<MarkPrice> cachedTickers_;
};
} // namespace hft::adapters

#endif // HFT_COMMON_JSONADAPTER_HPP