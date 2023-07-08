#include "Serialization.h"
#include"Dscp.h"
namespace LuRef {
	const Serialization* SerializationManager::FindSer(size_t hashCode) {
		auto it = serMap.find(hashCode);
		if (it == serMap.end())
			return nullptr;
		return &it->second;
	}

	const Serialization* SerializationManager::FindSer(const std::string& alias){
		auto t =  ClassAlias::Find(alias);
		auto it = serMap.find(*t);
		if (t == nullptr || it == serMap.end())
			return nullptr;
		return &it->second;
	}

	void SerializationManager::Serialize(YAML::Node& node, Object& obj)
	{
		auto it = serMap.find(obj.dscp.type.hashID);
		if (it == serMap.end())
			throw std::exception{"bad deserialize"};
		it->second.serialize(node, &obj);
	}

	void SerializationManager::Serialize(YAML::Node& node, SharedObject& obj){
		auto obj_ = obj.dscp.MakeWithData(obj.As<void>());
		Serialize(node, *obj_);
	}

	void SerializationManager::Deserialize(YAML::Node& node, Object& obj){
		auto it = serMap.find(obj.dscp.type.hashID);
		if (it == serMap.end())
			throw std::exception{"bad deserialize"};
		it->second.deserialize(node, &obj);
	}

	void SerializationManager::Deserialize(YAML::Node& node, SharedObject& obj){
		auto obj_ = obj.dscp.MakeWithData(obj.As<void>());
		Deserialize(node, *obj_);
	}

	void SerializationManager::PushSerialization(size_t hashCode, const Serialization ser){
		serMap.emplace(hashCode, ser);
	}
}

