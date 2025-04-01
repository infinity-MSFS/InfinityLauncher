#pragma once
#include "Frontend/Background/Meteors.hpp"

namespace Infinity {
  class Betas {
public:
    Betas() {}
    ~Betas() = default;

    void Render() {
      Meteors::GetInstance()->Update();
      Meteors::GetInstance()->Render();
    }

private:
  };

}  // namespace Infinity
