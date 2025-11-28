/*
 * animator.cpp (adapted)
 *
 * Implements the states of the animation (roll, zoom etc.).
 */

#include "animator.h"
#include <cmath>   // for fmod

namespace framework
{

  Animator::Animator() :
    mAnimation_(NONE),
    mInteractor_(nullptr),
    mFrame_(0),
    mFrames_(0),
    mFramesPerSecond_(30.0f),
    mHeight_(0),
    mWidth_(0)
  {
    stopwatch();
  }

  Animator::~Animator() = default;

  void Animator::animate()
  {
    if (elapsedSeconds() < 1.0 / mFramesPerSecond_)
      return;

    switch(mAnimation_)
    {
      case FIRST_PERSON: firstperson(); break;
      case ORBIT:        orbit();       break;
      case PAN:          pan();         break;
      case ROLL:         roll();        break;
      case ZOOM:         zoom();        break;
      case NONE: default: return;
    }
    stopwatch();
  }

  double Animator::elapsedSeconds()
  {
    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = now - mTic_;
    return seconds.count();
  }

  void Animator::firstperson()
  {
    // TODO: implement first-person camera motion using GLM
  }

  void Animator::orbit()
  {
    Camera *c = mInteractor_->getCamera();
    if (mFrame_ == 0)
    {
      mFrames_ = static_cast<int>(5 * mFramesPerSecond_);
      c->setEye(1.f, 1.f, 1.f);
      c->setUp(0.f, 0.f, 1.f);
      c->setCenter(0.f, 0.f, 0.f);
      c->update();
      mInteractor_->setCamera(c);
    }

    double x = std::fmod(mFrame_ * 4.0, static_cast<double>(mWidth_));
    double y = mHeight_ * 0.74;
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
    // TODO: implement pan motion
  }

  void Animator::reset()
  {
    mAnimation_ = NONE;
    mFrame_ = 0;
  }

  void Animator::roll()
  {
    // TODO: implement roll motion
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
    // TODO: implement zoom motion
  }

} // namespace framework
