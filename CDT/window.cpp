//#include "simulation.h"
#include "window.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

namespace framework
{

RenderWindowGLFW::RenderWindowGLFW() :  mWindow(0)
{
}

RenderWindowGLFW::~RenderWindowGLFW()
{
}

void RenderWindowGLFW::buttonCallback(GLFWwindow *window, int button,
                                      int action, int mods)
{
    switch(action)
    {
        case GLFW_PRESS:
        {
            switch(button)
            {
                case GLFW_MOUSE_BUTTON_LEFT:
                    instance().mInteractor.setLeftClicked(true);
                    break;
                case GLFW_MOUSE_BUTTON_MIDDLE:
                    instance().mInteractor.setMiddleClicked(true);
                    break;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    instance().mInteractor.setRightClicked(true);
                    break;
            }

            double xpos, ypos;
            glfwGetCursorPos(window, & xpos, & ypos);
            instance().mInteractor.setClickPoint(xpos, ypos);
            break;
        }
        case GLFW_RELEASE:
        {
            switch(button)
            {
                case GLFW_MOUSE_BUTTON_LEFT:
                    instance().mInteractor.setLeftClicked(false);
                    break;
                case GLFW_MOUSE_BUTTON_MIDDLE:
                    instance().mInteractor.setMiddleClicked(false);
                    break;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    instance().mInteractor.setRightClicked(false);
                    break;
            }
            break;
        }
        default: break;
    }
}

void RenderWindowGLFW::errorCallback(int error, const char* description)
{
    std::cerr << description << std::endl;
}

RenderWindowGLFW & RenderWindowGLFW::instance()
{
	static RenderWindowGLFW i;
	return i;
}

void RenderWindowGLFW::keyCallback(GLFWwindow *window, int key, int scancode,
                                   int action, int mods)
{
    float length;

    switch(action) {
        case GLFW_PRESS:
            switch(key)
            {
                case GLFW_KEY_ESCAPE:
                    // Exit app on ESC key.
                    glfwSetWindowShouldClose(window, GL_TRUE);
                    break;
                case GLFW_KEY_LEFT_CONTROL:
                case GLFW_KEY_RIGHT_CONTROL:
                    instance().mInteractor.setSpeed(5.f);
                    break;
                case GLFW_KEY_LEFT_SHIFT:
                case GLFW_KEY_RIGHT_SHIFT:
                    instance().mInteractor.setSpeed(.1f);
                    break;
                case GLFW_KEY_F1:
                    instance().mAnimator.setAnimation(Animator::ORBIT);
                    break;
                case GLFW_KEY_C:
                    std::cout
                        << "(" << instance().mCamera.getEye().x
                        << "," << instance().mCamera.getEye().y
                        << "," << instance().mCamera.getEye().z << ") "
                        << "(" << instance().mCamera.getCenter().x
                        << "," << instance().mCamera.getCenter().y
                        << "," << instance().mCamera.getCenter().z << ") "
                        << "(" << instance().mCamera.getUp().x
                        << "," << instance().mCamera.getUp().y
                        << "," << instance().mCamera.getUp().z  << ")\n";
                    break;
                case GLFW_KEY_R:
                    // Reset the view.
                    instance().mCamera.reset();
                    instance().mInteractor.setCamera(& instance().mCamera);
                    break;
                case GLFW_KEY_T:
                    // Toogle motion type.
                    if (instance().mInteractor.getMotionRightClick() ==
                            TrackBallInteractor::FIRSTPERSON) {
                        instance().mInteractor.setMotionRightClick(
                                TrackBallInteractor::PAN);
                    } else {
                        instance().mInteractor.setMotionRightClick(
                                TrackBallInteractor::FIRSTPERSON);
                    }
                    break;
                case GLFW_KEY_X:
                    // Snap view to axis.
                    length = glm::length(instance().mCamera.getEye() -
                                         instance().mCamera.getCenter());
                    instance().mCamera.setEye(length,0,0);
                    instance().mCamera.setUp(0,1,0);
                    instance().mCamera.update();
                    instance().mInteractor.setCamera(& instance().mCamera);
                    break;
                case GLFW_KEY_Y:
                    length = glm::length(instance().mCamera.getEye() -
                                         instance().mCamera.getCenter());
                    instance().mCamera.setEye(0,length,0);
                    instance().mCamera.setUp(1,0,0);
                    instance().mCamera.update();
                    instance().mInteractor.setCamera(& instance().mCamera);
                    break;
                case GLFW_KEY_Z:
                    length = glm::length(instance().mCamera.getEye() -
                                         instance().mCamera.getCenter());
                    instance().mCamera.setEye(0,0,length);
                    instance().mCamera.setUp(1,0,0);
                    instance().mCamera.update();
                    instance().mInteractor.setCamera(& instance().mCamera);
                    break;
                case GLFW_KEY_O:
                	// Isometric view
                	length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
                	instance().mCamera.setEye(length, length, length);
                	instance().mCamera.setUp(0, 1, 0);
                	instance().mCamera.update();
                	instance().mInteractor.setCamera(& instance().mCamera);
                	break;
                default: break;
            }
            break;
        case GLFW_RELEASE:
            switch(key)
            {
                case GLFW_KEY_LEFT_CONTROL:
                case GLFW_KEY_RIGHT_CONTROL:
                case GLFW_KEY_LEFT_SHIFT:
                case GLFW_KEY_RIGHT_SHIFT:
                    instance().mInteractor.setSpeed(1.f);
                    break;
            }
            break;
        default: break;
    }
}

void RenderWindowGLFW::moveCallback(GLFWwindow *window, double xpos,
                                    double ypos)
{
    instance().mInteractor.setClickPoint(xpos, ypos);
}

void RenderWindowGLFW::scrollCallback(GLFWwindow *window, double xpos,
                                      double ypos)
{
    instance().mInteractor.setScrollDirection(xpos + ypos > 0 ? true : false);
}

void RenderWindowGLFW::sizeCallback(GLFWwindow *window, int width, int height)
{
    instance().mRenderer.resize(width, height);
    instance().mInteractor.setScreenSize(width, height);
    instance().mAnimator.setScreenSize(width, height);
}

int RenderWindowGLFW::run(int width, int height)
{
    DWORD dwThreadId;
	HANDLE hSimulateThread = CreateThread(NULL, 0, automaton::SimulateThread, NULL, 0, &dwThreadId);
	CloseHandle(hSimulateThread);

    glfwSetErrorCallback(& RenderWindowGLFW::errorCallback);
    mWindow = glfwCreateWindow(width, height, "Cellular automaton", NULL, NULL);
    if (!mWindow)
    {
        glfwTerminate();
        return EXIT_FAILURE;
    }
//    std::cout << HELP;
    glfwMakeContextCurrent(mWindow);
    glfwSwapInterval(1);
    glfwSetCursorPosCallback(mWindow, & RenderWindowGLFW::moveCallback);
    glfwSetKeyCallback(mWindow, & RenderWindowGLFW::keyCallback);
    glfwSetMouseButtonCallback(mWindow, & RenderWindowGLFW::buttonCallback);
    glfwSetScrollCallback(mWindow, & RenderWindowGLFW::scrollCallback);
    glfwSetWindowSizeCallback(mWindow, &RenderWindowGLFW::sizeCallback);
    mInteractor.setCamera(& mCamera);
    mRenderer.setCamera(& mCamera);
    mAnimator.setInteractor(& mInteractor);
    mRenderer.init();
    sizeCallback(mWindow, width, height); // Set initial size.

	// Isometric view
	int length = glm::length(instance().mCamera.getEye() - instance().mCamera.getCenter());
	instance().mCamera.setEye(length, length, length);
	instance().mCamera.setUp(0, 1, 0);
	instance().mCamera.update();
	instance().mInteractor.setCamera(& instance().mCamera);

    while (!glfwWindowShouldClose(mWindow))
    {
        mAnimator.animate();
        mInteractor.update();
        mRenderer.render();
        glfwSwapBuffers(mWindow);
        glfwPollEvents();
    }
    glfwDestroyWindow(mWindow);
    glfwTerminate();
    return EXIT_SUCCESS;
}

} // end namespace rsmz
