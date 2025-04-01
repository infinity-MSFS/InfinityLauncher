#pragma once
#include <cmath>
#include <ctime>
#include <imgui.h>
#include <imgui_internal.h>
#include <random>
#include <vector>

namespace Infinity {
  struct Meteor {
    ImVec2 position;
    float speed{};
    float length{};
    float thickness{};
    ImColor start_color;
    ImColor end_color;

    explicit Meteor(const ImVec2 screen_size) { Respawn(screen_size); }

    static std::mt19937& GetRNG() {
      static std::random_device rd;
      static std::mt19937 gen(rd());
      return gen;
    }

    template<typename T>
    static T RandomRange(T min, T max) {
      std::uniform_real_distribution<T> dist(min, max);
      return dist(GetRNG());
    }

    void Respawn(ImVec2 screen_size);

    void Update(ImVec2 screen_size);

    void Render() const;
  };

  class Meteors {
public:
    static Meteors* GetInstance(const int count = 100) {
      static Meteors instance(count);
      return &instance;
    }

    void Update();

    void Render();

private:
    std::vector<Meteor> meteors;
    int count;

    explicit Meteors(int count);
  };
}  // namespace Infinity
