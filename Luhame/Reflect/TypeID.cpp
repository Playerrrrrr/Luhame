#include"TypeID.h"
#include"Dscp.h"
namespace LuRef {

	bool IDRegistry::IsRegistered(size_t hashID)
	{
		return hashToStr.find(hashID) != hashToStr.end();
	}

	const std::string* IDRegistry::NameOf(size_t hashID)
	{
		if (hashToStr.find(hashID) == hashToStr.end())
			return nullptr;
		return &hashToStr.at(hashID);
	}

	void IDRegistry::Register(size_t hashID, const std::string& alias){
		if (IsRegistered(hashID))
			return;
		hashToStr.emplace(hashID, alias);
	}

	bool MatchProperty(const DTypeID& type1, const DTypeID& type2) {
		//��Ӧ����һ���̳����ж�
		if (type1.isPtr != type2.isPtr)//ָ�벻��ת����
			return false;
		else if (type1.isPtr && type2.isPtr)//ָ��תָ��
			return true;
		if (!Inheritance::CanConvert(type1.hashID,type2.hashID))//����ͬһ�����ͣ����Ӧ���ü̳�����
			return false;
		if (type1.isConst == VisitProperty::Const && type2.isConst == VisitProperty::Non)
			return false;
		if (type1.isRef != RefProperty::RRef && type2.isRef == RefProperty::RRef)
			return false;
		if (type1.isRef == RefProperty::RRef && type2.isRef == RefProperty::LRef && type2.isConst != VisitProperty::Const)
			return false;
		return true;

	}
	const char* GetPropertyChar(VisitProperty pro)
	{
		static const char* Non = "";
		static const char* Const = "const";
		if (pro == VisitProperty::Non)
			return Non;
		return Const;
	}
	const char* GetPropertyChar(RefProperty pro)
	{
		static const char* Non = "";
		static const char* LRef = "&";
		static const char* RRef = "&&";
		if (pro == RefProperty::Non)
			return Non;
		else if (pro == RefProperty::LRef)
			return LRef;
		return RRef;
	}
}