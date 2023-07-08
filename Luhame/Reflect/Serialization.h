#pragma once
#include<string>
#include<functional>
#include<unordered_map>
#include"Alias.h"
#include<vector>
#include<unordered_map>
#include<map>
#include<queue>
#include<list>
#include"yaml-cpp/yaml.h"
#include"TypeID.h"
#include"Dscp.h"

namespace LuRef {
	class SharedObject;
	class Object;
	struct Serialization {
		std::function<void(YAML::Node&, const void* data)> serialize;
		std::function<bool(const YAML::Node&, void* data)> deserialize;
		size_t fieldCount;
	};

	class SerializationManager {
	public:
		static const Serialization* FindSer(size_t hashCode);
		static const Serialization* FindSer(const std::string& alias);
		static void Serialize(YAML::Node&, Object&);
		static void Serialize(YAML::Node&, SharedObject&);
		template<typename T>
		static void SerializeWithData(YAML::Node&, T*);
		static void Deserialize(YAML::Node&, Object&);
		static void Deserialize(YAML::Node&, SharedObject&);
		template<typename T>
		static void DeserializeWithData(YAML::Node&, T*);
	private:
		static void PushSerialization(size_t hashCode, Serialization);
		
		inline static std::unordered_map<size_t, Serialization> serMap;

		template<typename DT, typename ...Deriveds>
		friend class ClassBuilder;
	};

	//这边发现一个新的trait，std::convertible_to，这边能不能直接改进反射系统的传参

	template<typename T>
	concept yaml_not_convert = requires(const T & a) {
		{YAML::convert<T>::flag()}->std::convertible_to<bool>;
	};

	//判断类型T是否被yaml库重载
	template<class T>
	struct is_imp_yaml {
		static const constexpr bool value = !yaml_not_convert<T>;
	};

	
	template<class T>
	struct is_imp_by_yaml {
		static const constexpr bool value =
			is_imp_yaml<T>::value;
	};

	//下面是将复合类型拆解
	template <typename K, typename V, typename C, typename A>
	struct is_imp_by_yaml<std::map<K, V, C, A>> {
		static const constexpr bool value =
			is_imp_by_yaml<K>::value &&
			is_imp_by_yaml<V>::value;
	};

	template <typename K, typename V, typename H, typename P, typename A>
	struct is_imp_by_yaml<std::unordered_map<K, V, H, P, A>> {
		static const constexpr bool value =
			is_imp_by_yaml<K>::value &&
			is_imp_by_yaml<V>::value;
	};

	template <typename T, typename A>
	struct is_imp_by_yaml<std::vector<T,A>> {
		static const constexpr bool value =
			is_imp_by_yaml<T>::value;
	};

	template <typename T, typename A>
	struct is_imp_by_yaml<std::list<T, A>> {
		static const constexpr bool value =
			is_imp_by_yaml<T>::value;
	};

	template <typename T, std::size_t N>
	struct is_imp_by_yaml<std::array<T, N>> {
		static const constexpr bool value =
			is_imp_by_yaml<T>::value;
	};

	template <typename T>
	struct is_imp_by_yaml<std::valarray<T>> {
		static const constexpr bool value =
			is_imp_by_yaml<T>::value;
	};

	template <typename T, typename U>
	struct is_imp_by_yaml<std::pair<T, U>> {
		static const constexpr bool value =
			is_imp_by_yaml<T>::value;
	};

	template<typename T>
	inline void SerializationManager::SerializeWithData(YAML::Node& node, T* data){
		auto t = ClassManager::FindClass(*ClassAlias::Find(STypeID<T>::hashID))
			->MakeWithData(static_cast<void*>(data));
		Serialize(node,*t);
	}

	template<typename T>
	inline void SerializationManager::DeserializeWithData(YAML::Node& node, T* data){
		auto t = ClassManager::FindClass(*ClassAlias::Find(STypeID<T>::hashID))
			->MakeWithData(static_cast<void*>(data));
		Deserialize(node, *t);
	}

}

//yaml-cpp中自己实现了大部分的
