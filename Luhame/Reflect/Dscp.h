#pragma once
#include"TypeID.h"
#include"Util.hpp"
#include<any>
#include<memory>
#include<functional>
#include"Alias.h"
#include<queue>
#include"LuLog/lulog.hpp"
namespace LuRef {
	class Field;
	class SharedObject;
	class Object;
	class Method;

	enum VirtualFunState {
		NotVirtual = 0, Virtual = 1
	};

	enum OverrideFunState {
		NotOverride = 0, Override = 1
	};

	struct MethDscp {
	public:
		template<typename C, typename R, typename... Args,typename ...Derived>
		MethDscp(const std::string& alias, R(C::* funptr)(Args...), bool isVirtual,bool needConvert,Flag<Derived...>);


		template<typename R, typename... Args>
		MethDscp(const std::string& alias, R(*funptr)(Args...));


		template<typename T, typename...Args>
		void CheckParaType(int index) const;

		template<typename C>
		void CheckClass()const;
		template<typename R>
		void CheckReturnType()const;
		Method* Instantiate(void* data)const;
		 
		std::vector<const DTypeID*> paraListType;
		const DTypeID* resType;
		const DTypeID* classType;
		const DTypeID* funcType;
		const DTypeID* funcNoClassType;
		const std::string alias;
		const  bool isMember = false;
		const  bool isVirtual = false;
		bool needConvert = false;
	private:
		std::function<Method* (const MethDscp*,void*)> instantiateFun;
		//将子类指针转为基类的函数

		//const ClassDscp& owned;
	};


	class Method {
	public:
		//成员方法
		template<typename C, typename R, typename... Args>
		Method(void* data, const MethDscp& dscp, R(C::* funptr)(Args...));
		//非成员方法
		template<typename R, typename... Args>
		Method(const MethDscp& dscp, R(*funptr)(Args...));

		//若函数没有返回值，则建议使用res传入nullptr

		//如果返回值是非指针，那么视为在一块已知内存中构建对象，如果返回值是指针，那么视为获取该指针
		//注意，因为指针转指针是一级转一级，所以我们这边加上引用，要不然作用不到外部指针
		//res传入nullptr视为不接收结果,但是好像std::function会对参数进行调试，不能为空
		//callable调用
		template<typename C, typename R, typename...Args >
		void InvokeWithCallable(C*& obj, R*& res, Args&&... args);

		//动态调用
		template<typename R, typename...Args >
		void InvokeWithObj(SharedObject*&, R*& res, Args&&... args);

		template<typename R, typename...Args >
		void InvokeWithObj(Object*&, R*& res, Args&&... args);

		//res传入nullptr视为不接收结果
		template<typename R, typename...Args >
		void InvokeStatic(R*& res, Args&&... args);

		friend class ClassDscp;
	public:
		void* data = nullptr;//调用类的实例
		std::function<void(void*&, void*)> func;
		std::function<void(void*)> voidRFunc;
		const MethDscp& dscp;
	private:
	};


	class MethodDscpFlyWeight {
	public:
		//通过命名空间+别名查询
		static const MethDscp* FindPtr(const std::string& alias);
	private:
		//从builder中导入数据的接口
		static void PushMethod(const std::string& nameSpace, const std::string& MethodAlias, size_t hashID, const MethDscp& dscp) {
			//注意，因为应该类中有可能有几个类型相同的函数，所以加上别名来做以区分
			size_t methHash = hashID + std::hash<std::string>()(MethodAlias);
			aliasToHashID.emplace(nameSpace + "::" + MethodAlias, methHash);
			map.emplace(methHash, dscp);
		}
		inline static std::unordered_map<size_t, MethDscp> map;
		inline static std::unordered_map<std::string, size_t> aliasToHashID;

		template<typename DT, typename ...Deriveds>
		friend class ClassBuilder;
	};

	class Field;

