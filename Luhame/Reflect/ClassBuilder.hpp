#pragma once
#include"Dscp.h"
#include"Serialization.h"
namespace LuRef {
	/*
	*
	*                         引用折叠处理类型
	*                              |
	*                +-------------+-----------------+
					 |                               |
					 |                               |
			参数列表编译期类型                构建Method参数列表
					|                               |
					|                               |
					+------------运行时检测----------
									|
									|通过
									|
								std::apply调用

	*/
	/*
	所注册的信息如下

		假设：
			1：每一个函数都有不同的alias，以区别函数重载
			2：静态函数，且alias为construct的函数为构造器

		是否承认纯虚函数？ | yes

		1：注册类信息到ClassDscp，将其交给ClassManager
		2：注册字段信息到FieldDscp，将其交给FieldFlyWeight
		3：注册字段信息到MethDscp，将其交给MethodFlyWeight
		4：注册别名->hashID到ClassAlis
		5：注册hashID->别名到IDRegistry
		6：注册子类信息，并可查询保存
	*/
	template<typename DT, typename ...Deriveds>
	class ClassBuilder {
	public:
		/* 传入字段*/
		template<class C, class T>
		ClassBuilder<DT, Deriveds...>* PushField(const std::string& alias, T C::* ptr, bool isGlobal = false);
		/* 传入成员方法*/
		template<typename C, typename R, typename... Args>
		ClassBuilder<DT, Deriveds...>* PushMethod(const std::string& alias, R(C::* funptr)(Args...), bool isVirtual = false, bool IsOverride = false);

		//传入静态方法
		template<typename R, typename... Args>
		ClassBuilder<DT, Deriveds...>* PushStaticMethod(const std::string& alias, R(*funptr)(Args...));


		ClassBuilder(const std::string& alias);
		~ClassBuilder();
	private:

		void PullBaseClassInfo();
		template<typename T>
		void BuildeSerialization();

		std::shared_ptr<ClassDscp> dscp;
		std::string alias;

	};

	template<typename DT, typename ...Deriveds>
	template<class C, class T>
	inline ClassBuilder<DT, Deriveds...>* ClassBuilder<DT, Deriveds...>::PushField(const std::string& alias, T C::* ptr, bool isGlobal) {
		//构建序列化函数：
		/*
		*    是否序列化管理器中有相应的序列化
		*		是->不构建
		*		否->构建
		*/
		auto ser = SerializationManager::FindSer(STypeID<T>::hashID);
		if (ser == nullptr)
			BuildeSerialization<T>();

		dscp->fieldDscps.emplace(alias,
			FieldDscp::FieldDscp(alias, ptr, isGlobal, Flag<Deriveds...>{}));
		return this;
	}

	template<typename DT, typename ...Deriveds>
	template<typename C, typename R, typename ...Args>
	inline ClassBuilder<DT, Deriveds...>* ClassBuilder<DT, Deriveds...>::PushMethod(const std::string& alias, R(C::* funptr)(Args...), bool isVirtual, bool IsOverride) {
		dscp->methodsDscp.emplace(alias,
			MethDscp{ alias,funptr,isVirtual,IsOverride,Flag<Deriveds...> {} });
		return this;
	}
	template<typename DT, typename ...Deriveds>
	template<typename R, typename ...Args>
	inline ClassBuilder<DT, Deriveds...>* ClassBuilder<DT, Deriveds...>::PushStaticMethod(const std::string& alias, R(*funptr)(Args...)) {
		dscp->methodsDscp.emplace(alias,
			MethDscp{ alias,funptr });
		return this;
	}

	template<typename DT, typename ...Deriveds>
	inline ClassBuilder<DT, Deriveds...>::ClassBuilder(const std::string& alias)
		:alias(alias) {
		dscp.reset(new ClassDscp{ Flag<DT>{},alias });
		//导入父子类关系
		(
			[]() {
				Inheritance::RegisterRelationship<DT, Deriveds >();
			}()
				, ...);
		//导入alias---hashID关系
		ClassAlias::Register(alias, dscp->type.hashID);
		IDRegistry::Register(dscp->type.hashID, alias);
	}
	

	template<typename DT, typename ...Deriveds>
	inline ClassBuilder<DT, Deriveds...>::~ClassBuilder()
	{
		{
			//导入类
			ClassManager::PushClass(alias, dscp->type.hashID, dscp);
			//导入方法
			for (auto& t : dscp->methodsDscp) {
				if (t.second.alias == "construct" && !t.second.isMember) {
					dscp->constructor.reset(t.second.Instantiate(nullptr));
				}
				MethodDscpFlyWeight::PushMethod(alias, t.second.alias,
					t.second.funcType->hashID, t.second);
			}
			//导入字段
			for (auto& t : dscp->fieldDscps) {
				FieldDscpFlyWeight::PushMethod(alias, t.second.alias,
					t.second.type.hashID, t.second);
			}

			//导入子类的信息
			PullBaseClassInfo();

			BuildeSerialization<DT>();

		}
	}

