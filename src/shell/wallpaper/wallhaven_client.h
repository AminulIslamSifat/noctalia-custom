#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class HttpClient;

namespace wallhaven {

  struct SearchResult {
    std::string id;
    std::string url;           // wallhaven page URL
    std::string thumbUrl;      // thumbnail URL
    std::string fullUrl;       // full-resolution download URL
    std::string resolution;    // e.g. "1920x1080"
    std::string category;      // general/anime/people
    std::string purity;        // sfw/sketchy/nsfw
    std::uint64_t fileSize = 0;
    std::string fileType;      // image/jpeg, image/png
  };

  struct SearchResponse {
    bool ok = false;
    std::string error;
    std::vector<SearchResult> results;
    std::size_t currentPage = 1;
    std::size_t lastPage = 1;
    std::size_t totalResults = 0;
  };

  enum class Category : std::uint8_t {
    General = 1,
    Anime = 2,
    People = 4,
  };

  enum class Purity : std::uint8_t {
    SFW = 1,
    Sketchy = 2,
    NSFW = 4,
  };

  enum class Sorting : std::uint8_t {
    DateAdded,
    Relevance,
    Random,
    Views,
    Favorites,
    Toplist,
  };

  struct SearchParams {
    std::string query;
    std::uint8_t categories = 0b111; // all by default
    std::uint8_t purity = 0b001;     // SFW only by default
    Sorting sorting = Sorting::DateAdded;
    std::string topRange = "1M";     // used when sorting == Toplist
    std::string ratios;              // e.g. "16x9"
    std::string resolutions;         // e.g. "1920x1080"
    std::string colors;              // hex without #
    std::size_t page = 1;
    std::string apiKey;              // optional, needed for NSFW
  };

  // Async Wallhaven API client. All callbacks fire on the main thread via HttpClient's
  // deferred-callback semantics.
  class Client {
  public:
    using SearchCallback = std::function<void(SearchResponse)>;

    explicit Client(HttpClient& httpClient);

    void search(const SearchParams& params, SearchCallback callback);

    // Download a wallpaper to destPath. callback(true) on success.
    void downloadWallpaper(std::string_view fullUrl, const std::filesystem::path& destPath,
                           HttpClient::CompletionCallback callback);

  private:
    [[nodiscard]] std::string buildSearchUrl(const SearchParams& params) const;

    HttpClient& m_httpClient;
  };

} // namespace wallhaven
