
#pragma once


class Downloads {
  public:
  explicit Downloads();

  void Render();

  private:
  void AnimatedProgressBar(float& progress, bool completed,
                           bool show_percentage = true,
                           float smoothness = 0.1f);
};
