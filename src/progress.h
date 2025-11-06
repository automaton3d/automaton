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
  int barWidths[3];
  int barX, barY, barWidth, barHeight;
  int pointerX;
  float progress;
};