	struct FieldDscp {
		//first为子类的hashID，second为拉取函数
		using Offsetor = std::pair<size_t,std::function<void* (void*)>>;
		const DTypeID& type;
		//const ClassDscp& owned;
		const bool isGlobal;
		const size_t offset;
		std::vector<Offsetor> offsetor;
		const std::string alias;
		std::function<void(void*, const void*)> setFun;
		template<typename T>
		bool IsSame()const {
			return type.isSame<T>();
		}
		bool IsSame(const FieldDscp& oth)const;

		template<typename T>
		bool CanConvert()const {
			return type.CanConvert<T>();
		}

		bool CanConvert(const FieldDscp& oth)const;

		//要为子类做好setFun，要不然从静态实例中拉取字段指针会出现偏移
		template<typename C, typename T,typename...Deriverds>
		FieldDscp(const std::string& alias, T C::* fieldPtr, bool isGlobal,Flag<Deriverds...>)
			:type(DTypeFlyweight::FindRef<T>()), offset(FieldOffset(fieldPtr))
			, isGlobal(isGlobal), alias(alias)
		{
			setFun = [](void* oldData, const void* newData) {
				auto t1 = static_cast<const T*>(oldData);
				auto t2 = static_cast<const T*>(newData);
				new (reinterpret_cast<T*>(oldData))T{
					*reinterpret_cast<const T*>(newData)
				};
			};
			offsetor.push_back({ STypeID<C>::hashID,FieldOffsetor(fieldPtr) });
			//获取子类中该字段的拉取函数
			(
				offsetor.push_back(
					{STypeID<Deriverds>::hashID, FieldOffsetor(static_cast<T Deriverds::*>(fieldPtr)) }
				)
			, ...);

		}
		Field* Instantiate(void*, size_t hashID) const;
		friend class Object;

	};


	class Field
	{
	public:
		template<typename T>
		void SetData(const T&& data) {
			if (!DTypeFlyweight::FindRef<T>().CanConvert(dscp.type)) {
				throw std::exception{"error: convert error"};
			}
			if constexpr (std::is_copy_constructible_v<T>)
				setFun(this->data, &data);
			else
				throw std::exception{("error:" + std::string{dscp.alias} + " need to implement copy constrcut function").c_str()};
		}
		template<typename T>
		T* GetVal() {
			if (!dscp.CanConvert(DTypeFlyweight::FindRef<T>())) {
				throw std::exception{"error: convert error"};
			}
			return static_cast<T*>(data);
		}

		void GetVal(Field& field) {
			if (!dscp.CanConvert(field.dscp)) {
				throw std::exception{"error: convert error"};
			}
			field.data = nullptr;
		}

		const FieldDscp& GetDscp() { return dscp; }
	private:
		Field(void* data, const FieldDscp& dscp, std::function<void(void*, const void*)> fun)
			:data(data), dscp(dscp), setFun(fun) {}
		void* data{ nullptr };//数据,注意，这块内存Field并没有控制权限，因为这是对象中该字段所在位置，控制权在对象实例手上
		const FieldDscp& dscp;
		
		std::function<void(void*, const void*)> setFun;

		friend struct FieldDscp;
		friend class SharedObject;
		template<typename DT, typename ...Deriveds>
		friend class ClassBuilder;
		friend class SerializationManager;
		friend class Object;
	};


	class FieldDscpFlyWeight {
	public:
		//通过编译期类型来找
		template<typename T>
		static const FieldDscp* FindPtr() {
			if (map.find(STypeID<T>::UUID) == map.end())
				return nullptr;
			return &map.at(STypeID<T>::UUID);
		}
		//通过别名来找
		static const FieldDscp* FindPtr(const std::string& alias);

	private:
		//从builder中导入数据的接口
		static void PushMethod(const std::string& nameSpace, const std::string& MethodAlias, size_t hashID, const FieldDscp& dscp) {
			aliasToHashID.emplace(nameSpace + "::" + MethodAlias, hashID);
			map.emplace(hashID, dscp);
		}
		inline static std::unordered_map<size_t, FieldDscp> map;
		inline static std::unordered_map<std::string, size_t> aliasToHashID;

		template<typename DT, typename ...Deriveds>
		friend class ClassBuilder;
	};


