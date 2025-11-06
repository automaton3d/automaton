/*
 * animator.h
 *
 * Defines the states of the animation (roll, zoom etc.).
 */

#ifndef RSMZ_ANIMATOR_H
#define RSMZ_ANIMATOR_H

#include "trackball.h"
#include <chrono>

namespace framework
{

  class Animator
  {
  public:
    typedef enum AnimationType
    {
      NONE, FIRST_PERSON, ORBIT, PAN, ROLL, ZOOM
    } AnimationType;

    Animator();
    ~Animator();

    void animate();
    double elapsedSeconds();
    void firstperson();
    void orbit();
    void pan();
    void reset();
    void roll();
    void setAnimation(AnimationType type);
    void setScreenSize(int w, int h);
    void setInteractor(TrackBallInteractor *i);
    void stopwatch();
    void zoom();

  private:
    AnimationType mAnimation;
    TrackBallInteractor *mInteractor;
    int mFrame;
    int mFrames;
    float mFramesPerSecond;
    int mHeight;
    std::chrono::time_point<std::chrono::system_clock> mTic;
    int mWidth;
  };


}
#endif // RSMZ_ANIMATOR_H
