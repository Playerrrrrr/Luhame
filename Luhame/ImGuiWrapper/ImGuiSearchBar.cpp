#include "ImGuiSearchBar.h"
#include"LuLog/lulog.hpp"
namespace ImGuiWrapper {
	ImGuiSearchBar::ImGuiSearchBar(const std::string& title)
		:title(title),list_box("##") {

		std::function<bool(const std::string&, const std::string&)> cmp =
			[](const std::string& a, const std::string& b)->bool {
			return a > b;
		};
		//输入字符串时的回调
		callback_std = [this, cmp](const char* data)->int {
			if (!std::strlen(data))
				return 0;
			list_box.clear();
			for (auto& t : str_cmp_map) {
				if (t.first.find(data) != std::string::npos) {
					list_box.push(t.first);
				}
			}
			list_box.sort(cmp);
			return 1;
		};
		//选择选项时的回调
		std::function<void(const std::string&)> callback_on_select =
			[this](const std::string& str)mutable {
			auto t = str_cmp_map.find(str);
			if (t == str_cmp_map.end()) {
				LU_CORE_ERROR("internal data error");
				return;
			}
			this->current_cmp = t->second;
		};
		list_box.set_callback(callback_on_select);
	}
	void ImGuiSearchBar::push_kv(const std::string& k, std::shared_ptr<Component> v){
		if (str_cmp_map.find(k) != str_cmp_map.end()) {
			LU_CORE_ERROR("insertion fail,the key already exists");
		}
		str_cmp_map.emplace(k, v);
	}
	void ImGuiSearchBar::update() {
		ImGui::PushID(this);
		if (ImGui::InputText("input", buffer.data(),
			buffer.size(), flag)) {
			callback_std(buffer.data());
		}
		if (!list_box.test(ImGuiListBox::PropertyTest::IsNull)) {
			ImGui::BeginChildFrame(1, ImVec2(500, 60));
			ImGui::BeginChild("child",{500,60 },true);
			list_box.update();
			ImGui::EndChild();
			ImGui::EndChildFrame();
		}
		if (current_cmp != nullptr)
			current_cmp->update();
		ImGui::PopID();
	}
	
}