	class ClassDscp {
	private:
		std::unordered_map<std::string, FieldDscp> fieldDscps;
		std::unordered_map<std::string, MethDscp> methodsDscp;
		std::shared_ptr<Method> constructor;
		const DTypeID& type;
		const std::string alias;
		template<typename T>
		ClassDscp(Flag<T>, const std::string& alias)
			:type(DTypeFlyweight::FindRef<T>()), alias(alias) {}
		template<typename DT, typename ...Deriveds>
		friend class ClassBuilder;
	public:
		//创建field、method是内部，可以自己控制，但是Object是面向用户的，所以要std::shared_ptr
		template<typename...Args>
		std::shared_ptr<SharedObject> MakeShared(Args&&... args) const;
		template<typename...Args>
		Object* Make(Args&&... args) const;
		
		std::shared_ptr<SharedObject> MakeSharedWithData(void*) const;
		//这个方法是给一块被引用的内存创建对象，保证共享指针的引用会加一
		std::shared_ptr<SharedObject> MakeSharedWithData(std::shared_ptr<void>) const;
		Object* MakeWithData(void*) const;
		friend class Method;
		friend class Inheritance;
		template<typename DT, typename ...Deriveds>
		friend class ClassBuilder;
		friend class SerializationManager;
		friend class SharedObject;
		friend class Object;
	};


	class SharedObject {
	public:
		template<typename T>
		void SetFieldWithAlias(const std::string& alias, const T& data);

		std::shared_ptr<Field> GetFieldWithAlias(const std::string& alias) const;

		//如果返回值是非指针，那么视为在一块已知内存中构建对象，如果返回值是指针，那么视为获取该指针
		//注意，因为指针转指针是一级转一级，所以我们这边加上引用，要不然作用不到外部指针
		//res传入nullptr视为不接收结果,但是好像std::function会对参数进行调试，不能为空
		//callable调用

		//调用非成员函数，无返回值res传入nullptr
		template<typename R, typename... Args>
		void InvokeStatic(const std::string& alias, R*& ptr, Args&&... args);

		template<typename R, typename... Args>
		void InvokeStatic(const std::string& alias, R*&& ptr, Args&&... args);

		//调用成员函，无返回值res传入nullptr
		template<typename R, typename... Args>
		void InvokeMember(const std::string& alias, R*& res, Args&&... args);


		template<typename R, typename... Args>
		void InvokeMember(const std::string& alias, R*&& res, Args&&... args);


		template<typename T>
		T* As() { 
			return static_cast<T*>(objPtr.get());
		}

		std::shared_ptr<SharedObject*> Convert() {

		}
	protected:
		SharedObject(const ClassDscp& dscp, void* data, std::vector<std::shared_ptr<Field>>&& field,
			std::vector<std::shared_ptr<Method>>&& method)
			:dscp(dscp), objPtr(data),
			fieldInstance(std::move(field)), methodInstance(std::move(method)) {}
		SharedObject(const ClassDscp& dscp, std::shared_ptr<void> data, std::vector<std::shared_ptr<Field>>&& field,
			std::vector<std::shared_ptr<Method>>&& method)
			:dscp(dscp), objPtr(data),
			fieldInstance(std::move(field)), methodInstance(std::move(method)) {}

		const ClassDscp& dscp;
		std::shared_ptr<void> objPtr;
		std::vector<std::shared_ptr<Field>> fieldInstance;
		std::vector<std::shared_ptr<Method>> methodInstance;
		friend struct FieldDscp;
		friend class ClassDscp;
		friend struct MethDscp;
		friend class Method;
		friend class SerializationManager;
		template<typename DT, typename ...Deriveds>
		friend class ClassBuilder;
		friend class Inheritance;
	};

	class Object {
	public:
		template<typename T>
		void SetFieldWithAlias(const std::string& alias, const T& data);

		std::shared_ptr<Field> GetFieldWithAlias(const std::string& alias) const;

		//调用非成员函数，无返回值res传入nullptr
		template<typename R, typename... Args>
		void InvokeStatic(const std::string& alias, R*& ptr, Args&&... args);

		template<typename R, typename... Args>
		void InvokeStatic(const std::string& alias, R*&& ptr, Args&&... args);

