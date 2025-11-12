// replay_progress.h
#pragma once

#include <cstddef>

class ReplayProgressBar
{
public:
  ReplayProgressBar(int screenWidth, int screenHeight);
  void update(size_t currentFrame, size_t totalFrames);
  void render();
  void setPosition(int screenWidth, int screenHeight, int progressY); // For window resize

private:
  int barX_, barY_, barWidth_, barHeight_;
  float progress_;
  size_t currentFrame_;
  size_t totalFrames_;
};