	template<typename DT, typename ...Deriveds>
	inline void ClassBuilder<DT, Deriveds...>::PullBaseClassInfo()
	{
		{
			auto baseClassDscp = Inheritance::FindBaseClass(alias);
			if (!baseClassDscp.has_value())
				return;
			for (auto t : baseClassDscp.value()) {
				for (auto fieldOnBase : t->fieldDscps) {
					//观察子类中的字段是否应该被导入
					// 如果子类中存在，父类中不存在，导入
					// 如果子类中存在，父类中也存在，不导入

					/*
					* 是否父类中已经登记
					*	  否->导入
					*	  是->不导入
					*/

					if (dscp->fieldDscps.find(fieldOnBase.second.alias) != dscp->fieldDscps.end())
						continue;
					dscp->fieldDscps.emplace(fieldOnBase.second.alias, fieldOnBase.second);
				}
			}
			for (auto t : baseClassDscp.value()) {
				for (auto methodOnBase : t->methodsDscp) {
					/*
					*   programing：
					*	是否是成员函数
					*		是：
					*			是否是虚函数
					*				否->导入
					*				是：
					*					是否重载
					*						是->不导入
					*						否->导入
					*		否：
					*			是否是构造器：
					*				是->不导入
					*				否：
					*					是否重载：
					*						否->导入
					*						是->不导入
					*/

					/*
					* 需要将静态实例转换的情况
					*	是否是虚函数：
					*		是：
					*			是否重载
					*				是->不转换
					*				否->转换
					*		否->转换
					*/
					if (methodOnBase.second.isMember) {
						if (!methodOnBase.second.isVirtual) {
							//将其标记为需要将静态实例转换
							methodOnBase.second.needConvert = true;
							dscp->methodsDscp.emplace(methodOnBase.first, methodOnBase.second);
						}
						else {
							auto it = dscp->methodsDscp.find(methodOnBase.first);
							if (it != dscp->methodsDscp.end() &&
								*it->second.funcNoClassType == *methodOnBase.second.funcNoClassType)
								continue;
							//将其标记为需要将静态实例转换
							methodOnBase.second.needConvert = true;
							dscp->methodsDscp.emplace(methodOnBase.first, methodOnBase.second);

						}
					}
					else {//静态函数
						if (methodOnBase.first == "construct")
							continue;
						auto it = dscp->methodsDscp.find(methodOnBase.first);
						if (it != dscp->methodsDscp.end() &&
							*it->second.funcNoClassType == *methodOnBase.second.funcNoClassType) //重载
							continue;
						//没重载
						dscp->methodsDscp.emplace(methodOnBase.first, methodOnBase.second);
					}

				}
			}
		}
	}

	template<typename DT, typename ...Deriveds>
	template<typename T>
	inline void ClassBuilder<DT, Deriveds...>::BuildeSerialization(){
		Serialization ser;
		//如果为登记类，则用动态的方法构建
		std::cout << "construct serialization function for: " << STypeID<T>::strID << std::endl;
		if (ClassAlias::Find(STypeID<T>::hashID)) {
			ser.serialize = [](YAML::Node& node, const void* data) {
				const Object* obj = static_cast<const Object*>(data);
				//拿到字段描述
				auto& fields = obj->fieldInstance;
				for (auto& t : fields) {
					//找到对应字段的序列化函数
					const Serialization* fieldSer = SerializationManager::FindSer(t->dscp.type.hashID);
					if (fieldSer == nullptr || fieldSer->serialize == nullptr)
						throw std::exception{(obj->dscp.alias + "." + t->dscp.alias + " have not serialization function").c_str()};
					YAML::Node subNode = node[t->dscp.alias];
					//这边要判断一下是否是动态方法构建序列化函数，如果是，要将其封装为反射Object
					void* fieldData;
					if (ClassAlias::Find(t->dscp.type.hashID)) {
						auto classDscpOf_t = IDRegistry::NameOf(t->dscp.type.hashID);
						if (!classDscpOf_t)
							throw std::exception{"bad serialize"};
						fieldData = ClassManager::FindClass(
							*classDscpOf_t
						)->MakeWithData(t->data);
					}
					else
						fieldData = t->data;
					fieldSer->serialize(subNode, fieldData);
				}
				//遍历字段
			};
		}
		else {//如果为非登记类，那么看是否yaml是否支持

			ser.serialize = [](YAML::Node& node, const void* data) {
				if (!data)
					throw std::exception{"the data ptr that is serialized is nullptr"};
				const T& data_ = *static_cast<const T*>(data);
				node = YAML::convert<T>::encode(data_);
			};
		}
		SerializationManager::PushSerialization(STypeID<T>::hashID, ser);
	}

}



namespace YAML {
	//要不要把yaml对接到SerializationManger？？？
	template <typename T>
	struct convert {
		static bool flag() { return true; }
		static Node encode(const T& rhs) {
			auto serPtr = ::LuRef::SerializationManager::
				FindSer(::LuRef::STypeID<T>::hashID);
			if (!serPtr)
				throw std::exception{"bad decode"};
			Node node;
			//可能传入的是一个对象，要判别一下
			const void* fieldData;
			if (::LuRef::ClassAlias::Find(::LuRef::STypeID<T>::hashID)) {
				auto classDscpOf_T = ::LuRef::IDRegistry::NameOf(::LuRef::STypeID<T>::hashID);
				if (!classDscpOf_T)
					throw std::exception{"bad serialize"};
				fieldData = LuRef::ClassManager::FindClass(
					*classDscpOf_T
				)->MakeWithData((void*)(&rhs));
				//这边强制const T* -> void*也是没办法的事情
				//Object要求内部指针是非常量的，我们也不会
				//我们也可以保证在该段不会对rhs进行修改所以...
				//千万别在这里出bug！！！！！
			}
			else
				fieldData = &rhs;

			serPtr->serialize(node, fieldData);
			return node;
		}

		static bool decode(const Node& node, T& rhs) {
			auto serPtr = ::LuRef::SerializationManager::
				FindSer(::LuRef::STypeID<T>::hashID);
			if (!serPtr)
				throw std::exception{"bad decode"};
			return serPtr->deserialize(node, &rhs);
		}
	};
}