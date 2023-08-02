#pragma once
#include"imgui.h"
namespace ImGuiWrapper {
	struct Component {
		virtual void update() = 0;
	};

}