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



/*

		序列化系统概述

		1：使用yaml-cpp库
		2：看了yaml-cpp的源码，只要一个类正确地实现了YAML::convert<>模板，
		   那么就可以用yamp-cpp库进行序列化
		3：实现不需要额外的注册信息实现广泛的序列化

		原理：1：与yaml-cpp库进行对接，递归地解析被序列化的对象，直到被解析
			     成基本类型，而基本类型yaml-cpp库已经支持序列化
			  2：利用反射系统可以遍历字段的特性，以此来进行遍历和解析对象中
				 的字段
			  3：将YAML::convert<T>封装成std::function<void(Node&,const void*)>和
				 std::function<void(const Node&,void*)>,前者是convert<T>::encode
				 后者是convert<T>::decode
			  4：改了yaml-cpp的源码，让其YAML::convert没有支持的自定义类型T也有
				 convert::encode和convert::decode,其对接到存储转换函数的
				 SerializationManager中

		假设：1：在构建一个类的反序列化函数的时候，其自定义类型的反序列化函数
			     已经构建好了
			  2：所有类型都是由基本类型组成的

		时机[序号没有前后之分]：
			  1：在ClassBuilder::PushField<C T::*>的时候利用模板将convert<T*>
			     在ClassBuilder<DT>::~ClassBuilder()时对convert<DT>进行封装

		模型：定义一个对象的模型为
				struct X{
					x1; x2; y1; x3; y2 ....
					xi(1.2.3...n)
					yi(1,2,3...n)
				}
				其中xi是不可被yaml-cpp自带的convert<>序列化的类型
				yi是可以被yaml-cpp自带的convert<>序列化的类型，然后
				根据假设2，所有类型都是基本类型的几何，所以对于任意
				一个xi有：
				struct xi{
					x1; x2; y1; x3; y2 ....
					xi(1.2.3...n)
					yi(1,2,3...n)
				}
				这样递归下去，所有可以找到所有类型都是可以被序列化的
				，这样可以构成一颗树
							x1
						   /  \
						  x2   y1
						 / \
						y2  y3
					我们从根节点出发，序列化每一个字段，就可以得到
					
				完成序列化x1，不难发现，这棵树上的子节点全是y
				然后我们对x节点进行细分一种是对反射对象类型[LuRef::
				
				Object]进行遍历，一直是是对std容器进行遍历，
				拿convert<T>::encode举例子，其被封装成
				void(node,const void* data),对于前者，我们应该data的
				内存用Object进行描述，对于后者，应该用T类型进行描述，
				为什么要分开描述呢？因为前者是反射注册过的类型，我们
				可以用拿到Field类进行遍历，后者是通过标准库的迭代器进
				行遍历，这个yaml库已经有很好的支持了，前者节点姑且称为
				x节点，后者为y节点
				这边就会有一个问题了，那就是如果x节点在包含y节点，那么
				传入下一个节点的data就是原生指针，如果是y节点中包含x节
				点那么传入下一个节点的就是Object指针
				看下面例子：

				/o   表示传入Object指针
				/p   表示传入原生指针

				struct A1{
					int i;
					map<int,double> mapid;
				}
				struct A2{
					float f;
				}
				struct B{
					A1 a1;
					array<A2> arrA2;
				}
				struct C{
					string str;
					B b;
				}
				对于C obj，来说，obj的序列化树为：

							obj
						   /o  \p
						  /    str
						 b
					   /   \
					  /o    \p
					 a1     arrA2
					/p\p      \o
				   i   map   A2 in arrA2
				               | p   
							   f




*/
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
