#include "hda_window.hpp"


#include <string>



// definition of constructor 
HdaWindow::HdaWindow(int width, int height, const char* name) : winWidth(width), winHeight(height), winName(name) {
	
	initWin();
}

// destructor
HdaWindow::~HdaWindow() {

	std::cout << "HdaWindow: Destructor\n";
	glfwDestroyWindow(window);
	glfwTerminate();
}


// glfw initialization and setup
void HdaWindow::initWin() {

	glfwInit();

	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(winWidth, winHeight, winName, nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferCallback);
	glfwSetKeyCallback(window, keyCallback);
}

void HdaWindow::framebufferCallback(GLFWwindow* window, int width, int height) {

	auto hdaWin = reinterpret_cast<HdaWindow*>(glfwGetWindowUserPointer(window));
	hdaWin->framebufferResized = true;
}

void HdaWindow::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	
	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		
		auto hdaWin = reinterpret_cast<HdaWindow*>(glfwGetWindowUserPointer(window));
		hdaWin->keyPressedX = true;
	}

	if (key == GLFW_KEY_M && action == GLFW_PRESS) {

		auto hdaWin = reinterpret_cast<HdaWindow*>(glfwGetWindowUserPointer(window));
		hdaWin->keyPressedM = true;
	}

	if (key == GLFW_KEY_I && action == GLFW_PRESS) {

		auto hdaWin = reinterpret_cast<HdaWindow*>(glfwGetWindowUserPointer(window));
		hdaWin->keyPressedI = true;
	}
}

void HdaWindow::createWindowSurface(vk::Instance instance, vk::SurfaceKHR  *winSurface) {

	// convert vk::Surface to VkSurface because glfwCreateWindowSurface() requiared it
	auto vkSurface = VkSurfaceKHR(*winSurface);
	if (glfwCreateWindowSurface(instance, window, nullptr, &vkSurface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

	// convert back
	*winSurface = vk::SurfaceKHR(vkSurface);
}
