/*
 * animator.h (adapted)
 *
 * Defines the states of the animation (roll, zoom etc.).
 */

#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <chrono>

namespace framework
{

    enum AnimationType
    {
      //NONE,
      FIRST_PERSON,
      ORBIT,
      //PAN,
      //ROLL,
      //ZOOM
    };

    void animate() {}
    double elapsedSeconds() { return 0.0; }
    void firstperson() {}
    void orbit() {}
    void pan() {}
    void reset() {}
    void roll() {}
    void setAnimation(AnimationType type) {}
    void setScreenSize(int w, int h) {}
//    void setInteractor(TrackBallInteractor *i);
    void stopwatch() {}
    void zoom() {}

    AnimationType mAnimation_;
  //  TrackBallInteractor *mInteractor_;
    int mFrame_;
    int mFrames_;
    float mFramesPerSecond_;
 //   int mHeight_;
    std::chrono::time_point<std::chrono::system_clock> mTic_;
   // int mWidth_;

} // namespace framework

#endif // ANIMATOR_H
