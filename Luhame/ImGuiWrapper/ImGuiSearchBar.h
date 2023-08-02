#pragma once
#include"Core.h"
#include<map>
#include<string>
#include<memory>
#include<array>
#include"ImGuiWindow.h"
#include"ImGuiListBox.h"
namespace ImGuiWrapper {
	class ImGuiSearchBar:public Component{
		//这边应该写一个前缀树来优化一下
		std::map<std::string, std::shared_ptr<Component>> str_cmp_map;
		std::array<char, 256> buffer;
		std::shared_ptr<Component> current_cmp = nullptr;
		std::function<int(const char*)> callback_std;
		ImGuiListBox list_box;
		ImGuiInputTextFlags flag = ImGuiInputTextFlags_None;
		std::string title;
	public:
		
		ImGuiSearchBar(const std::string& title);
		void push_kv(const std::string&, std::shared_ptr<Component>);
		virtual void update() override;
		void erase(const std::string&);
	};
}

