#pragma once
#include "hda_window.hpp"
#include "hda_instancegpu.hpp"
#include "hda_swapchain.hpp"
#include "hda_pipeline.hpp"
#include "builder.hpp"


#include <iostream>

const char win_name[] = "HDR Demo Application";
const int WIN_WIDTH = 1920;
const int WIN_HEIGHT = 1080;

/*
*
* Tøída reprezentující hlavní modul, který volá metodu Builder::render()
* pro vykreslení jednoho snímku.
*
*/

class HdrDemoApp {

public:

	HdrDemoApp() { cout << "HdrDemoApp(): constructor\n"; };
	~HdrDemoApp() { cout << "HdrDemoApp: Destructor\n"; };

	void runApp();

private:

	HdaWindow hdaAppWindow{ WIN_WIDTH, WIN_HEIGHT, win_name };

	HdaInstanceGpu device{ hdaAppWindow };

	HdaSwapchain swapchain{ device, WIN_WIDTH, WIN_HEIGHT };

	HdaPipeline pipeline{ device, swapchain };

	HdaBuilder builder{ device, hdaAppWindow, swapchain, pipeline };

};