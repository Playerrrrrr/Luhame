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

		���л�ϵͳ����

		1��ʹ��yaml-cpp��
		2������yaml-cpp��Դ�룬ֻҪһ������ȷ��ʵ����YAML::convert<>ģ�壬
		   ��ô�Ϳ�����yamp-cpp��������л�
		3��ʵ�ֲ���Ҫ�����ע����Ϣʵ�ֹ㷺�����л�

		ԭ��1����yaml-cpp����жԽӣ��ݹ�ؽ��������л��Ķ���ֱ��������
			     �ɻ������ͣ�����������yaml-cpp���Ѿ�֧�����л�
			  2�����÷���ϵͳ���Ա����ֶε����ԣ��Դ������б����ͽ���������
				 ���ֶ�
			  3����YAML::convert<T>��װ��std::function<void(Node&,const void*)>��
				 std::function<void(const Node&,void*)>,ǰ����convert<T>::encode
				 ������convert<T>::decode
			  4������yaml-cpp��Դ�룬����YAML::convertû��֧�ֵ��Զ�������TҲ��
				 convert::encode��convert::decode,��Խӵ��洢ת��������
				 SerializationManager��

		���裺1���ڹ���һ����ķ����л�������ʱ�����Զ������͵ķ����л�����
			     �Ѿ���������
			  2���������Ͷ����ɻ���������ɵ�

		ʱ��[���û��ǰ��֮��]��
			  1����ClassBuilder::PushField<C T::*>��ʱ������ģ�彫convert<T*>
			     ��ClassBuilder<DT>::~ClassBuilder()ʱ��convert<DT>���з�װ

		ģ�ͣ�����һ�������ģ��Ϊ
				struct X{
					x1; x2; y1; x3; y2 ....
					xi(1.2.3...n)
					yi(1,2,3...n)
				}
				����xi�ǲ��ɱ�yaml-cpp�Դ���convert<>���л�������
				yi�ǿ��Ա�yaml-cpp�Դ���convert<>���л������ͣ�Ȼ��
				���ݼ���2���������Ͷ��ǻ������͵ļ��Σ����Զ�������
				һ��xi�У�
				struct xi{
					x1; x2; y1; x3; y2 ....
					xi(1.2.3...n)
					yi(1,2,3...n)
				}
				�����ݹ���ȥ�����п����ҵ��������Ͷ��ǿ��Ա����л���
				���������Թ���һ����
							x1
						   /  \
						  x2   y1
						 / \
						y2  y3
					���ǴӸ��ڵ���������л�ÿһ���ֶΣ��Ϳ��Եõ�
					
				������л�x1�����ѷ��֣�������ϵ��ӽڵ�ȫ��y
				Ȼ�����Ƕ�x�ڵ����ϸ��һ���ǶԷ����������[LuRef::
				
				Object]���б�����һֱ���Ƕ�std�������б�����
				��convert<T>::encode�����ӣ��䱻��װ��
				void(node,const void* data),����ǰ�ߣ�����Ӧ��data��
				�ڴ���Object�������������ں��ߣ�Ӧ����T���ͽ���������
				ΪʲôҪ�ֿ������أ���Ϊǰ���Ƿ���ע��������ͣ�����
				�������õ�Field����б�����������ͨ����׼��ĵ�������
				�б��������yaml���Ѿ��кܺõ�֧���ˣ�ǰ�߽ڵ���ҳ�Ϊ
				x�ڵ㣬����Ϊy�ڵ�
				��߾ͻ���һ�������ˣ��Ǿ������x�ڵ��ڰ���y�ڵ㣬��ô
				������һ���ڵ��data����ԭ��ָ�룬�����y�ڵ��а���x��
				����ô������һ���ڵ�ľ���Objectָ��
				���������ӣ�

				/o   ��ʾ����Objectָ��
				/p   ��ʾ����ԭ��ָ��

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
				����C obj����˵��obj�����л���Ϊ��

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

	//��߷���һ���µ�trait��std::convertible_to������ܲ���ֱ�ӸĽ�����ϵͳ�Ĵ���

	template<typename T>
	concept yaml_not_convert = requires(const T & a) {
		{YAML::convert<T>::flag()}->std::convertible_to<bool>;
	};

	//�ж�����T�Ƿ�yaml������
	template<class T>
	struct is_imp_yaml {
		static const constexpr bool value = !yaml_not_convert<T>;
	};

	
	template<class T>
	struct is_imp_by_yaml {
		static const constexpr bool value =
			is_imp_yaml<T>::value;
	};

	//�����ǽ��������Ͳ��
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

//yaml-cpp���Լ�ʵ���˴󲿷ֵ�
