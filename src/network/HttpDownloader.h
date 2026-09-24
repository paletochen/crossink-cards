#pragma once
#include <HalStorage.h>
#include <Stream.h>

#include <functional>
#include <string>
#include <string_view>
#include <utility>

/**
 * HTTP client utility for fetching content and downloading files.
 * Streams requests through the configured HTTP transport so large downloads
 * do not need to fit in RAM.
 */
class HttpDownloader {
 public:
  using ProgressCallback = std::function<void(size_t downloaded, size_t total)>;
  using CancelCallback = std::function<bool()>;
  // Called with each body chunk as it arrives; return false to abort. Lets a
  // streaming parser consume the response without buffering the whole body.
  using DataCallback = std::function<bool(const uint8_t* data, size_t len)>;

  enum DownloadError {
    OK = 0,
    HTTP_ERROR,
    FILE_ERROR,
    ABORTED,
    TIMED_OUT,
  };

  enum class Transport {
    ESP_HTTP,
    WOLFSSL,
  };

  struct DownloadOptions {
    DownloadOptions() = default;
    explicit DownloadOptions(bool preservePartial, bool resumePartial = false,
                             CancelCallback shouldCancel = nullptr, size_t bufferSize = 0,
                             Transport transport = Transport::ESP_HTTP)
        : preservePartial(preservePartial),
          resumePartial(resumePartial),
          shouldCancel(std::move(shouldCancel)),
          bufferSize(bufferSize),
          transport(transport) {}

    bool preservePartial = false;
    bool resumePartial = false;
    CancelCallback shouldCancel = nullptr;
    size_t bufferSize = 0;
    Transport transport = Transport::ESP_HTTP;
    // Borrowed only for this synchronous request. Basic credentials are sent
    // only to this origin; empty keeps the request URL as the credential origin.
    std::string_view authorizationOrigin;

    uint32_t operationTimeoutMs = 60000;
    uint32_t overallTimeoutMs = 0;
    bool bypassCache = false;
    CancelCallback cancelRequested = nullptr;
    int* outHttpStatus = nullptr;
    size_t* outBytesReceived = nullptr;
    size_t* outExpectedBytes = nullptr;
  };

  /**
   * Fetch text content from a URL with optional credentials.
   */
  static bool fetchUrl(const std::string& url, std::string& outContent, const std::string& username = "",
                       const std::string& password = "");

  static bool fetchUrl(const std::string& url, Stream& stream, const std::string& username = "",
                       const std::string& password = "");

  /**
   * Stream the response body to onData as it arrives, without buffering it.
   */
  static bool fetchUrl(const std::string& url, const DataCallback& onData, const std::string& username = "",
                       const std::string& password = "");

  /**
   * Stream a URL with cancellation/progress support and a detailed result.
   */
  static DownloadError streamUrl(const std::string& url, const DataCallback& onData,
                                 ProgressCallback progress = nullptr, const std::string& username = "",
                                 const std::string& password = "", DownloadOptions options = DownloadOptions());

  /**
   * Download a file to the SD card with optional credentials.
   */
  static DownloadError downloadToFile(const std::string& url, const std::string& destPath,
                                      ProgressCallback progress = nullptr, bool* cancelFlag = nullptr,
                                      const std::string& username = "", const std::string& password = "",
                                      DownloadOptions options = DownloadOptions());

  static DownloadError downloadToFile(const std::string& url, const std::string& destPath,
                                      const DownloadOptions& options, ProgressCallback progress = nullptr,
                                      const std::string& username = "", const std::string& password = "",
                                      bool downgradeRedirectsToHttp = false);
};
