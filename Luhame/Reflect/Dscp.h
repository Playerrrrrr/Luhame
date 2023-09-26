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
		//������ָ��תΪ����ĺ���

		//const ClassDscp& owned;
	};


	class Method {
	public:
		//��Ա����
		template<typename C, typename R, typename... Args>
		Method(void* data, const MethDscp& dscp, R(C::* funptr)(Args...));
		//�ǳ�Ա����
		template<typename R, typename... Args>
		Method(const MethDscp& dscp, R(*funptr)(Args...));

		//������û�з���ֵ������ʹ��res����nullptr

		//�������ֵ�Ƿ�ָ�룬��ô��Ϊ��һ����֪�ڴ��й��������������ֵ��ָ�룬��ô��Ϊ��ȡ��ָ��
		//ע�⣬��Ϊָ��תָ����һ��תһ��������������߼������ã�Ҫ��Ȼ���ò����ⲿָ��
		//res����nullptr��Ϊ�����ս��,���Ǻ���std::function��Բ������е��ԣ�����Ϊ��
		//callable����
		template<typename C, typename R, typename...Args >
		void InvokeWithCallable(C*& obj, R*& res, Args&&... args);

		//��̬����
		template<typename R, typename...Args >
		void InvokeWithObj(SharedObject*&, R*& res, Args&&... args);

		template<typename R, typename...Args >
		void InvokeWithObj(Object*&, R*& res, Args&&... args);

		//res����nullptr��Ϊ�����ս��
		template<typename R, typename...Args >
		void InvokeStatic(R*& res, Args&&... args);

		friend class ClassDscp;
	public:
		void* data = nullptr;//�������ʵ��
		std::function<void(void*&, void*)> func;
		std::function<void(void*)> voidRFunc;
		const MethDscp& dscp;
	private:
	};


	class MethodDscpFlyWeight {
	public:
		//ͨ�������ռ�+������ѯ
		static const MethDscp* FindPtr(const std::string& alias);
	private:
		//��builder�е������ݵĽӿ�
		static void PushMethod(const std::string& nameSpace, const std::string& MethodAlias, size_t hashID, const MethDscp& dscp) {
			//ע�⣬��ΪӦ�������п����м���������ͬ�ĺ��������Լ��ϱ�������������
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
		//firstΪ�����hashID��secondΪ��ȡ����
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

		//ҪΪ��������setFun��Ҫ��Ȼ�Ӿ�̬ʵ������ȡ�ֶ�ָ������ƫ��
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
			//��ȡ�����и��ֶε���ȡ����
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
		void* data{ nullptr };//����,ע�⣬����ڴ�Field��û�п���Ȩ�ޣ���Ϊ���Ƕ����и��ֶ�����λ�ã�����Ȩ�ڶ���ʵ������
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
		//ͨ����������������
		template<typename T>
		static const FieldDscp* FindPtr() {
			if (map.find(STypeID<T>::UUID) == map.end())
				return nullptr;
			return &map.at(STypeID<T>::UUID);
		}
		//ͨ����������
		static const FieldDscp* FindPtr(const std::string& alias);

	private:
		//��builder�е������ݵĽӿ�
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
		//����field��method���ڲ��������Լ����ƣ�����Object�������û��ģ�����Ҫstd::shared_ptr
		template<typename...Args>
		std::shared_ptr<SharedObject> MakeShared(Args&&... args) const;
		template<typename...Args>
		Object* Make(Args&&... args) const;
		
		std::shared_ptr<SharedObject> MakeSharedWithData(void*) const;
		//��������Ǹ�һ�鱻���õ��ڴ洴�����󣬱�֤����ָ������û��һ
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

		//�������ֵ�Ƿ�ָ�룬��ô��Ϊ��һ����֪�ڴ��й��������������ֵ��ָ�룬��ô��Ϊ��ȡ��ָ��
		//ע�⣬��Ϊָ��תָ����һ��תһ��������������߼������ã�Ҫ��Ȼ���ò����ⲿָ��
		//res����nullptr��Ϊ�����ս��,���Ǻ���std::function��Բ������е��ԣ�����Ϊ��
		//callable����

		//���÷ǳ�Ա�������޷���ֵres����nullptr
		template<typename R, typename... Args>
		void InvokeStatic(const std::string& alias, R*& ptr, Args&&... args);

		template<typename R, typename... Args>
		void InvokeStatic(const std::string& alias, R*&& ptr, Args&&... args);

		//���ó�Ա�����޷���ֵres����nullptr
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

		//���÷ǳ�Ա�������޷���ֵres����nullptr
		template<typename R, typename... Args>
		void InvokeStatic(const std::string& alias, R*& ptr, Args&&... args);

		template<typename R, typename... Args>
		void InvokeStatic(const std::string& alias, R*&& ptr, Args&&... args);

		//���ó�Ա�����޷���ֵres����nullptr
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
		* ��ѯ�ӿ�,�ҵ����ض�Ӧָ�룬���򷵻�nullptr
		* ֧��hashcode����������̬���Ͳ�ѯ
		*/
		static const ClassDscp* FindClass(size_t);
		static const ClassDscp* FindClass(const std::string&);
		template<typename T>
		static const ClassDscp& FindClass() {
			return *FindClass(STypeID<T>::hashID);
		}
		//ʵ�����ӿ�
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

	//����̳���ϵ���࣬Ӧ��Ҫ��һ��ʵ��
	//�����һ����Ƶò�����ĵط���InheritanceӦ��ֻ����̳й�ϵ�����ȡClassDscp�Ĺ���Ӧ��ȥClassManager
	class Inheritance {
	public:
		static bool IsBaseOf(const std::string& base, const std::string& sub);
		static bool IsBaseOf(size_t base, size_t sub);
		//����type1�Ƿ���ת����type2
		//ע�⣬��ߵ�ԭ����������ڼ̳����У���ô���Կ��Ƿ�ת�ͣ����������float����ô�Ϳ�hashID�Ƿ���ͬ
		static bool CanConvert(const std::string& derived, const std::string& base);
		static bool CanConvert(size_t derived, size_t base);


		//ǳ����
		//��ָ��derived���͵�ָ��data����ȫת��Ϊdata�����ʧ�ܣ�return nullptr
		static void* Convert(void* data,size_t base, size_t derived);
		static Object* Convert(Object*, const std::string& base);
		static std::shared_ptr<SharedObject> Convert(std::shared_ptr<SharedObject>, const std::string& base);



		static std::optional<std::vector<const ClassDscp*>> FindBaseClass(size_t);
		static std::optional<std::vector<const ClassDscp*>> FindBaseClass(const std::string&);
		static std::optional<std::vector<const ClassDscp*>> FindDerivedClass(size_t);
		static std::optional<std::vector<const ClassDscp*>> FindDerivedClass(const std::string&);

		static bool IsRegister(size_t hashCode) { return rels.find(hashCode) != rels.end(); }


		/*
		* �Ǽǵ���Ϣ��
		*	1���̳й�ϵ���ɲ�ѯ
		*	2������ָ��ת��������������ָ�밲ȫת��Ϊ����ָ��
		*/
		template<typename Base, typename Sub>
		static void RegisterRelationship();
		
	private:
		static size_t hash(size_t base, size_t derived) { return size_t(float(base) / float(derived / 6352729811)); }
		//��һ���ڵ㿪ʼ��������������·���ϴ�������ѯ��hashID���򷵻�true������false
		static bool TraversalToBase(Relationship* ship, size_t hashID);
		template<typename T>
		static void Register();
		inline static std::unordered_map<size_t, std::shared_ptr<Relationship>> rels;

		//����ѯ���Ľ������������keyΪ��������baseID/derivedID��value��firstΪbase��secondΪderived
		inline static std::unordered_map<size_t, std::pair<size_t, size_t>> cache;
		inline static std::unordered_map<size_t,
			std::function<void* (void*)>> convertFun;
	};






	/*
	*
	*                         �����۵���������
	*                              |
	*                +-------------+-----------------+
					 |                               |
					 |                               |
			�����б����������                ����Method�����б�
					|                               |
					|                               |
					+------------����ʱ���----------
									|
									|ͨ��
									|
								std::apply����

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
		if (resType->isPtr)//������ֵλָ��ʱ����Ϊָ��תָ��
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
		* ����ֵ�Ƿ�Ϊ�գ�
		*	��->���������浽fun����
		*		����ֵ�Ƿ�Ϊָ�����ͣ�
		*			��
		*				���շ���ֵ��resָ���Ƿ�Ϊ��
		*					�ǣ������շ���ֵ
		*					������res�÷���ֵ���п�������
		*			�ǣ�ֱ�Ӹ�ֵ
		*	��-���������浽voidRFun
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
		//����Դ�����ֵΪ�յĺ���
		if constexpr (!std::is_same_v<void, R>) {
			func = [funptr](void*& res, void* paras) {
				using para_type_list = std::tuple<Args&&...>;
				para_type_list&& paraList = std::move(*reinterpret_cast<para_type_list*>(paras));
				//����Ƿ�ָ�룬��ô���ǵ��ÿ������캯���������ָ�룬�����ָ��
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
	//�����һ���Ż�����һ����̬����invoke���func�������Ͳ������ݸ�invoke�������Ϳ����ڱ����ڽ����걸�����ͼ�飬�ο�std::thread��ʵ��
	template<typename C, typename R, typename ...Args>
	inline void Method::InvokeWithCallable(C*& obj, R*& res, Args && ...args)
	{
		static bool isPass = false;
		try {
			if (!isPass) {
				dscp.CheckClass<C>();
				if(res!=nullptr)//����res��Ϊ�����շ���ֵ
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
				if (res != nullptr)//����res��Ϊ�����շ���ֵ
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
				if (res != nullptr)//����res��Ϊ�����շ���ֵ
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
		//����ָ��ת������
		convertFun.emplace(
			hash(STypeID<Base>::hashID, STypeID<Derived>::hashID),
			[](void* ptr)->void* {
				return static_cast<Base*>(static_cast<Derived*>(ptr));
			}
		);
	}

}