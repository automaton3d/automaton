/*
 * animator.cpp
 *
 * Helps to implement the states of the animation (roll, zoom etc.).
 */

#include "animator.h"

namespace framework
{

  Animator::Animator() :
    mAnimation_(NONE),
    mInteractor_(0),
    mFrame_(0),
    mFrames_(0),
    mFramesPerSecond_(30.),
    mHeight_(0),
    mWidth_(0)
  {
      stopwatch();
  }

  Animator::~Animator()
  {
  }

  void Animator::animate()
  {
    if (elapsedSeconds() < 1.0 / mFramesPerSecond_)
    {
      return;
    }
    switch(mAnimation_)
    {
      case FIRST_PERSON:
        firstperson();
        break;
      case ORBIT:
        orbit();
        break;
      case PAN:
        pan();
        break;
      case ROLL:
        roll();
        break;
      case ZOOM:
        zoom();
        break;
      case NONE: default:
        return;
    }
    stopwatch();
  }

  double Animator::elapsedSeconds()
  {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = now - mTic_;
    return seconds.count();
  }

  void Animator::firstperson()
  {
  }

  void Animator::orbit()
  {
    Camera *c = mInteractor_->getCamera();
    if (0 == mFrame_)
    {
      mFrames_ = 5 * mFramesPerSecond_;
      c->setEye(1., 1., 1.);
      c->setUp(-0., -0., 1.);
      c->setCenter(0, 0, 0);
      c->update();
      mInteractor_->setCamera(c);
    }
    double x = fmod(mFrame_*4, mWidth_);
    double y = mHeight_ * .74;
    mInteractor_->setLeftClicked(true);
    mInteractor_->setClickPoint(x, y);
    if (++mFrame_ >= mFrames_)
    {
      mInteractor_->setLeftClicked(false);
      reset();
    }
  }

  void Animator::pan()
  {
  }

  void Animator::reset()
  {
    mAnimation_ = NONE;
    mFrame_ = 0;
  }

  void Animator::roll()
  {
  }

  void Animator::setAnimation(AnimationType type)
  {
    mAnimation_ = type;
  }

  void Animator::setInteractor(TrackBallInteractor *i)
  {
    mInteractor_ = i;
  }

  void Animator::setScreenSize(int w, int h)
  {
    mWidth_ = w;
    mHeight_ = h;
  }

  void Animator::stopwatch()
  {
    mTic_ = std::chrono::system_clock::now();
  }

  void Animator::zoom()
  {
  }

}
