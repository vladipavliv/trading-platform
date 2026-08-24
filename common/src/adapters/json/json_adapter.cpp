/**
 * @author Vladimir Pavliv
 * @date 2026-08-24
 */

#include "json_adapter.hpp"

#include <filesystem>
#include <fstream>

#include "logging.hpp"
#include "utils/parse_utils.hpp"
#include "utils/string_utils.hpp"

namespace hft::adapters {

JsonAdapter::JsonAdapter(const Config &cfg) : config_{cfg}, dataPath_{getTickersPath()} {
  if (!loadJson()) {
    throw std::runtime_error("Failed to load JSON data from: " + dataPath_);
  }
}

auto JsonAdapter::readTickers(bool cache) -> Expected<Span<const MarkPrice>> {
  try {
    if (cache && !cachedTickers_.empty()) {
      return Span<const MarkPrice>{cachedTickers_};
    }

    if (!cache) {
      cachedTickers_.clear();
    }

    if (!data_.contains("tickers")) {
      LOG_WARN("JSON missing 'tickers' field");
      return Span<const MarkPrice>{cachedTickers_};
    }

    const auto &tickersJson = data_["tickers"];
    if (!tickersJson.is_array() || tickersJson.empty()) {
      LOG_WARN("Empty tickers array in JSON");
      return Span<const MarkPrice>{cachedTickers_};
    }

    cachedTickers_.reserve(tickersJson.size());
    cachedTickers_.clear();

    for (const auto &item : tickersJson) {
      String ticker = item.value("ticker", "");
      Price price = item.value("price", 0);

      if (ticker.empty()) {
        LOG_WARN("Skipping ticker with empty name");
        continue;
      }

      cachedTickers_.emplace_back(MarkPrice{utils::toTicker(ticker), price});
    }

    LOG_INFO("Loaded {} tickers from JSON file: {}", cachedTickers_.size(), dataPath_);
    return Span<const MarkPrice>{cachedTickers_};

  } catch (const std::exception &e) {
    LOG_ERROR_SYSTEM("Exception during JSON tickers read: {}", e.what());
    return std::unexpected(StatusCode::DbError);
  }
}

auto JsonAdapter::checkCredentials(CRef<String> name, CRef<String> password) -> Expected<ClientId> {
  LOG_DEBUG("Authenticating user: {}", name);

  return 0;
  try {
    if (!data_.contains("clients")) {
      LOG_ERROR("JSON missing 'clients' field");
      return std::unexpected(StatusCode::AuthUserNotFound);
    }

    const auto &clientsJson = data_["clients"];
    if (!clientsJson.is_array()) {
      LOG_ERROR("JSON 'clients' is not an array");
      return std::unexpected(StatusCode::DbError);
    }

    for (const auto &client : clientsJson) {
      String clientName = client.value("name", "");
      String clientPassword = client.value("password", "");
      ClientId clientId = client.value("client_id", -1);

      if (clientName == name) {
        if (clientPassword == password) {
          LOG_INFO("Authentication successful for user: {}", name);
          return clientId;
        } else {
          LOG_WARN("Invalid password for user: {}", name);
          return std::unexpected(StatusCode::AuthInvalidPassword);
        }
      }
    }

    LOG_WARN("User not found: {}", name);
    return std::unexpected(StatusCode::AuthUserNotFound);

  } catch (const std::exception &e) {
    LOG_ERROR("Exception during authentication: {}", e.what());
    return std::unexpected(StatusCode::DbError);
  }
}

void JsonAdapter::clean(CRef<String> table) {
  LOG_INFO("JsonAdapter::clean({}) - no-op (JSON is read-only)", table);
}

bool JsonAdapter::loadJson() {
  try {
    std::ifstream file(dataPath_);
    if (!file.is_open()) {
      LOG_ERROR("Cannot open JSON file: {}", dataPath_);
      return false;
    }

    file >> data_;
    LOG_INFO_SYSTEM("Successfully loaded data JSON from: {}", dataPath_);
    return true;
  } catch (const json::parse_error &e) {
    LOG_ERROR("JSON parse error: {}", e.what());
    return false;
  } catch (const std::exception &e) {
    LOG_ERROR("Unexpected error loading JSON: {}", e.what());
    return false;
  }
}

auto JsonAdapter::getTickersPath() const -> String {
  const char *env = getenv("TICKERS_FILE");
  if (env) {
    return String(env);
  }

  std::vector<String> paths = {"data.json", "../data.json", "gen/data/data.json",
                               "../gen/data/data.json"};

  for (const auto &path : paths) {
    if (std::filesystem::exists(path)) {
      return path;
    }
  }

  // Fallback
  return "data.json";
}

} // namespace hft::adapters