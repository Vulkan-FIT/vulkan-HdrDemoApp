#pragma once


#include <iostream>
#include <functional>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>



class HdaWindow {

public:

	// constructor
	HdaWindow(int, int, const char*);

	// destructor
	~HdaWindow();

	// delete copy construktor and copy operator 
	// (to avoid losing pointers to GLFW *window)
	HdaWindow(const HdaWindow&) = delete;
	HdaWindow& operator=(const HdaWindow&) = delete;


	inline bool glfwShouldClose() { return glfwWindowShouldClose(window); }
	inline GLFWwindow* getGLFWwindow() const { return window; }

	inline bool getFramebufferResizedFlag() { return framebufferResized; }
	inline void setFramebufferResizedFlag() { framebufferResized = false; }
	inline bool getKeyPressedXFlag() { return keyPressedX; }
	inline void setKeyPressedXFlag() { keyPressedX = false; }
	inline bool getKeyPressedMFlag() { return keyPressedM; }
	inline void setKeyPressedMFlag() { keyPressedM = false; }


	void createWindowSurface(vk::Instance, vk::SurfaceKHR *);

private:

	GLFWwindow* window;
	int winWidth;
	int winHeight;
	const char* winName;

	bool framebufferResized = false;
	bool keyPressedX = false;
	bool keyPressedM = false;

	vk::Instance instance;
	vk::PhysicalDevice physDev;
	vk::Device dev;
	vk::SurfaceKHR surf;

	static void framebufferCallback(GLFWwindow*, int, int);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	// initialization of main window
	void initWin();
};