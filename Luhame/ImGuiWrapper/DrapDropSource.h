#pragma once
#include "Core.h"
#include<string>
#include<functional>
namespace ImGuiWrapper {
	template<class T>
	struct DragSourceData: public ::ImGuiPayload {
		T* get_source() { return static_cast<T>(Data); }
	};
	template<class T>
	class DrapDropTarget :public Component{
		using callback_fun_type = std::function<void(const DragSourceData<T>*)>;
		std::string flag;
		callback_fun_type callback;
	public:
		void set_callback(callback_fun_type callback_);
		void set_flag(const std::string& new_flag_);
		size_t get_type_id() { return typeid(T).hash_code(); }
		virtual void update();
	};

	template<class T>
	class DrapDropSource :public Component{
		std::string flag;
		T* data;
		size_t data_size;
	public:
		void set_data(T*, size_t size);
		void set_flag(const std::string& new_flag_);
		size_t get_type_id() { return typeid(T).hash_code(); }
		virtual void update();
	};

	template<class T>
	inline void DrapDropTarget<T>::set_callback(callback_fun_type callback_){
		callback = callback_;
	}

	template<class T>
	inline void DrapDropTarget<T>::set_flag(const std::string& new_flag_){
		flag = new_flag_;
	}
	template<class T>
	inline void DrapDropTarget<T>::update(){
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(flag.c_str())) {
				callback(static_cast<const DragSourceData<T>*>(payload));
			}
		}
	}
	/* data_数据项指针，size 数据字数
	   ImGui会对这段数据进行拷贝，所以内存大的数据最好存一个指针，但是得内存内存风险
	*/
	template<class T>
	inline void DrapDropSource<T>::set_data(T* data_,size_t size){
		data = data_;
		data_size = size;
	}

	template<class T>
	inline void DrapDropSource<T>::set_flag(const std::string& new_flag_){
		flag = new_flag_;
	}

	template<class T>
	inline void DrapDropSource<T>::update(){
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(flag.c_str(),data, data_size);
			ImGui::EndDragDropSource();
		}
	}

}

