/*
 * trackball.cpp
 *
 * Implements the trackball routines.
 */

#include <glm/gtx/norm.hpp> // length2
#include <glm/vec3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp> // pi
#include <glm/glm.hpp>
#include "trackball.h"

namespace framework
{

	const glm::vec3 TrackBallInteractor::X(1.f, 0.f, 0.f);
	const glm::vec3 TrackBallInteractor::Y(0.f, 1.f, 0.f);
	const glm::vec3 TrackBallInteractor::Z(0.f, 0.f, 1.f);

	TrackBallInteractor::TrackBallInteractor() :
	  mCamera_(NULL),
	  mCameraMotionLeftClick_(ARC),
	  mCameraMotionMiddleClick_(ROLL),
	  mCameraMotionRightClick_(FIRSTPERSON),
	  mCameraMotionScroll_(ZOOM),
	  mHeight_(1),
	  mIsDragging_(false),
	  mIsLeftClick_(false),
	  mIsMiddleClick_(false),
	  mIsRightClick_(false),
	  mIsScrolling_(false),
	  mPanScale_(.005f),
	  mRollScale_(.005f),
	  mRollSum_(0.f),
	  mRotation_(1.f, 0, 0, 0),
	  mRotationSum_(1.f, 0, 0, 0),
	  mSpeed_(1.f),
	  mTranslateLength_(0),
	  mWidth_(1),
	  mZoomSum_(0.f),
	  mZoomScale_(.1f)
	{
	}

	TrackBallInteractor::~TrackBallInteractor()
	{
	}

	char TrackBallInteractor::clickQuadrant(float x, float y)
	{
	    float halfw = .5 * mWidth_;
	    float halfh = .5 * mHeight_;

	    if (x > halfw) {
	        // Opengl image coordinates origin is upperleft.
	        if (y < halfh) {
	            return 1;
	        } else {
	            return 4;
	        }
	    } else {
	        if (y < halfh) {
	            return 2;
	        } else {
	            return 3;
	        }
	    }
	}

	void TrackBallInteractor::computeCameraEye(glm::vec3 & eye)
	{
	    glm::vec3 orientation = mRotationSum_ * Z;

	    if (mZoomSum_) {
	        mTranslateLength_ += mZoomScale_ * mZoomSum_;
	        mZoomSum_ = 0; // Freeze zooming after applying.
	    }

	    eye = mTranslateLength_ * orientation + mCamera_->getCenter();
	}

	void TrackBallInteractor::computeCameraUp(glm::vec3 & up)
	{
	    up = glm::normalize(mRotationSum_ * Y);
	}

	void TrackBallInteractor::computePan(glm::vec3 & pan)
	{
	    glm::vec2 click = mClickPoint_ - mPrevClickPoint_;
	    glm::vec3 look = mCamera_->getEye() - mCamera_->getCenter();
	    float length = glm::length(look);
	    glm::vec3 right = glm::normalize(mRotationSum_ * X);

	    pan = (mCamera_->getUp() * -click.y + right * click.x) *
	          mPanScale_ * mSpeed_ * length;
	}

	void TrackBallInteractor::computePointOnSphere(
	    const glm::vec2 & point, glm::vec3 & result)
	{
	    float x = (2.f * point.x - mWidth_) / mWidth_;
	    float y = (mHeight_ - 2.f * point.y) / mHeight_;
	    float length2 = x*x + y*y;
	    if (length2 <= .5)
	    {
	        result.z = sqrt(1.0 - length2);
	    }
	    else
	    {
	        result.z = 0.5 / sqrt(length2);
	    }
	    float norm = 1.0 / sqrt(length2 + result.z*result.z);
	    result.x = x * norm;
	    result.y = y * norm;
	    result.z *= norm;
	}

