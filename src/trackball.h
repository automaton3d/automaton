/*
 * trackball.h
 *
 * Encapsulates the trackball routines.
 */

#ifndef TRACKBALL_H
#define TRACKBALL_H

#include "camera.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/quaternion.hpp>

namespace framework
{

  class TrackBallInteractor
  {
    public:
      typedef enum CameraMotionType
      {
        NONE, ARC, FIRSTPERSON, PAN, ROLL, ZOOM
      } CameraMotionType;

      static const glm::vec3 X, Y, Z;

      TrackBallInteractor();
      ~TrackBallInteractor();

      void computePointOnSphere(const glm::vec2 & point, glm::vec3 & result);
      void computeRotationBetweenVectors(const glm::vec3 & start,
                                           const glm::vec3 & stop,
                                           glm::quat & result);
      Camera* getCamera();
      CameraMotionType getMotionLeftClick();
      CameraMotionType getMotionMiddleClick();
      CameraMotionType getMotionRightClick();
      CameraMotionType getMotionScroll();
      void setScrollDirection(bool up);
      void setCamera(Camera *c);
      void setClickPoint(double x, double y);
      void setLeftClicked(bool value);
      void setMiddleClicked(bool value);
      void setMotionLeftClick(CameraMotionType motion);
      void setMotionMiddleClick(CameraMotionType motion);
      void setMotionRightClick(CameraMotionType motion);
      void setMotionScroll(CameraMotionType motion);
      void setRightClicked(bool value);
      void setScreenSize(float width, float height);
      void setSpeed(float s);
      void update();

    protected:
      char clickQuadrant(float x, float y);
        void computeCameraEye(glm::vec3 & eye);
        void computeCameraUp(glm::vec3 & up);
        void computePan(glm::vec3 & pan);
        void drag();
        void drag(bool isClicked, CameraMotionType motion);
        void dragArc();
        // Change eye position and up direction while keeping center point static.
        void dragArcCamera();
        void dragFirstPerson();
        void dragZoom();
        // Simulate zoom by moving camera along viewing eye direction.
        void dragZoomCamera();
        // Move camera focal center position with static up and eye direction.
        void dragPan();
        // Roll about eye direction.
        void rollCamera();
        void freezeTransform();
        void scroll();
        void updateCameraEyeUp(bool eye, bool up);

    private:
        Camera *mCamera_;
        CameraMotionType mCameraMotionLeftClick_;
        CameraMotionType mCameraMotionMiddleClick_;
        CameraMotionType mCameraMotionRightClick_;
        CameraMotionType mCameraMotionScroll_;
        glm::vec2 mClickPoint_;
        float mHeight_;
        bool mIsDragging_;
        bool mIsLeftClick_;
        bool mIsMiddleClick_;
        bool mIsRightClick_;
        bool mIsScrolling_;
        float mPanScale_;
        glm::vec2 mPrevClickPoint_;
        float mRollScale_;
        float mRollSum_;
        glm::quat mRotation_;
        glm::quat mRotationSum_;
        float mSpeed_;
        glm::vec3 mStartVector_;
        glm::vec3 mStopVector_;
        float mTranslateLength_;
        float mWidth_;
        float mZoomSum_;
        float mZoomScale_;

  }; // end class TrackBallInteractor

} // end namespace framework

#endif // TRACKBALL_H
