#pragma once
#include"Core.h"
#include"LuLog/lulog.hpp"
#include<string>
#include<functional>
#include<vector>
#include<filesystem>
#include<fstream>
namespace ImGuiWrapper {
	template<class T>
	concept char_type = std::is_same_v<char, T> || std::is_same_v<wchar_t, T>;
	template<char_type T>
	class TextEditor: public Component{
		using callback_type = int(*)(ImGuiInputTextCallbackData*);
		using callback_fun_type = std::function<int(ImGuiInputTextCallbackData*)>;
		std::vector<T> str_buffer;
		std::string lable;
		ImGuiInputTextFlags flag;
		callback_type callback_invoker;
		ImVec2 size;
		std::filesystem::path path;
		static int call_back(ImGuiInputTextCallbackData* data);
	public:
		
		TextEditor(const std::string& lable, size_t buffer_size , const ImVec2& size,
			ImGuiInputTextFlags flags = 0, callback_type callback = 0);
		virtual void update();
		size_t get_len() {
			return std::strlen(str_buffer.data());
		}
		std::tuple<T*, size_t> data() { return std::tuple<T*>{str_buffer.data(), get_len()}; }
		void save();
		void set_targetpath(const std::filesystem::path& path) { this->path = path; }
		//find and hightlight
		void load(const std::filesystem::path& path);
	};



	template<char_type T>
	inline int TextEditor<T>::call_back(ImGuiInputTextCallbackData* data)
	{
		auto text_ptr = static_cast<TextEditor<T>*>(data->UserData);
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
			//当当前buffer不足以容纳text时，进行内存重分配
			if (data->BufTextLen + 1 >= text_ptr->str_buffer.size()) {
				text_ptr->str_buffer.resize(text_ptr->str_buffer.size() << 1);
				data->Buf = text_ptr->str_buffer.data();
				data->BufSize = text_ptr->str_buffer.size();
			}
		}
		return 1;
	}

	template<char_type T>
	TextEditor<T>::TextEditor<T>(const std::string& lable, size_t buffer_size, const ImVec2& size,
		ImGuiInputTextFlags flags, callback_type callback)
		:lable(lable), flag(
			flags | ImGuiInputTextFlags_AllowTabInput| ImGuiInputTextFlags_CallbackResize
		), callback_invoker(callback), size(size)
	{
		str_buffer.resize(buffer_size);
		if (this->callback_invoker == nullptr) {
			this->callback_invoker = &TextEditor<T>::call_back;
		}
	}
	template<char_type T>
	void TextEditor<T>::update() {
		if (ImGui::InputTextMultiline(lable.c_str(), str_buffer.data(), str_buffer.size()
			, size, flag, callback_invoker,this)) {
		}
	}
	template<char_type T>
	inline void TextEditor<T>::save(){
		if (!path.string().size()) {
			LU_CORE_WARN("the target path is null");
		}
		std::ofstream ofs(path);
		if (ofs.is_open()) {
			LU_CORE_ERROR("it arise fault when opening{}", path.string());
			return;
		}
		T* data = str_buffer.data();
		ofs << data;
		ofs.close();
	}
	template<char_type T>
	inline void TextEditor<T>::load(const std::filesystem::path& path){
		std::ifstream ifs(path);
		if (!ifs.is_open()) {
			LU_CORE_ERROR("it arise fault when opening{}", path.string());
			return;
		}
		std::stringstream sst;
		std::string line;
		sst.str("");
		while (std::getline(ifs, line)) {
			sst << line << '\n';
		}
		std::vector<T> vec{std::istreambuf_iterator<char>{sst}, {}};
		str_buffer = std::move(vec);
		ifs.close();
	}
}

