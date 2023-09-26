#pragma once
#include"Core.h"
#include<list>
#include<string>
#include<vector>
#include<functional>
namespace ImGuiWrapper {

	class ImGuiListBox :public Component {
	public:
		using callback = std::function<void(const std::string&)>;
	private:
		std::vector<std::string> strs;
		std::vector<const char*> str_ptrs;
		callback callback_fun;
		std::string title;
	public:
		enum class PropertyTest {
			IsNull
		};
		//if need null str ,the title should be "##"
		ImGuiListBox(const std::string& title);
		virtual void update() override;
		void set_callback(callback);
		void push(const std::string&);
		void erase(const std::string&);
		void pop();
		void clear();
		void sort(std::function<bool(const std::string&, const std::string&)> base_on);
		bool test(PropertyTest);
		size_t size() { return strs.size(); }
	};
}