		//调用成员函，无返回值res传入nullptr
		template<typename R, typename... Args>
		void InvokeMember(const std::string& alias, R*& res, Args&&... args);


		template<typename R, typename... Args>
		void InvokeMember(const std::string& alias, R*&& res, Args&&... args);


		template<typename T>
		T* As() { return static_cast<T*>(objPtr); }

	protected:
		Object(const ClassDscp& dscp, void* data, std::vector<std::shared_ptr<Field>>&& field,
			std::vector<std::shared_ptr<Method>>&& method)
			:dscp(dscp), objPtr(data),
			fieldInstance(std::move(field)), methodInstance(std::move(method)) {}


		const ClassDscp& dscp;
		void* objPtr;
		std::vector<std::shared_ptr<Field>> fieldInstance;
		std::vector<std::shared_ptr<Method>> methodInstance;
		friend struct FieldDscp;
		friend class ClassDscp;
		friend struct MethDscp;
		friend class Method;
		friend class SerializationManager;
		template<typename DT, typename ...Deriveds>
		friend class ClassBuilder;
		friend class Inheritance;
	};










	class ClassManager {
	public:
		/*
		* 查询接口,找到返回对应指针，否则返回nullptr
		* 支持hashcode、别名、静态类型查询
		*/
		static const ClassDscp* FindClass(size_t);
		static const ClassDscp* FindClass(const std::string&);
		template<typename T>
		static const ClassDscp& FindClass() {
			return *FindClass(STypeID<T>::hashID);
		}
		//实例化接口
	private:
		static void PushClass(const std::string& alias, size_t hashID, std::shared_ptr<ClassDscp> dscp);

		inline static std::unordered_map<size_t, std::shared_ptr<ClassDscp>> classMap;

		template<typename DT, typename ...Deriveds>
		friend class ClassBuilder;
	};



	struct Relationship {
		std::vector<Relationship*> baseClass;
		std::vector<Relationship*> subClass;
		size_t hashID;
		std::string name;
		const ClassDscp& GetClassDscp() { return *ClassManager::FindClass(hashID); }
		Relationship(const size_t& hashID,std::string name);
	};

	//管理继承体系的类，应该要有一个实例
	//这边有一个设计得不合理的地方，Inheritance应该只管理继承关系，其获取ClassDscp的工作应该去ClassManager
	class Inheritance {
	public:
		static bool IsBaseOf(const std::string& base, const std::string& sub);
		static bool IsBaseOf(size_t base, size_t sub);
		//返回type1是否能转换成type2
		//注意，这边的原则是如果类在继承树中，那么可以看是否转型，如果不在如float，那么就看hashID是否相同
		static bool CanConvert(const std::string& derived, const std::string& base);
		static bool CanConvert(size_t derived, size_t base);


		//浅拷贝
		//将指向derived类型的指针data，安全转换为data，如果失败，return nullptr
		static void* Convert(void* data,size_t base, size_t derived);
		static Object* Convert(Object*, const std::string& base);
		static std::shared_ptr<SharedObject> Convert(std::shared_ptr<SharedObject>, const std::string& base);



		static std::optional<std::vector<const ClassDscp*>> FindBaseClass(size_t);
		static std::optional<std::vector<const ClassDscp*>> FindBaseClass(const std::string&);
		static std::optional<std::vector<const ClassDscp*>> FindDerivedClass(size_t);
		static std::optional<std::vector<const ClassDscp*>> FindDerivedClass(const std::string&);

		static bool IsRegister(size_t hashCode) { return rels.find(hashCode) != rels.end(); }


		/*
		* 登记的信息：
		*	1：继承关系，可查询
		*	2：函数指针转换函数，将子类指针安全转换为基类指针
		*/
		template<typename Base, typename Sub>
		static void RegisterRelationship();
		
	private:
		static size_t hash(size_t base, size_t derived) { return size_t(float(base) / float(derived / 6352729811)); }
		//从一个节点开始往子类遍历，如果路劲上存在所查询的hashID，则返回true，否则false
		static bool TraversalToBase(Relationship* ship, size_t hashID);
		template<typename T>
		static void Register();
		inline static std::unordered_map<size_t, std::shared_ptr<Relationship>> rels;