	void TrackBallInteractor::computeRotationBetweenVectors(
	    const glm::vec3 & u, const glm::vec3 & v, glm::quat & result)
	{
	    float cosTheta = glm::dot(u, v);
	    glm::vec3 rotationAxis;
	    static const float EPSILON = 1.0e-5f;

	    if (cosTheta < -1.0f + EPSILON)
	    {
	        // Parallel and opposite directions.
	        rotationAxis = glm::cross(glm::vec3(0.f, 0.f, 1.f), u);

	        if (glm::length2(rotationAxis) < 0.01) {
	            // Still parallel, retry.
	            rotationAxis = glm::cross(glm::vec3(1.f, 0.f, 0.f), u);
	        }

	        rotationAxis = glm::normalize(rotationAxis);
	        result = glm::angleAxis(180.0f, rotationAxis);
	    } else if (cosTheta > 1.0f - EPSILON)
	    {
	        // Parallel and same direction.
	        result = glm::quat(1, 0, 0, 0);
	        return;
	    } else
	    {
	        float theta = acos(cosTheta);
	        rotationAxis = glm::cross(u, v);

	        rotationAxis = glm::normalize(rotationAxis);
	        result = glm::angleAxis(theta * mSpeed_, rotationAxis);
	    }
	}

	void TrackBallInteractor::drag()
	{
	    if (mPrevClickPoint_ == mClickPoint_)
	    {
	        // Not moving during drag state, so skip unnecessary processing.
	        return;
	    }
	    computePointOnSphere(mClickPoint_, mStopVector_);
	    computeRotationBetweenVectors(mStartVector_,
	                                  mStopVector_,
	                                  mRotation_);
	    // Reverse so scene moves with cursor and not away due to camera model.
	    mRotation_ = glm::inverse(mRotation_);

	    drag(mIsLeftClick_, mCameraMotionLeftClick_);
	    drag(mIsMiddleClick_, mCameraMotionMiddleClick_);
	    drag(mIsRightClick_, mCameraMotionRightClick_);

	    // After applying drag, reset relative start state.
	    mPrevClickPoint_ = mClickPoint_;
	    mStartVector_ = mStopVector_;
	}

	void TrackBallInteractor::drag(bool isClicked, CameraMotionType motion)
	{
	    if (!isClicked)
	    {
	        return;
	    }
	    switch(motion)
	    {
	        case ARC:
	            dragArc();
	            break;
	        case FIRSTPERSON:
	            dragFirstPerson();
	            break;
	        case PAN:
	            dragPan();
	            break;
	        case ROLL:
	            rollCamera();
	            break;
	        case ZOOM:
	            dragZoom();
	            break;
	        default: break;
	    }
	}

	void TrackBallInteractor::dragArc()
	{
	    mRotationSum_ *= mRotation_; // Accumulate quaternions.
	    updateCameraEyeUp(true, true);
	}

	void TrackBallInteractor::dragFirstPerson()
	{
	    glm::vec3 pan;
	    computePan(pan);
	    mCamera_->setCenter(pan + mCamera_->getCenter());
	    mCamera_->update();
	    freezeTransform();
	}

	void TrackBallInteractor::dragPan()
	{
	    glm::vec3 pan;
	    computePan(pan);
	    mCamera_->setCenter(pan + mCamera_->getCenter());
	    mCamera_->setEye(pan + mCamera_->getEye());
	    mCamera_->update();
	    freezeTransform();
	}

	void TrackBallInteractor::dragZoom()
	{
	    glm::vec2 dir = mClickPoint_ - mPrevClickPoint_;
	    float ax = fabs(dir.x);
	    float ay = fabs(dir.y);

	    if (ay >= ax) {
	        setScrollDirection(dir.y <= 0);
	    } else {
	        setScrollDirection(dir.x <= 0);
	    }
	    updateCameraEyeUp(true, false);
	}

	void TrackBallInteractor::freezeTransform()
	{
	    if (mCamera_)
	    {
	        // Opengl is ZYX order.
	        // Flip orientation to rotate scene with sticky cursor.
	        mRotationSum_ = glm::inverse(glm::quat(mCamera_->getMatrix()));
	        mTranslateLength_ = glm::length(mCamera_->getEye()-mCamera_->getCenter());
	    }
	}

	Camera* TrackBallInteractor::getCamera()
	{
	    return mCamera_;
	}

	TrackBallInteractor::CameraMotionType TrackBallInteractor::getMotionLeftClick()
	{
	    return mCameraMotionLeftClick_;
	}

	TrackBallInteractor::CameraMotionType TrackBallInteractor::getMotionMiddleClick()
	{
	    return mCameraMotionMiddleClick_;
	}

	TrackBallInteractor::CameraMotionType TrackBallInteractor::getMotionRightClick()
	{
	    return mCameraMotionRightClick_;
	}

	TrackBallInteractor::CameraMotionType TrackBallInteractor::getMotionScroll()
	{
	    return mCameraMotionScroll_;
	}

