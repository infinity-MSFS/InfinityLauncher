#pragma once
#include <Json/json.hpp>
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>

#include "Frontend/Background/Meteors.hpp"


namespace Infinity {

  struct BetaLink {
    std::string link;
    std::string groupName;
  };

  class Betas {
public:
    Betas() {}
    ~Betas() = default;

    void Render() {
      Meteors::GetInstance()->Update();
      Meteors::GetInstance()->Render();
    }

private:
    std::vector<BetaLink> GetLinks(const std::string& hwid) {
      std::vector<BetaLink> links;
      if (hwid.empty()) return links;

      std::string url = "http://3.144.20.213:3030/get_link/" + hwid;

      CURL* curl = curl_easy_init();
      if (!curl) {
        std::cerr << "curl_easy_init failed" << std::endl;
        return links;
      }

      std::string response;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform failed: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return links;
      }

      curl_easy_cleanup(curl);

      try {
        auto json = nlohmann::json::parse(response);
        for (const auto& item: json["links"]) {
          links.push_back(BetaLink{item.value("link", ""), item.value("groupName", "")});
        }
      } catch (const std::exception& e) {
        std::cerr << "JSON parse error" << e.what() << std::endl;
      }
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
      static_cast<std::string*>(userp)->append(static_cast<char*>(contents), size * nmemb);
      return size * nmemb;
    }
  };

}  // namespace Infinity