		//将查询到的结果缓存起来，key为两个类型baseID/derivedID，value中first为base，second为derived
		inline static std::unordered_map<size_t, std::pair<size_t, size_t>> cache;
		inline static std::unordered_map<size_t,
			std::function<void* (void*)>> convertFun;
	};






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


	template<typename C, typename R, typename... Args,typename ...Derived>
	inline MethDscp::MethDscp(const std::string& alias, R(C::* funptr)(Args...), bool isVirtual, bool needConvert, Flag<Derived...>)
		:alias(alias), isMember(true), isVirtual(isVirtual), needConvert(needConvert){
		(paraListType.push_back(
			DTypeFlyweight::FindPtr<Args>()
		), ...);
		std::vector<std::pair<VisitProperty, std::string>> test;
		(
			test.push_back({ STypeID<Args>::isConst, STypeID<Args>::strID })
			, ...);
		resType = DTypeFlyweight::FindPtr<R>();
		classType = DTypeFlyweight::FindPtr<C>();
		funcType = DTypeFlyweight::FindPtr<decltype(funptr)>();
		funcNoClassType = DTypeFlyweight::FindPtr<R(*)(Args...)>();
		instantiateFun = [funptr, this](const MethDscp* dscp,void* data) {
			return new Method{ data, *dscp ,funptr };
		};
	}

	template<typename R, typename... Args>
	inline MethDscp::MethDscp(const std::string& alias, R(*funptr)(Args...))
		:alias(alias), isMember(false){
		(paraListType.push_back(
			DTypeFlyweight::FindPtr<Args>()
		), ...);
		resType = DTypeFlyweight::FindPtr<R>();
		funcType = DTypeFlyweight::FindPtr<decltype(funptr)>();
		instantiateFun = [funptr, this](const MethDscp* dscp,void* data) {
			return new Method{*dscp,funptr };
		};
	}


	template<typename T, typename...Args>
	void inline MethDscp::CheckParaType(int index) const {
		try {
			if (!MatchProperty(DTypeFlyweight::FindRef<T>(), *paraListType[index])) {
				throw std::exception{
					std::string{
						std::string("error: the para type is wrong [") +
							GetPropertyChar(STypeID<T>::isConst) + " " +
							std::string(STypeID<T>::strID) +
							GetPropertyChar(STypeID<T>::isRef) + " " +
							std::string("] convert to [") +
							GetPropertyChar(paraListType[index]->isConst) + " " +
							std::string(paraListType[index]->strID) +
							GetPropertyChar(paraListType[index]->isRef) + " " +
							"] at position " +
							std::to_string(index)
					}.c_str()
				};
			}
			if constexpr (sizeof...(Args) != 0)
				CheckParaType<Args...>(index + 1);
			else
				return;
		}
		catch (std::exception e) {
			throw e;
		}
	}

	template<typename C>
	void inline MethDscp::CheckClass()const {
		if (!MatchProperty(DTypeFlyweight::FindRef<C>(), *classType)) {
			throw std::exception{
				std::string{
					std::string("error: the class type is wrong") +
						GetPropertyChar(STypeID<C>::isConst) +
						std::string(STypeID<C>::strID) +
						GetPropertyChar(STypeID<C>::isRef) +
						std::string(" convert to ") +
						GetPropertyChar(classType->isConst) +
						std::string(classType->strID) +
						GetPropertyChar(classType->isRef)

				}.c_str()
			};
		}
	}

	template<typename R>
	void inline MethDscp::CheckReturnType()const {
		if (resType->isPtr)//当返回值位指针时，视为指针转指针
			return;
		if (!MatchProperty(DTypeFlyweight::FindRef<R>(), *resType)) {
			throw std::exception{
				std::string{
					std::string("error: the return res type is wrong") +
						GetPropertyChar(STypeID<R>::isConst) +
						std::string(STypeID<R>::strID) +
						GetPropertyChar(STypeID<R>::isRef) +
						std::string(" convert to ") +
						GetPropertyChar(resType->isConst) +
						std::string(resType->strID) +
						GetPropertyChar(resType->isRef)
				}.c_str()
			};
		}
	}

