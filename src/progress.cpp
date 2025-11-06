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
  barHeight = 20;

  // Use global viewport width if available
  barWidth = screenWidth / 4;
  barX = (screenWidth - barWidth) / 2;
  barY = 100;

  // Compute section widths based on simulation phase durations
  double totalRatio = static_cast<double>(FRAME);
  barWidths[0] = static_cast<int>(barWidth * static_cast<double>(CONVOL) / totalRatio);
  barWidths[1] = static_cast<int>(barWidth * static_cast<double>(DIFFUSION - CONVOL) / totalRatio);
  barWidths[2] = static_cast<int>(barWidth * static_cast<double>(RELOC - DIFFUSION) / totalRatio);

  progress = 0.0f;
  pointerX = barX;
}

void ProgressBar::update(unsigned long long timer)
{
  progress = static_cast<float>(timer % FRAME) / FRAME;
  pointerX = barX + static_cast<int>(progress * barWidth);
}

void ProgressBar::render()
{
  std::string steps[3] = {"Convolution", "Diffusion", "Relocation"};
  int x0 = barX + barWidth + 22;
  int w = 20;
  int h = 5;
  int y0 = barY - 15;
  int accumulatedWidth = 0;

  for (int i = 0; i < 3; i++)
  {
    int sectionStart = accumulatedWidth;
    int sectionEnd = accumulatedWidth + barWidths[i];

    if (progress > 0.01f)
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
      glVertex2i(barX + sectionStart, barY);
      glVertex2i(barX + sectionEnd, barY);
      glVertex2i(barX + sectionEnd, barY + barHeight);
      glVertex2i(barX + sectionStart, barY + barHeight);
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
    framework::drawString12(steps[i], x0 + w + 10, y0 + 7);
    accumulatedWidth += barWidths[i];
    y0 += 20;
  }

  glColor3f(0.6f, 0.6f, 0.6f);
  glLineWidth(2);
  glBegin(GL_LINE_LOOP);
    glVertex2i(barX - 1, barY);
    glVertex2i(barX - 1 + barWidth, barY);
    glVertex2i(barX - 1 + barWidth, barY + barHeight);
    glVertex2i(barX - 1, barY + barHeight);
  glEnd();

  glColor3f(1.0f, 1.0f, 0.0f);
  glBegin(GL_QUADS);
    glVertex2i(pointerX - 2, barY + 2);
    glVertex2i(pointerX + 2, barY + 2);
    glVertex2i(pointerX + 2, barY + barHeight - 2);
    glVertex2i(pointerX - 2, barY + barHeight - 2);
  glEnd();
}


