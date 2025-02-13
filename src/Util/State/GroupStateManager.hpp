
#pragma once

#include <iostream>
//#include <map>
#include <vector>
#include <string>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <thread>
#include <future>

#include "curl/curl.h"
#include "zlib.h"
#include "Json/json.hpp"
#include "msgpack.hpp"
#include "imgui.h"
#include "State.hpp"

namespace Infinity {
    inline size_t WriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
        if (!userdata)
            return 0;

        auto *buffer = static_cast<std::vector<uint8_t> *>(userdata);
        const size_t bytes_to_write = size * nmemb;

        try {
            const size_t current_size = buffer->size();
            buffer->resize(current_size + bytes_to_write);
            std::copy(ptr, ptr + bytes_to_write, buffer->data() + current_size);
            return bytes_to_write;
        } catch (const std::exception &) {
            return 0; // Signal error to curl
        }
    }

    class CurlGuard {
        CURL *curl;

    public:
        explicit CurlGuard(CURL *curl) : curl(curl) {
        }

        ~CurlGuard() { if (curl) { curl_easy_cleanup(curl); } }

        CurlGuard(const CurlGuard &) = delete;

        CurlGuard &operator=(const CurlGuard &) = delete;
    };

    struct Package {
        std::string owner;
        std::string repoName;
        std::string version;
        std::string fileName;

        MSGPACK_DEFINE(owner, repoName, version, fileName);
    };

    struct Project {
        std::string name;
        std::string version;
        std::string date;
        std::string changelog;
        std::string overview;
        std::string description;
        std::string background;
        std::optional<std::string> pageBackground;
        std::optional<std::vector<std::string> > variants;
        std::optional<Package> package;

        MSGPACK_DEFINE(name, version, date, changelog, overview, description, background, pageBackground, variants,
                       package);
    };

    struct Palette {
        std::string primary;
        std::string secondary;
        std::string circle1;
        std::string circle2;
        std::string circle3;
        std::string circle4;
        std::string circle5;
        MSGPACK_DEFINE(primary, secondary, circle1, circle2, circle3, circle4, circle5);
    };

    struct BetaProject {
        std::string background;

        MSGPACK_DEFINE(background);
    };

    struct GroupData {
        std::string name;
        std::vector<Project> projects;
        BetaProject beta;
        std::string logo;
        [[deprecated("Handled Via launcher backend")]] std::optional<bool> update;
        std::string path;
        Palette palette;
        std::optional<bool> hide;

        MSGPACK_DEFINE(name, projects, beta, logo, update, path, palette, hide);
    };

    struct GroupDataState {
        std::map<std::string, GroupData> groups;
    };

    using GroupMap = std::map<std::string, GroupData>;
    static GroupMap group_data;

    inline std::map<std::string, GroupData> decode_bin(const std::vector<uint8_t> &raw_data) {
        z_stream strm{};
        strm.next_in = reinterpret_cast<Bytef *>(const_cast<uint8_t *>(raw_data.data()));
        strm.avail_in = static_cast<uInt>(raw_data.size());

        if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
            throw std::runtime_error("Failed to initialize zlib for decompression");
        }

        std::vector<uint8_t> decompressed_data;
        std::array<uint8_t, 8192> buffer{};

        int ret;
        do {
            strm.next_out = buffer.data();
            strm.avail_out = static_cast<uInt>(buffer.size());

            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                inflateEnd(&strm);
                throw std::runtime_error("Decompression error");
            }

            size_t decompressed_size = buffer.size() - strm.avail_out;
            decompressed_data.insert(decompressed_data.end(), buffer.data(), buffer.data() + decompressed_size);
        } while (ret != Z_STREAM_END);

        inflateEnd(&strm);

        try {
            msgpack::object_handle oh = msgpack::unpack(reinterpret_cast<const char *>(decompressed_data.data()),
                                                        decompressed_data.size());
            msgpack::object obj = oh.get();

            std::map<std::string, GroupData> group_data;
            obj.convert(group_data);
            return group_data;
        } catch (const std::exception &e) {
            throw std::runtime_error(std::string("MessagePack deserialization error: ") + e.what());
        }
    }

    // temporary struct for queueing images to be loaded into vulkan textures
    struct ProjectImagesBin {
        std::vector<uint8_t> backgroundImage;
        std::optional<std::vector<uint8_t> > pageBackgroundImage;
    };

    struct BetaProjectImagesBin {
        std::vector<uint8_t> background;
    };

    struct GroupDataImagesBin {
        std::vector<uint8_t> logo;
        std::vector<ProjectImagesBin> projectImages;
        BetaProjectImagesBin beta;
    };

    struct StateImagesBin {
        std::map<std::string, GroupDataImagesBin> groupImages;
    };

    //////////////////////////

    struct ProjectImages {
        std::shared_ptr<Image> backgroundImage;
        std::optional<std::shared_ptr<Image> > pageBackgroundImage;
    };

    struct BetaProjectImages {
        std::shared_ptr<Image> background;
    };

    struct GroupDataImages {
        std::shared_ptr<Image> logo;
        std::vector<ProjectImages> projectImages;
        BetaProjectImages beta;
    };

    struct StateImages {
        std::map<std::string, GroupDataImages> groupImages;
    };

    class MainState : public PageState {
    public:
        GroupDataState state;
        StateImages images;

        MainState(GroupDataState &state) : state(state) {
        }

        void PrintState() const override { std::cout << "MainState::PrintState()" << std::endl; }
    };

    inline StateImagesBin FetchAllImages(const GroupDataState &state) {
        StateImagesBin bin;
        std::vector<std::future<void> > futures;
        std::mutex bin_mutex;

        for (const auto &[group_key, group_data]: state.groups) {
            futures.push_back(std::async(std::launch::async, [&,group_key, group_data ] {
                GroupDataImagesBin group_bin;

                group_bin.logo = Image::FetchFromURL(group_data.logo);

                group_bin.projectImages.reserve(group_data.projects.size());
                for (const auto &project: group_data.projects) {
                    ProjectImagesBin project_bin;
                    project_bin.backgroundImage = Image::FetchFromURL(project.background);

                    if (project.pageBackground) {
                        project_bin.pageBackgroundImage = Image::FetchFromURL(*project.pageBackground);
                    }

                    group_bin.projectImages.push_back(std::move(project_bin));
                }
                group_bin.beta.background = Image::FetchFromURL(group_data.beta.background); {
                    std::lock_guard lock(bin_mutex);
                    bin.groupImages[group_key] = std::move(group_bin);
                }
            }));
        }
        for (auto &future: futures) {
            future.get();
        }
        return bin;
    }

    inline StateImages CreateVulkanImages(const StateImagesBin &bin) {
        StateImages images;

        for (const auto &[group_key, group_bin]: bin.groupImages) {
            GroupDataImages group_images;

            group_images.logo = Image::ConstructFromBin(group_bin.logo);

            group_images.projectImages.reserve(group_bin.projectImages.size());
            for (const auto &project_bin: group_bin.projectImages) {
                ProjectImages project_images;
                project_images.backgroundImage = Image::ConstructFromBin(project_bin.backgroundImage);

                if (project_bin.pageBackgroundImage) {
                    project_images.pageBackgroundImage = Image::ConstructFromBin(*project_bin.pageBackgroundImage);
                }

                group_images.projectImages.push_back(std::move(project_images));
            }

            group_images.beta.background = Image::ConstructFromBin(group_bin.beta.background);

            images.groupImages[group_key] = std::move(group_images);
        }
        return images;
    }

    inline void HandleImages(const GroupDataState &state, StateImages &images) {
        StateImagesBin bin = FetchAllImages(state);
        images = CreateVulkanImages(bin);
    }

    inline void fetch_and_decode_groups(std::shared_ptr<MainState> &thread_state_ptr) {
        const auto url = "https://github.com/infinity-MSFS/groups/raw/refs/heads/main/groups.bin";
        std::vector<uint8_t> received_data;

        CURL *curl = curl_easy_init();
        if (!curl) {
            throw std::runtime_error("Failed to initialize CURL");
        }

        CurlGuard guard(curl);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &received_data);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Infinity-MSFS-Client/1.0");

        if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
            throw std::runtime_error(std::string("Failed to fetch data: ") +
                                     curl_easy_strerror(res));
        }

        std::cout << "Fetched data" << received_data.size() << " bytes" << std::endl;

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200) {
            throw std::runtime_error("HTTP error: " + std::to_string(http_code));
        }
        GroupDataState state;
        state.groups = decode_bin(received_data);
        thread_state_ptr->state = state;

        StateImages images;
        HandleImages(thread_state_ptr->state, images);
        std::cout << "Decoded all images\n";
        thread_state_ptr->images = images;
    }


    inline ImVec4 hexToImVec4(const std::string &hexColor) {
        const std::string hex = (hexColor[0] == '#') ? hexColor.substr(1) : hexColor;

        if (hex.length() != 6 && hex.length() != 8) {
            throw std::invalid_argument("Invalid hex color format");
        }

        float alpha = 1.0f;

        unsigned int colorValue;
        std::stringstream ss;
        ss << std::hex << hex;
        ss >> colorValue;

        float r, g, b;

        if (hex.length() == 6) {
            r = ((colorValue >> 16) & 0xFF) / 255.0f;
            g = ((colorValue >> 8) & 0xFF) / 255.0f;
            b = (colorValue & 0xFF) / 255.0f;
        } else {
            r = ((colorValue >> 24) & 0xFF) / 255.0f;
            g = ((colorValue >> 16) & 0xFF) / 255.0f;
            b = ((colorValue >> 8) & 0xFF) / 255.0f;
            alpha = (colorValue & 0xFF) / 255.0f;
        }
        return {r, g, b, alpha};
    }
}