	template<typename...Args>
	inline std::shared_ptr<SharedObject> ClassDscp::MakeShared(Args&&... args) const {
		void* objPtr = nullptr;
		constructor->InvokeStatic(objPtr, std::forward<Args>(args)...);
		return MakeSharedWithData(objPtr);
	}

	template<typename...Args>
	inline Object* ClassDscp::Make(Args&&... args) const {
		void* objPtr = nullptr;
		constructor->InvokeStatic(objPtr, std::forward<Args>(args)...);
		return MakeWithData(objPtr);
	}


	template<typename C, typename R, typename... Args>
	inline Method::Method(void* data, const MethDscp& dscp, R(C::* funptr)(Args...))
		:dscp(dscp), data(data) {
		/*
		* 返回值是否为空：
		*	否->将函数保存到fun，并
		*		返回值是否为指针类型：
		*			否：
		*				接收返回值的res指针是否为空
		*					是：不接收返回值
		*					否：在上res用返回值进行拷贝构造
		*			是：直接赋值
		*	是-将函数保存到voidRFun
		*/
		if constexpr (!std::is_same_v<void, R>) {
			func = [funptr](void*& res, void* paras) {
				using para_type_list = std::tuple<C*, Args&&...>;
				para_type_list&& paraList = std::move(*reinterpret_cast<para_type_list*>(paras));

				if constexpr (!std::is_pointer_v<R>) {
					if (res != nullptr)
						new (reinterpret_cast<R*>(res)) R{
							std::apply(funptr, std::forward<decltype(paraList)>(paraList))
					};
					else
						std::apply(funptr, std::forward<decltype(paraList)>(paraList));
				}
				else {
					res = std::apply(funptr, std::forward<decltype(paraList)>(paraList));
				}
			};
		}
		else {
			voidRFunc = [funptr](void* paras) {
				using para_type_list = std::tuple<C*, Args&&...>;
				para_type_list&& paraList = std::move(*reinterpret_cast<para_type_list*>(paras));
				std::apply(funptr, std::forward<decltype(paraList)>(paraList));
			};
		}
	}

	template<typename R, typename...Args >
	inline Method::Method(const MethDscp& dscp, R(*funptr)(Args...))
		:dscp(dscp) {
		//区别对待返回值为空的函数
		if constexpr (!std::is_same_v<void, R>) {
			func = [funptr](void*& res, void* paras) {
				using para_type_list = std::tuple<Args&&...>;
				para_type_list&& paraList = std::move(*reinterpret_cast<para_type_list*>(paras));
				//如果是非指针，那么就是调用拷贝构造函数，如果是指针，则接收指针
				if constexpr (!std::is_pointer_v<R>) {
					if (res != nullptr)
						new (reinterpret_cast<R*>(res)) R{
						std::apply(funptr, std::forward<decltype(paraList)>(paraList))
					};
					else
						std::apply(funptr, std::forward<decltype(paraList)>(paraList));
				}
				else {
					res = std::apply(funptr, std::forward<decltype(paraList)>(paraList));
				}
			};
		}
		else {
			voidRFunc = [funptr](void* paras) {
				using para_type_list = std::tuple<Args&&...>;
				para_type_list&& paraList = std::move(*reinterpret_cast<para_type_list*>(paras));
				std::apply(funptr, std::forward<decltype(paraList)>(paraList));
			};
		}
	}
	//这边有一个优化，用一个静态函数invoke替代func，把类型参数传递给invoke，这样就可以在编译期进行完备的类型检查，参考std::thread的实现
	template<typename C, typename R, typename ...Args>
	inline void Method::InvokeWithCallable(C*& obj, R*& res, Args && ...args)
	{
		static bool isPass = false;
		try {
			if (!isPass) {
				dscp.CheckClass<C>();
				if(res!=nullptr)//传入res视为不接收返回值
					dscp.CheckReturnType<R>();
				if constexpr (sizeof...(Args) != 0)
					dscp.CheckParaType<Args&&...>(0);
			}
			isPass = true;

		}
		catch (std::exception& e) {
			LU_CORE_ERROR_DS(e.what());
			return;
		}
		using para_type_list = std::tuple<C*, Args&&...>;
		para_type_list paraList{ obj,std::forward<Args>(args)... };

		if (dscp.resType->hashID != STypeID<void>::hashID)
			func(reinterpret_cast<void*&>(res), reinterpret_cast<void*>(&paraList));
		else
			voidRFunc(reinterpret_cast<void*>(&paraList));
	}

