// replay_progress.cpp
#include "replay_progress.h"
#include <GL/glut.h>
#include <text.h>
#include <cstdio>
#include <cstddef>

ReplayProgressBar::ReplayProgressBar(int screenWidth, int screenHeight)
{
  barHeight_ = 15;
  barWidth_ = screenWidth / 3;
  barX_ = (screenWidth - barWidth_) / 2;
  barY_ = screenHeight - 50; // Near bottom of screen

  progress_ = 0.0f;
  currentFrame_ = 0;
  totalFrames_ = 0;
}

void ReplayProgressBar::update(size_t currentFrame, size_t totalFrames)
{
  currentFrame_ = currentFrame;
  totalFrames_ = totalFrames;

  if (totalFrames > 0) {
    progress_ = static_cast<float>(currentFrame) / static_cast<float>(totalFrames);
  } else {
    progress_ = 0.0f;
  }
}

void ReplayProgressBar::render()
{
  if (totalFrames_ == 0) return;

  // Draw filled progress
  glColor3f(0.2f, 0.6f, 0.8f); // Cyan/blue color
  int filledWidth = static_cast<int>(progress_ * barWidth_);
  glBegin(GL_QUADS);
    glVertex2i(barX_, barY_);
    glVertex2i(barX_ + filledWidth, barY_);
    glVertex2i(barX_ + filledWidth, barY_ + barHeight_);
    glVertex2i(barX_, barY_ + barHeight_);
  glEnd();

  // Draw outline
  glColor3f(0.6f, 0.6f, 0.6f);
  glLineWidth(2);
  glBegin(GL_LINE_LOOP);
    glVertex2i(barX_, barY_);
    glVertex2i(barX_ + barWidth_, barY_);
    glVertex2i(barX_ + barWidth_, barY_ + barHeight_);
    glVertex2i(barX_, barY_ + barHeight_);
  glEnd();

  // Draw text: "Frame X / Y"
  char text[64];
  sprintf(text, "Frame %zu / %zu", currentFrame_, totalFrames_);
  glColor3f(1.0f, 1.0f, 1.0f);
  framework::drawString(text, barX_, barY_ - 10, 12);
}

void ReplayProgressBar::setPosition(int screenWidth, int screenHeight, int progressY)
{
    barWidth_ = screenWidth / 3;
    barX_ = (screenWidth - barWidth_) / 2;
    barY_ = progressY;   // use ProgressBar’s Y
}
