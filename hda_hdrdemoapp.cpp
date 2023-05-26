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

		if (hdaAppWindow.getKeyPressedIFlag() == true) {
			showUsage();
			hdaAppWindow.setKeyPressedIFlag();
		}

		builder.render();
	}



	cout << "\nrunApp(): Application end.\n";


	// waiting for working done
	if (device.getDevice())
		device.getDevice().waitIdle();

}

void HdrDemoApp::showUsage() {

	cout << "\n\n- - -                 APP USAGE                  - - -\n";
	cout << "To list these keyboard controls in the console, press\n";
	cout << "the >I< key in the app whenever you need to.\n";
	cout << "\n";
	cout << "The most interesting key is X, start with it first\n";
	cout << "because it activates the HDR view.\n";
	cout << "\n";
	cout << "X	turn ON/OFF the HDR function\n";
	cout << "\n";
	cout << "W	turn the camera up\n";
	cout << "A	turn the camera to the left\n";
	cout << "S	turn the camera down\n";
	cout << "D	turn the camera to the right\n";
	cout << "E	move forward\n";
	cout << "Q	move backwards\n";
	cout << "\n";
	cout << "O	turn on the pointlights\n";
	cout << "\n";
	cout << "M	switch the TMO\n";
	cout << "\n";
	cout << "C	higher exposure\n";
	cout << "Y	lower exposure\n\n";
}