	template<typename R, typename ...Args>
	inline void Method::InvokeWithObj(SharedObject*& obj, R*& res, Args && ...args)
	{
		static bool isPass = false;
		try {
			if (!isPass) {
				dscp.classType->CanConvert(obj->dscp.type);
				if (res != nullptr)//传入res视为不接收返回值
					dscp.CheckReturnType<R>();
				if constexpr (sizeof...(Args) != 0)
					dscp.CheckParaType<Args&&...>(0);
			}
			isPass = true;
		}
		catch (std::exception e) {
			LU_CORE_ERROR_DS(e.what());
			return;
		}
		using para_type_list = std::tuple<void*, Args&&...>;
		para_type_list paraList{
			data,
			std::forward<Args>(args)... };

		if (dscp.resType->hashID != STypeID<void>::hashID)
			func(reinterpret_cast<void*&>(res), reinterpret_cast<void*>(&paraList));
		else
			voidRFunc(reinterpret_cast<void*>(&paraList));
	}


	template<typename R, typename ...Args>
	inline void Method::InvokeWithObj(Object*& obj, R*& res, Args && ...args)
	{
		static bool isPass = false;
		try {
			if (!isPass) {
				dscp.classType->CanConvert(obj->dscp.type);
				if (res != nullptr)//传入res视为不接收返回值
					dscp.CheckReturnType<R>();
				if constexpr (sizeof...(Args) != 0)
					dscp.CheckParaType<Args&&...>(0);
			}
			isPass = true;
		}
		catch (std::exception e) {
			LU_CORE_ERROR_DS(e.what());
			return;
		}
		using para_type_list = std::tuple<void*, Args&&...>;
		para_type_list paraList{
			data,
			std::forward<Args>(args)... };

		if (dscp.resType->hashID != STypeID<void>::hashID)
			func(reinterpret_cast<void*&>(res), reinterpret_cast<void*>(&paraList));
		else
			voidRFunc(reinterpret_cast<void*>(&paraList));
	}

	template<typename R, typename ...Args>
	inline void Method::InvokeStatic(R*& res, Args && ...args) {
		static bool isPass = false;
		try {
			if (!isPass) {
				dscp.CheckReturnType<R>();
				if constexpr (sizeof...(Args) != 0)
					dscp.CheckParaType<Args&&...>(0);
			}
			isPass = true;
		}
		catch (std::exception e) {

			LU_CORE_ERROR_DS(e.what());

			return;
		}
		using para_type_list = std::tuple<Args&&...>;
		para_type_list paraList{ std::forward<Args>(args)... };

		if (dscp.resType->hashID != STypeID<void>::hashID)
			func(reinterpret_cast<void*&>(res), reinterpret_cast<void*>(&paraList));
		else
			voidRFunc(reinterpret_cast<void*>(&paraList));
	}



	template<typename T>
	inline void SharedObject::SetFieldWithAlias(const std::string& alias, const T& data) {
		for (auto& t : fieldInstance) {
			if (t->dscp.alias == alias) {
				t->SetData(std::forward<const T>(data));
				break;
			}
		}
	}

	template<typename R, typename ...Args>
	inline void SharedObject::InvokeStatic(const std::string& alias, R*& res, Args && ...args) {
		for (auto t : methodInstance) {
			if (t->dscp.alias == alias && !t->dscp.isMember) {
				t->InvokeStatic(res, std::forward<Args>(args)...);
				break;
			}
		}
	}

