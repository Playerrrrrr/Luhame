#pragma once
#include"Core.h"
#include<string>
#include"imgui.h"
namespace ImGuiWrapper {
	template<class T>
	concept float_or_integer = std::is_floating_point_v<T> || std::is_integral_v<T>;

	template<float_or_integer T,size_t N>
	class drag_vec :public Component
	{
	public:
		virtual void update() override;
		void push_vec(T* data) { this->data = data; };
		void config(float step, float min, float max) {
			this->step = step; this->min = min; this->max = max;
		};
	private:
		static const char* get_lable(int idx);
		template<size_t... nums>
		static const void init_lable(std::index_sequence<nums...>);
		T* data;
		float step = 1.0f, min = 0, max = 100;
		inline static std::string lables[N];
	};

	template<float_or_integer T ,size_t N>
	void 
		ImGuiWrapper::drag_vec<T,N>::update() {
		const static ImVec4 colors[] = { ImVec4{255,0,0,255},ImVec4{0,255,0,255},ImVec4{0,0,255,0} };
		for (int i = 0; i < N; ++i) {
			ImGui::PushID(data + i);
			ImVec2 textSize = ImGui::CalcTextSize(get_lable(i));  // 计算文本的尺寸
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();  // 获取光标位置
			if constexpr (N <= 3) {
				ImGui::GetWindowDrawList()->AddRectFilled(
					cursorPos, ImVec2(cursorPos.x + textSize.x + 5, cursorPos.y + textSize.y + 5),
					IM_COL32(colors[i].x, colors[i].y, colors[i].z, 255));  // 绘制填充的矩形作为文本的背景
			}
			else {
				ImGui::GetWindowDrawList()->AddRectFilled(
					cursorPos, ImVec2(cursorPos.x + textSize.x + 5, cursorPos.y + textSize.y + 5),
					IM_COL32(102, 153, 204, 255));  // 绘制填充的矩形作为文本的背景
			}
			ImGui::TextColored(ImVec4{ 0, 0, 0,255 }, get_lable(i));
			ImGui::SameLine();
			ImGui::PushItemWidth(40);
			if constexpr (std::is_integral_v<T>) {
				ImGui::DragInt("##", &data[i], step, min, max);
			}
			else {
				ImGui::DragFloat("##", &data[i], step, min, max, "%.2f");
			}
			ImGui::PopItemWidth();
			if (i != N-1)
				ImGui::SameLine();
			ImGui::PopID();
		}
	}
	template<float_or_integer T, size_t N>
	const char* 
		ImGuiWrapper::drag_vec<T, N>::get_lable(int idx)
	{
		//考虑是否使用std::call_once
		static bool is_init = false;
		if (!is_init) {
			init_lable(std::make_index_sequence<N>{});
			is_init = true;
		}
		if (idx < N)
			return lables[idx].c_str();
		throw std::exception{"the index out off the scope"};
	}
	template<float_or_integer T, size_t N>
	template<size_t... nums>
	const void
		drag_vec<T,N>::init_lable(std::index_sequence<nums...>)
	{
		if constexpr (N == 1) {
			lables[0] = "X";
		}
		else if constexpr (N == 2) {
			lables[0] = "X";
			lables[1] = "Y";
		}
		else if constexpr (N == 3) {
			lables[0] = "X";
			lables[1] = "Y";
			lables[2] = "Z";
		}
		else {
			(
				(lables[nums] = std::string{ "V" } + std::to_string(nums))
				, ...);
		}
	}
}

