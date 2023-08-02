#pragma once
#include"Core.h"
#include<list>
#include<memory>
#include<string>
#include<glm/glm.hpp>

namespace ImGuiWrapper {
	class ImGuiWindow :public Component
	{
		std::list<std::shared_ptr<Component>> components;
		std::string title;
		ImGuiWindowFlags flag = ImGuiWindowFlags_None;
	public:
		ImGuiWindow(const std::string& title, ImGuiWindowFlags flag = ImGuiWindowFlags_None);
		virtual void update() override;
		void push_component(std::shared_ptr<Component> cmp);
	};
	class ImGuiChildWindow :public Component
	{
		std::list<std::shared_ptr<Component>> components;
		std::string title;
		ImGuiWindowFlags flag = ImGuiWindowFlags_None;
		ImVec2 size;
		bool has_border = true;
	public:
		ImGuiChildWindow(const std::string& title, glm::vec2 size = {100,200}, 
			bool has_border = true, ImGuiWindowFlags flag = ImGuiWindowFlags_None);
		virtual void update() override;
		void push_component(std::shared_ptr<Component> cmp);
	};
}