	template<typename R, typename ...Args>
	inline void SharedObject::InvokeStatic(const std::string& alias, R*&& res, Args && ...args) {
		for (auto t : methodInstance) {
			if (t->dscp.alias == alias && !t->dscp.isMember) {
				SharedObject* temp = this;
				t->InvokeStatic(res, std::forward<Args>(args)...);
				return;
			}
		}
		throw std::exception{"bad invoke"};
	}

	template<typename R, typename ...Args>
	inline void SharedObject::InvokeMember(const std::string& alias, R*& res, Args && ...args) {
		for (auto t : methodInstance) {
			if (t->dscp.alias == alias && t->dscp.isMember) {
				SharedObject* temp = this;
				t->InvokeWithObj(temp, res, std::forward<Args>(args)...);
				return;
			}
		}
		throw std::exception{"bad invoke"};
	}

	template<typename R, typename ...Args>
	inline void SharedObject::InvokeMember(const std::string& alias, R*&& res, Args && ...args) {
		InvokeMember(alias, res, std::forward<Args>(args)...);
	}



	template<typename T>
	inline void Object::SetFieldWithAlias(const std::string& alias, const T& data) {
		for (auto& t : fieldInstance) {
			if (t->dscp.alias == alias) {
				t->SetData(std::forward<const T>(data));
				break;
			}
		}
	}

	template<typename R, typename ...Args>
	inline void Object::InvokeStatic(const std::string& alias, R*& res, Args && ...args) {
		for (auto t : methodInstance) {
			if (t->dscp.alias == alias && !t->dscp.isMember) {
				t->InvokeStatic(res, std::forward<Args>(args)...);
				break;
			}
		}
	}

	template<typename R, typename ...Args>
	inline void Object::InvokeStatic(const std::string& alias, R*&& res, Args && ...args) {
		InvokeMember(alias, res, std::forward<Args>(args)...);
	}

	template<typename R, typename ...Args>
	inline void Object::InvokeMember(const std::string& alias, R*& res, Args && ...args) {
		for (auto t : methodInstance) {
			if (t->dscp.alias == alias && t->dscp.isMember) {
				Object* temp = this;
				t->InvokeWithObj(temp, res, std::forward<Args>(args)...);
				return;
			}
		}
		throw std::exception{"bad invoke"};
	}

	template<typename R, typename ...Args>
	inline void Object::InvokeMember(const std::string& alias, R*&& res, Args && ...args) {
		InvokeMember(alias, res, std::forward<Args>(args)...);
	}




	template<typename T>
	inline void Inheritance::Register() {
		if (rels.find(STypeID<T>::hashID) != rels.end())
			return;
		rels.emplace(STypeID<T>::hashID,
			std::shared_ptr<Relationship>{new Relationship{ STypeID<T>::hashID,STypeID<T>::strID }}
		);
	}
	template<typename Base_, typename Derived_>
	inline void Inheritance::RegisterRelationship() {
		using Base = std::decay_t<Base_>;
		using Derived = std::decay_t<Derived_>;
		if constexpr (!std::is_base_of_v<Base, Derived>) {
			LU_CORE_WARN_FL("{} not base of {}", STypeID<Base>::strID, STypeID<Derived>::strID);
			return;
		}
		Register<Base>();
		Register<Derived>();
		auto itBase = rels.find(STypeID<Base>::hashID);
		bool isExist = false;
		for (auto t : itBase->second->subClass) {
			if (t->hashID == STypeID<Derived_>::hashID)
				isExist = true;
		}
		if (isExist)
			return;
		auto itSub = rels.find(STypeID<Derived>::hashID);
		std::cout << STypeID<Derived>::strID << " " << STypeID<Base>::strID << std::endl;
		itBase->second->subClass.push_back(itSub->second.get());
		itSub->second->baseClass.push_back(itBase->second.get());
		//保存指针转换函数
		convertFun.emplace(
			hash(STypeID<Base>::hashID, STypeID<Derived>::hashID),
			[](void* ptr)->void* {
				return static_cast<Base*>(static_cast<Derived*>(ptr));
			}
		);
	}

}