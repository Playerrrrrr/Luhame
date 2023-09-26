#include "ImGuiListBox.h"
#include"LuLog/lulog.hpp"
namespace ImGuiWrapper {
	ImGuiWrapper::ImGuiListBox::ImGuiListBox(const std::string& title)
		:title(title) {
		if (title.empty()) {
			this->title = "##";
		}
	}
	void ImGuiWrapper::ImGuiListBox::update() {
		int selectedIndex = -1;
		ImGui::PushID(this);
		if (ImGui::ListBox(title.c_str(), &selectedIndex, str_ptrs.data(), str_ptrs.size())) {
			if (selectedIndex >= 0 && callback_fun != nullptr) {
				callback_fun(strs[selectedIndex]);
			}
		}
		ImGui::PopID();
	}

	void ImGuiWrapper::ImGuiListBox::set_callback(callback callback_fun) {
		this->callback_fun = callback_fun;
	}

	void ImGuiWrapper::ImGuiListBox::push(const std::string& str) {
		for (auto& t : strs) {
			if (t == str) {
				LU_CORE_ERROR("insertion failed,the key already exist");
				return;
			}
		}
		strs.push_back(str);
		str_ptrs.push_back(str.c_str());
	}

	void ImGuiWrapper::ImGuiListBox::erase(const std::string& str) {
		for (int i = 0; i < strs.size(); i++) {
			if (strs[i] == str) {
				strs.erase(strs.begin() + i);
				str_ptrs.erase(str_ptrs.begin() + i);
				return;
			}
		}
		LU_CORE_ERROR("erasing failed,the key not exist");
	}

	void ImGuiWrapper::ImGuiListBox::pop() {
		if (strs.size() > 0) {
			strs.pop_back();
			str_ptrs.pop_back();
		}
	}

	void ImGuiWrapper::ImGuiListBox::clear() {
		strs.clear();
		str_ptrs.clear();
	}

	void ImGuiWrapper::ImGuiListBox::sort(std::function<bool(const std::string&, const std::string&)> base_on) {
		std::sort(strs.begin(), strs.end(), base_on);
		str_ptrs.clear();
		for (auto& t : strs) {
			str_ptrs.push_back(t.c_str());
		}
	}

	bool ImGuiWrapper::ImGuiListBox::test(PropertyTest eu) {
		switch (eu)
		{
		case PropertyTest::IsNull:
			return !size();
		default:
			break;
		}
		LU_CORE_ERROR("invaild PropertyTest enum");
		return false;
	}
}