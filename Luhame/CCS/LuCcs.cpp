#include "LuCcs.h"
#include"LuLog/lulog.hpp"
namespace LuCcs {
	void LuCcs::registry::destory(carrier_id id){
		auto it  = carrier_info_map.find(id);
		if (it == carrier_info_map.end())
			return;
		for (auto t : it->second) {
			pool_table.at(t).remove(id);
			//调用remove回调函数
			invoke_deconstruct_callback(t, id);
		}
		carrier_info_map.erase(id);
	}

	//检测数据是否正确
	void registry::check(){
		bool flag = true;
		for (auto info_t : carrier_info_map) {
			for (auto compid_t : info_t.second) {
				if (pool_table.find(compid_t) == pool_table.end()) {
					LU_CORE_ERROR("the component exist in the carrier_info_map, but not in pool_table");
					flag = false;
				}else if (pool_table.find(compid_t)->second.has(info_t.first)) {
					LU_CORE_ERROR("the carrier recode the component,but the component not recode the carrier");
					flag = false;
				}
			}
		}
		for (auto compool_t : pool_table) {
			for (auto car_t : compool_t.second) {
				if (carrier_info_map.find(car_t.first) == carrier_info_map.end()) {
					LU_CORE_ERROR("the carrier exist in the component_pool, but not in carrier_info_map");
					flag = false;
				}
				else {
					auto coms_t = carrier_info_map.find(car_t.first)->second;
					bool recode = false;
					for (auto t : coms_t) {
						if (t == compool_t.first)
							recode = true;
					}
					if (!recode) {
						LU_CORE_ERROR("the component_pool recode the carrier,but carrier not recode component");
						flag = false;
					}
				}
			}
		}
		if (flag == true)
			LU_CORE_INFO("the ccs is healthy");
	}

	void registry::invoke_construct_callback(component_id com_id,carrier_id id){
		auto con_fun_t = callback_con_table.find(com_id);
		if (con_fun_t == callback_con_table.end())
			return;
		con_fun_t->second(*this, id);
	}

	void registry::invoke_deconstruct_callback(component_id com_id, carrier_id id)
	{
	}

	void component_pool::emplace(carrier_id id, std::shared_ptr<void> data){
		pool.emplace(id, data);
	}

	void LuCcs::component_pool::remove(carrier_id id){
		pool.erase(id);
	}
}