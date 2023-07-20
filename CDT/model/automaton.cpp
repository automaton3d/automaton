#include <windows.h>
#include "window.h"

namespace automaton
{
// UID

HWND front_chk, track_chk, p_chk, plane_chk, cube_chk, latt_chk, axes_chk;
HWND single_rad, partial_rad, full_rad, rand_rad;
HWND xy_rad, yz_rad, zx_rad, iso_rad;
HPEN xPen, yPen, zPen, boxPen;
HWND stopButton, suspendButton, centerButton;
boolean momentum, wavefront, single, partial, full, track, cube, plane, lattice, axes, xy, yz, zx, iso, rnd;

// Trackball

bool stop;
bool active = true;
unsigned long timer = 0;
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
    if (!glfwInit())
    {
        return EXIT_FAILURE;
    }
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    // Get the video mode of the primary monitor
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
    if (!videoMode)
    {
    	glfwTerminate();
    	return -1;
    }
	return framework::RenderWindowGLFW::instance().run(videoMode->width, videoMode->height);
}
