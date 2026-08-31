#include "shell/wallpaper/wallhaven_client.h"

#include "core/log.h"
#include "net/http_client.h"

#include <nlohmann/json.hpp>

#include <sstream>
#include <string>

namespace {
  constexpr Logger kLog("wallhaven");
  constexpr std::string_view kBaseUrl = "https://wallhaven.cc/api/v1";

  [[nodiscard]] std::string sortingToString(wallhaven::Sorting s) {
    switch (s) {
    case wallhaven::Sorting::DateAdded:
      return "date_added";
    case wallhaven::Sorting::Relevance:
      return "relevance";
    case wallhaven::Sorting::Random:
      return "random";
    case wallhaven::Sorting::Views:
      return "views";
    case wallhaven::Sorting::Favorites:
      return "favorites";
    case wallhaven::Sorting::Toplist:
      return "toplist";
    }
    return "date_added";
  }

  // Encode categories bitmask to "111"-style string.
  [[nodiscard]] std::string encodeCategories(std::uint8_t cats) {
    std::string result(3, '0');
    if (cats & static_cast<std::uint8_t>(wallhaven::Category::General))
      result[0] = '1';
    if (cats & static_cast<std::uint8_t>(wallhaven::Category::Anime))
      result[1] = '1';
    if (cats & static_cast<std::uint8_t>(wallhaven::Category::People))
      result[2] = '1';
    return result;
  }

  [[nodiscard]] std::string encodePurity(std::uint8_t pur) {
    std::string result(3, '0');
    if (pur & static_cast<std::uint8_t>(wallhaven::Purity::SFW))
      result[0] = '1';
    if (pur & static_cast<std::uint8_t>(wallhaven::Purity::Sketchy))
      result[1] = '1';
    if (pur & static_cast<std::uint8_t>(wallhaven::Purity::NSFW))
      result[2] = '1';
    return result;
  }

  [[nodiscard]] std::string urlEncode(const std::string& value) {
    std::ostringstream encoded;
    for (char c : value) {
      if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
        encoded << c;
      } else {
        encoded << '%' << std::uppercase << std::hex << static_cast<int>(static_cast<unsigned char>(c));
      }
    }
    return encoded.str();
  }
} // namespace

namespace wallhaven {

  Client::Client(HttpClient& httpClient) : m_httpClient(httpClient) {}

  std::string Client::buildSearchUrl(const SearchParams& params) const {
    std::ostringstream url;
    url << kBaseUrl << "/search?";
    url << "q=" << urlEncode(params.query);
    url << "&categories=" << encodeCategories(params.categories);
    url << "&purity=" << encodePurity(params.purity);
    url << "&sorting=" << sortingToString(params.sorting);
    if (params.sorting == Sorting::Toplist) {
      url << "&topRange=" << urlEncode(params.topRange);
    }
    if (!params.ratios.empty()) {
      url << "&ratios=" << urlEncode(params.ratios);
    }
    if (!params.resolutions.empty()) {
      url << "&resolutions=" << urlEncode(params.resolutions);
    }
    if (!params.colors.empty()) {
      url << "&colors=" << urlEncode(params.colors);
    }
    url << "&page=" << params.page;
    if (!params.apiKey.empty()) {
      url << "&apikey=" << urlEncode(params.apiKey);
    }
    return url.str();
  }

  void Client::search(const SearchParams& params, SearchCallback callback) {
    const std::string url = buildSearchUrl(params);
    kLog.debug("Searching: {}", url);

    HttpRequest req;
    req.method = "GET";
    req.url = url;

    m_httpClient.request(std::move(req), [callback = std::move(callback)](HttpResponse resp) mutable {
      SearchResponse result;

      if (!resp.transportOk) {
        result.ok = false;
        result.error = "Network error";
        callback(std::move(result));
        return;
      }

      if (resp.status != 200) {
        result.ok = false;
        result.error = "HTTP " + std::to_string(resp.status);
        callback(std::move(result));
        return;
      }

      try {
        auto json = nlohmann::json::parse(resp.body);

        if (auto metaIt = json.find("meta"); metaIt != json.end()) {
          result.currentPage = metaIt->value("current_page", 1U);
          result.lastPage = metaIt->value("last_page", 1U);
          result.totalResults = metaIt->value("total", 0U);
        }

        if (auto dataIt = json.find("data"); dataIt != json.end() && dataIt->is_array()) {
          for (const auto& item : *dataIt) {
            SearchResult sr;
            sr.id = item.value("id", "");
            sr.url = item.value("url", "");
            sr.resolution = item.value("resolution", "");
            sr.category = item.value("category", "");
            sr.purity = item.value("purity", "");
            sr.fileSize = item.value("file_size", 0ULL);
            sr.fileType = item.value("file_type", "");

            if (auto thumbsIt = item.find("thumbs"); thumbsIt != item.end()) {
              sr.thumbUrl = thumbsIt->value("original", "");
            }

            // Full-res URL is in the "path" field
            sr.fullUrl = item.value("path", "");

            if (!sr.fullUrl.empty()) {
              result.results.push_back(std::move(sr));
            }
          }
        }

        result.ok = true;
      } catch (const std::exception& e) {
        result.ok = false;
        result.error = std::string("JSON parse error: ") + e.what();
      }

      callback(std::move(result));
    });
  }

  void Client::downloadWallpaper(std::string_view fullUrl, const std::filesystem::path& destPath,
                                 HttpClient::CompletionCallback callback) {
    m_httpClient.download(fullUrl, destPath, std::move(callback));
  }

} // namespace wallhaven
