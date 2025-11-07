/*
 * progress.h
 */

#pragma once

class ProgressBar
{
public:
  ProgressBar(int screenWidth);
  void update(unsigned long long timer);
  void render();

private:
  int barWidths_[3];
  int barX_, barY_, barWidth_, barHeight_;
  int pointerX_;
  float progress_;
};
