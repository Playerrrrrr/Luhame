#pragma once
#include<unordered_set>
#include<functional>
#include<list>
#include<unordered_map>
#include<vector>
#include<queue>
#include<memory>
namespace LuCcs {
	/*
		设计原则：
			1：尽可能安全检查放置在与用户接触的接口上，底层代码提供查询
			   以供接口进行检测
			2：查询速度为主要目标
	
	*/
	template<typename...Args>
	struct Flag;


	using carrier_id = uint32_t;
	using component_id = size_t;
	template <typename T>
	T& get_null_t() { return *static_cast<T*>(nullptr); }
	class component_pool {
	public:
		void emplace(carrier_id, std::shared_ptr<void>);
		bool has(carrier_id id) { return pool.find(id) != pool.end(); }
		template<typename T>
		T& get(carrier_id);
		auto begin() { return pool.begin(); }
		auto end() { return pool.end(); }
		void remove(carrier_id);
	private:
		std::unordered_map<carrier_id, std::shared_ptr<void>> pool;
	};


	template<typename...Args>
	class viewport {
	private:
		using package = std::tuple<Args*...>;
	public:
		void foreach(std::function<void(package)>);
		void foreach(std::function<void(carrier_id, package)>);
	private:
		std::list<package> ps;
		inline static const constexpr uint32_t package_size = sizeof...(Args);
	};


	class registry {
	private:
		using callback = std::function<void(registry&,carrier_id)>;
	public:
		//维护component_pool
		template<class T>
		void register_component();//done

		//维护component_pool和carrier_info_table
		template<typename T,typename  ...Args>
		void emplace(carrier_id carrier, Args&&... args);//done
		
		template<typename ...Args, typename...Delete>
		viewport<Args...> view(Flag<Delete...>);
		template<typename ...Args,typename...Delete>
		void create_viewport(Flag<Delete...>);
		//仅仅是访问数据，不对内部数据进行修改
		template<typename T>
		bool has(carrier_id id);
		//all_of
		//one_of
		bool has_id(carrier_id id) { return carrier_info_map.find(id) != carrier_info_map.end(); }

		//仅仅是访问数据，不对内部数据进行修改
		template<typename T>
		T& get(carrier_id);//done

		//维护component_pool和carrier_info_table
		//假设调用destory时全部数据都是正确的
		void destory(carrier_id);

		//维护component_pool和carrier_info_table
		template<typename...Args>
		void remove(carrier_id);//done

		template<typename T>
		void on_construct(callback);
		template<typename T>
		void on_deconstruct(callback);

		carrier_id create() { 
			id_set.insert(++id_geneator);
			return id_geneator;
		}

		void check();

	private:
		template<typename T>
		bool has_pool() { return pool_table.find(typeid(T).hash_code()) != pool_table.end(); }
		void invoke_construct_callback(component_id com_id, carrier_id id);
		void invoke_deconstruct_callback(component_id com_id, carrier_id id);
		std::unordered_map<component_id, component_pool> pool_table;
		std::unordered_map<component_id, callback> callback_con_table;
		std::unordered_map<component_id, callback> callback_decon_table;
		std::unordered_map<carrier_id, std::list<component_id>> carrier_info_map;
		std::unordered_set<carrier_id> id_set;
		carrier_id id_geneator;
	};
	template<class T>
	inline void registry::register_component(){
		if (!has_pool<T>()) {
			pool_table.emplace(typeid(T).hash_code(), component_pool{});
		}
	}
	template<typename T, typename ...Args>
	inline void registry::emplace(carrier_id carrier, Args && ...args){
		if (!has_pool<T>()) {
			register_component<T>();
		}
		auto& it = pool_table.at(typeid(T).hash_code());

		if (it.has(carrier))//排除component_pool中已经存在该carrier的情况
			return;
		std::shared_ptr<void> ptr(new T{ args... });
		it.emplace(carrier, ptr);
		//维护carrier的信息
		if (!has_id(carrier))
			carrier_info_map.emplace(carrier, std::list<component_id>{});
		auto& c_info_t = carrier_info_map.at(carrier);
		//调用该
		c_info_t.push_back(typeid(T).hash_code());
		invoke_construct_callback(typeid(T).hash_code(), carrier);
	}

	template<typename T>
	inline bool registry::has(carrier_id id){
		if (!has_pool<T>()) {
			return false;
		}
		return pool_table.at(typeid(T).hash_code()).has(id);
	}
	template<typename T>
	inline T& registry::get(carrier_id id){
		if (!this->has_pool<T>())
			return get_null_t<T>();
		auto& it = pool_table.at(typeid(T).hash_code());
		if(!it.has(id))
			return get_null_t<T>();
		return it.get<T>(id);
	}
	template<typename ...Args>
	inline void registry::remove(carrier_id id){
		(
			[this, id]()mutable {
				if (!this->has_pool<Args>())
					return;
				auto& it = pool_table.at(typeid(Args).hash_code());
				if (it.has(id)) {
					it.remove(id);
					//调用remove回调函数
					invoke_deconstruct_callback(typeid(Args).hash_code(), id);
				}
			}()

		, ...);
		//维护信息
		auto it = carrier_info_map.find(id);
		if (it == carrier_info_map.end())
			return;
		auto& c_info_t = it->second;
		const static std::unordered_set<component_id> ids{typeid(Args).hash_code()...};
		std::queue<std::list<component_id>::iterator> its;
		for (auto t = c_info_t.begin(); t != c_info_t.end(); t++)
			if (ids.find(*t)!=ids.end()) {
				its.push(t);
			}
		while (!its.empty()) {
			c_info_t.erase(its.front());
			its.pop();
		}
	}
	template<typename T>
	inline void registry::on_construct(callback callbackFun){
		callback_con_table.emplace(typeid(T).hash_code(), callbackFun);
	}
	template<typename T>
	inline void registry::on_deconstruct(callback callbackFun){
		callback_decon_table.emplace(typeid(T).hash_code(), callbackFun);
	}
	template<typename T>
	inline T& component_pool::get(carrier_id id){
		auto t = pool.at(id);
		return *static_cast<T*>(t.get());
	}
}