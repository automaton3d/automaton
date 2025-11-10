/*
 * progress.cpp
 */
#include "progress.h"
#include "model/simulation.h"
#include <GL/glut.h>
#include <text.h>

using namespace automaton;

ProgressBar::ProgressBar(int screenWidth)
{
  // Default bar dimensions; will be updated on resize
  barHeight_ = 20;

  // Use global viewport width if available
  barWidth_ = screenWidth / 4;
  barX_ = (screenWidth - barWidth_) / 2;
  barY_ = 100;

  // Compute section widths based on simulation phase durations
  double totalRatio = static_cast<double>(FRAME);
  barWidths_[0] = static_cast<int>(barWidth_ * static_cast<double>(CONVOL) / totalRatio);
  barWidths_[1] = static_cast<int>(barWidth_ * static_cast<double>(DIFFUSION - CONVOL) / totalRatio);
  barWidths_[2] = static_cast<int>(barWidth_ * static_cast<double>(RELOC - DIFFUSION) / totalRatio);

  progress_ = 0.0f;
  pointerX_ = barX_;
}

void ProgressBar::update(unsigned long long timer)
{
  progress_ = static_cast<float>(timer % FRAME) / FRAME;
  pointerX_ = barX_ + static_cast<int>(progress_ * barWidth_);
}

void ProgressBar::render()
{
  std::string steps[3] = {"Convolution", "Diffusion", "Relocation"};
  int x0 = barX_ + barWidth_ + 22;
  int w = 20;
  int h = 5;
  int y0 = barY_ - 15;
  int accumulatedWidth = 0;

  for (int i = 0; i < 3; i++)
  {
    int sectionStart = accumulatedWidth;
    int sectionEnd = accumulatedWidth + barWidths_[i];

    if (progress_ > 0.01f)
    {
      switch (i)
      {
        case 0: glColor3f(0.3f, 0.3f, 0.0f); break;
        case 1: glColor3f(0.5f, 0.0f, 0.0f); break;
        case 2: glColor3f(0.0f, 0.5f, 0.0f); break;
      }
    }
    else
    {
      glColor3f(1, 1, 1);
    }

    glBegin(GL_QUADS);
      glVertex2i(barX_ + sectionStart, barY_);
      glVertex2i(barX_ + sectionEnd, barY_);
      glVertex2i(barX_ + sectionEnd, barY_ + barHeight_);
      glVertex2i(barX_ + sectionStart, barY_ + barHeight_);
    glEnd();

    glBegin(GL_TRIANGLES);
      glVertex2f(x0, y0);
      glVertex2f(x0 + w, y0);
      glVertex2f(x0 + w, y0 + h);
      glVertex2f(x0, y0);
      glVertex2f(x0 + w, y0 + h);
      glVertex2f(x0, y0 + h);
    glEnd();

    glColor3f(0.6f, 0.6f, 0.6f);
    framework::drawString(steps[i], x0 + w + 10, y0 + 7, 12);
    accumulatedWidth += barWidths_[i];
    y0 += 20;
  }

  glColor3f(0.6f, 0.6f, 0.6f);
  glLineWidth(2);
  glBegin(GL_LINE_LOOP);
    glVertex2i(barX_ - 1, barY_);
    glVertex2i(barX_ - 1 + barWidth_, barY_);
    glVertex2i(barX_ - 1 + barWidth_, barY_ + barHeight_);
    glVertex2i(barX_ - 1, barY_ + barHeight_);
  glEnd();

  glColor3f(1.0f, 1.0f, 0.0f);
  glBegin(GL_QUADS);
    glVertex2i(pointerX_ - 2, barY_ + 2);
    glVertex2i(pointerX_ + 2, barY_ + 2);
    glVertex2i(pointerX_ + 2, barY_ + barHeight_ - 2);
    glVertex2i(pointerX_ - 2, barY_ + barHeight_ - 2);
  glEnd();
}


