#include "hda_hdrdemoapp.hpp"

#include <iostream>
#include <stdexcept>

using namespace std;


int main(int /*argc*/, char** /*argv*/) {

	// catch exceptions
	// (vulkan.hpp functions throw if they fail)
	try {
		
		HdrDemoApp mainApp;
		mainApp.runApp();

	// catch exceptions
	} 
	catch(vk::Error& e) {

		cout << "[ERROR] Vulkan exception: " << e.what() << endl;
		cout << "\n-     PRESS ENTER FOR EXIT     -\n";
		cin.ignore();
		return EXIT_FAILURE; 
	}
	catch(exception& e) {

		cout << "[ERROR] Runtime exception: " << e.what() << endl;
		cout << "\n-     PRESS ENTER FOR EXIT     -\n";
		cin.ignore();
		return EXIT_FAILURE; 
	}
	catch(...) {

		cout << "[ERROR] Unspecified exception.\n";
		cout << "\n-     PRESS ENTER FOR EXIT     -\n";
		cin.ignore();
		return EXIT_FAILURE;
	}

	cout << "\n- Application end successfully -\n";
	cout << "\n-     PRESS ENTER FOR EXIT     -\n";
	cin.ignore();

	return EXIT_SUCCESS;
}
