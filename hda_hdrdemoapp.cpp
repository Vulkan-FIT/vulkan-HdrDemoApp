#include "hda_hdrdemoapp.hpp"

using namespace std;

void HdrDemoApp::runApp() {

	device.initGpu();
	swapchain.initSwapchain();
	pipeline.initPipeline();
	builder.initBuilder();

	cout << "\n\nrunApp(): Application started >> >> >>\n";

	//HdaWindow mainWindow
	// main loop of application, here the images are rendered one by one
	while (!hdaAppWindow.glfwShouldClose()) {
		
		glfwPollEvents();
		
		builder.render();
	}



	cout << "\nrunApp(): Application end.\n";


	// waiting for working done
	if (device.getDevice())
		device.getDevice().waitIdle();

}