	void TrackBallInteractor::rollCamera()
	{
	    glm::vec2 delta = mClickPoint_ - mPrevClickPoint_;
	    char quad = clickQuadrant(mClickPoint_.x, mClickPoint_.y);
	    switch (quad) {
	        case 1:
	            delta.y = -delta.y;
	            delta.x = -delta.x;
	            break;
	        case 2:
	            delta.x = -delta.x;
	            break;
	        case 3:
	            break;
	        case 4:
	            delta.y = -delta.y;
	            break;
	        default:
	            break;
	    }
	    glm::vec3 axis = glm::normalize(mCamera_->getCenter() - mCamera_->getEye());
	    float angle = mRollScale_ * mSpeed_ * (delta.x + delta.y + mRollSum_);
	    glm::quat rot = glm::angleAxis(angle, axis);
	    mCamera_->setUp(rot * mCamera_->getUp());
	    mCamera_->update();
	    freezeTransform();
	    mRollSum_ = 0;
	}

	void TrackBallInteractor::scroll()
	{
	    switch(mCameraMotionScroll_)
	    {
	        case ROLL:
	            rollCamera();
	            break;
	        case ZOOM:
	            updateCameraEyeUp(true, false);
	            break;
	        default: break;
	    }
	}

	void TrackBallInteractor::setCamera(Camera *c)
	{
	    mCamera_ = c;
	    freezeTransform();
	}

	void TrackBallInteractor::setClickPoint(double x, double y)
	{
		if(x > 100 && x < 1680)
		{
			mPrevClickPoint_ = mClickPoint_;
			mClickPoint_.x = x;
			mClickPoint_.y = y;
		}
	}

	void TrackBallInteractor::setLeftClicked(bool value)
	{
	  mIsLeftClick_ = value;
	}

	void TrackBallInteractor::setMiddleClicked(bool value)
	{
	    mIsMiddleClick_ = value;
	}

	void TrackBallInteractor::setMotionLeftClick(CameraMotionType motion)
	{
	    mCameraMotionLeftClick_ = motion;
	}

	void TrackBallInteractor::setMotionMiddleClick(CameraMotionType motion)
	{
	    mCameraMotionMiddleClick_ = motion;
	}

	void TrackBallInteractor::setMotionRightClick(CameraMotionType motion)
	{
	    mCameraMotionRightClick_ = motion;
	}

	void TrackBallInteractor::setMotionScroll(CameraMotionType motion)
	{
	    mCameraMotionScroll_ = motion;
	}

	void TrackBallInteractor::setRightClicked(bool value)
	{
	  mIsRightClick_ = value;
	}

	void TrackBallInteractor::setScreenSize(float width, float height)
	{
	    if (width > 1 && height > 1)
	    {
	        mWidth_ = width;
	        mHeight_ = height;
	    }
	}

	void TrackBallInteractor::setScrollDirection(bool up)
	{
	    mIsScrolling_ = true;
	    float inc = mSpeed_ * (up ? -1.f : 1.f);
	    mZoomSum_ += inc;
	    mRollSum_ += inc;
	}

	void TrackBallInteractor::setSpeed(float s)
	{
	    mSpeed_ = s;
	}

	void TrackBallInteractor::update()
	{
	    // Prevent camera interaction if a slider is being dragged
	    const bool isClick = mIsLeftClick_ || mIsMiddleClick_ || mIsRightClick_;
	    if (!mIsDragging_)
	    {
	        if (isClick)
	        {
	            mIsDragging_ = true;
	            computePointOnSphere(mClickPoint_, mStartVector_);
	        }
	        else if (mIsScrolling_)
	        {
	            scroll();
	            mIsScrolling_ = false;
	        }
	    }
	    else
	    {
	        if (isClick)
	        {
	            drag();
	        }
	        else
	        {
	            mIsDragging_ = false;
	        }
	    }
	}

	void TrackBallInteractor::updateCameraEyeUp(bool eye, bool up)
	{
	    if (eye)
	    {
	        glm::vec3 eye;
	        computeCameraEye(eye);
	        mCamera_->setEye(eye);
	    }
	    if (up) {
	        glm::vec3 up;
	        computeCameraUp(up);
	        mCamera_->setUp(up);
	    }
	    mCamera_->update();
	}

}
