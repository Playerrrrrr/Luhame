#pragma once
#include"Dscp.h"
#include"Serialization.h"
namespace LuRef {
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
	/*
	��ע�����Ϣ����

		���裺
			1��ÿһ���������в�ͬ��alias��������������
			2����̬��������aliasΪconstruct�ĺ���Ϊ������

		�Ƿ���ϴ��麯���� | yes

		1��ע������Ϣ��ClassDscp�����佻��ClassManager
		2��ע���ֶ���Ϣ��FieldDscp�����佻��FieldFlyWeight
		3��ע���ֶ���Ϣ��MethDscp�����佻��MethodFlyWeight
		4��ע�����->hashID��ClassAlis
		5��ע��hashID->������IDRegistry
		6��ע��������Ϣ�����ɲ�ѯ����
	*/
	template<typename DT, typename ...Deriveds>
	class ClassBuilder {
	public:
		/* �����ֶ�*/
		template<class C, class T>
		ClassBuilder<DT, Deriveds...>* PushField(const std::string& alias, T C::* ptr, bool isGlobal = false);
		/* �����Ա����*/
		template<typename C, typename R, typename... Args>
		ClassBuilder<DT, Deriveds...>* PushMethod(const std::string& alias, R(C::* funptr)(Args...), bool isVirtual = false, bool IsOverride = false);

		//���뾲̬����
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
		//�������л�������
		/*
		*    �Ƿ����л�������������Ӧ�����л�
		*		��->������
		*		��->����
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
		//���븸�����ϵ
		(
			[]() {
				Inheritance::RegisterRelationship<DT, Deriveds >();
			}()
				, ...);
		//����alias---hashID��ϵ
		ClassAlias::Register(alias, dscp->type.hashID);
		IDRegistry::Register(dscp->type.hashID, alias);
	}
	

	template<typename DT, typename ...Deriveds>
	inline ClassBuilder<DT, Deriveds...>::~ClassBuilder()
	{
		{
			//������
			ClassManager::PushClass(alias, dscp->type.hashID, dscp);
			//���뷽��
			for (auto& t : dscp->methodsDscp) {
				if (t.second.alias == "construct" && !t.second.isMember) {
					dscp->constructor.reset(t.second.Instantiate(nullptr));
				}
				MethodDscpFlyWeight::PushMethod(alias, t.second.alias,
					t.second.funcType->hashID, t.second);
			}
			//�����ֶ�
			for (auto& t : dscp->fieldDscps) {
				FieldDscpFlyWeight::PushMethod(alias, t.second.alias,
					t.second.type.hashID, t.second);
			}

			//�����������Ϣ
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
					//�۲������е��ֶ��Ƿ�Ӧ�ñ�����
					// ��������д��ڣ������в����ڣ�����
					// ��������д��ڣ�������Ҳ���ڣ�������

					/*
					* �Ƿ������Ѿ��Ǽ�
					*	  ��->����
					*	  ��->������
					*/

					if (dscp->fieldDscps.find(fieldOnBase.second.alias) != dscp->fieldDscps.end())
						continue;
					dscp->fieldDscps.emplace(fieldOnBase.second.alias, fieldOnBase.second);
				}
			}
			for (auto t : baseClassDscp.value()) {
				for (auto methodOnBase : t->methodsDscp) {
					/*
					*   programing��
					*	�Ƿ��ǳ�Ա����
					*		�ǣ�
					*			�Ƿ����麯��
					*				��->����
					*				�ǣ�
					*					�Ƿ�����
					*						��->������
					*						��->����
					*		��
					*			�Ƿ��ǹ�������
					*				��->������
					*				��
					*					�Ƿ����أ�
					*						��->����
					*						��->������
					*/

					/*
					* ��Ҫ����̬ʵ��ת�������
					*	�Ƿ����麯����
					*		�ǣ�
					*			�Ƿ�����
					*				��->��ת��
					*				��->ת��
					*		��->ת��
					*/
					if (methodOnBase.second.isMember) {
						if (!methodOnBase.second.isVirtual) {
							//������Ϊ��Ҫ����̬ʵ��ת��
							methodOnBase.second.needConvert = true;
							dscp->methodsDscp.emplace(methodOnBase.first, methodOnBase.second);
						}
						else {
							auto it = dscp->methodsDscp.find(methodOnBase.first);
							if (it != dscp->methodsDscp.end() &&
								*it->second.funcNoClassType == *methodOnBase.second.funcNoClassType)
								continue;
							//������Ϊ��Ҫ����̬ʵ��ת��
							methodOnBase.second.needConvert = true;
							dscp->methodsDscp.emplace(methodOnBase.first, methodOnBase.second);

						}
					}
					else {//��̬����
						if (methodOnBase.first == "construct")
							continue;
						auto it = dscp->methodsDscp.find(methodOnBase.first);
						if (it != dscp->methodsDscp.end() &&
							*it->second.funcNoClassType == *methodOnBase.second.funcNoClassType) //����
							continue;
						//û����
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
		//���Ϊ�Ǽ��࣬���ö�̬�ķ�������
		std::cout << "construct serialization function for: " << STypeID<T>::strID << std::endl;
		if (ClassAlias::Find(STypeID<T>::hashID)) {
			ser.serialize = [](YAML::Node& node, const void* data) {
				const Object* obj = static_cast<const Object*>(data);
				//�õ��ֶ�����
				auto& fields = obj->fieldInstance;
				for (auto& t : fields) {
					//�ҵ���Ӧ�ֶε����л�����
					const Serialization* fieldSer = SerializationManager::FindSer(t->dscp.type.hashID);
					if (fieldSer == nullptr || fieldSer->serialize == nullptr)
						throw std::exception{(obj->dscp.alias + "." + t->dscp.alias + " have not serialization function").c_str()};
					YAML::Node subNode = node[t->dscp.alias];
					//���Ҫ�ж�һ���Ƿ��Ƕ�̬�����������л�����������ǣ�Ҫ�����װΪ����Object
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
				//�����ֶ�
			};
		}
		else {//���Ϊ�ǵǼ��࣬��ô���Ƿ�yaml�Ƿ�֧��

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
	//Ҫ��Ҫ��yaml�Խӵ�SerializationManger������
	template <typename T>
	struct convert {
		static bool flag() { return true; }
		static Node encode(const T& rhs) {
			auto serPtr = ::LuRef::SerializationManager::
				FindSer(::LuRef::STypeID<T>::hashID);
			if (!serPtr)
				throw std::exception{"bad decode"};
			Node node;
			//���ܴ������һ������Ҫ�б�һ��
			const void* fieldData;
			if (::LuRef::ClassAlias::Find(::LuRef::STypeID<T>::hashID)) {
				auto classDscpOf_T = ::LuRef::IDRegistry::NameOf(::LuRef::STypeID<T>::hashID);
				if (!classDscpOf_T)
					throw std::exception{"bad serialize"};
				fieldData = LuRef::ClassManager::FindClass(
					*classDscpOf_T
				)->MakeWithData((void*)(&rhs));
				//���ǿ��const T* -> void*Ҳ��û�취������
				//ObjectҪ���ڲ�ָ���Ƿǳ����ģ�����Ҳ����
				//����Ҳ���Ա�֤�ڸöβ����rhs�����޸�����...
				//ǧ����������bug����������
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