#include "ImGuiWindow.h"
namespace ImGuiWrapper {
	ImGuiWindow::ImGuiWindow(const std::string& title, ImGuiWindowFlags flag)
		:flag(flag),title(title)
	{
	}
	void 
		ImGuiWindow::update(){
		ImGui::Begin(title.c_str(), nullptr, flag);
		for (auto& t : components)
			t->update();
		ImGui::End();
	}
	void 
		ImGuiWindow::push_component(std::shared_ptr<Component> cmp){
		components.push_back(cmp);
	}
	ImGuiChildWindow::ImGuiChildWindow(const std::string& title, glm::vec2 size,
		bool has_border, ImGuiWindowFlags flag)
		:title(title),size(size.x,size.y),has_border(has_border),flag(flag)
	{
	}
	void 
		ImGuiChildWindow::update()
	{
		ImGui::BeginChild(title.c_str(), size, has_border, flag);

		ImGui::EndChild();
	}
	void 
		ImGuiChildWindow::push_component(std::shared_ptr<Component> cmp){
		components.push_back(cmp);
	}
}