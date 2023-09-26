#include "Dscp.h"

namespace LuRef {
	bool LuRef::FieldDscp::IsSame(const FieldDscp& oth)const {
		return oth.type == this->type &&
			oth.type == this->type &&
			oth.isGlobal == this->isGlobal;
	}

	bool LuRef::FieldDscp::CanConvert(const FieldDscp& oth) const
	{
		return MatchProperty(this->type, oth.type);
	}

	const ClassDscp* LuRef::ClassManager::FindClass(const std::string& alias){
		auto it = ClassAlias::Find(alias);
		if (it == nullptr)
			return nullptr;
		return FindClass(*it);
	}
	const ClassDscp* LuRef::ClassManager::FindClass(size_t hashID){
		auto it = classMap.find(hashID);
		if (it == classMap.end())
			return nullptr;
		return it->second.get();
	}

	const MethDscp* MethodDscpFlyWeight::FindPtr(const std::string& alias) {
		auto it = aliasToHashID.find(alias);
		if (it == aliasToHashID.end())
			return nullptr;
		return &map.at(it->second);
	}

	const FieldDscp* FieldDscpFlyWeight::FindPtr(const std::string& alias) {
		auto it = aliasToHashID.find(alias);
		if (it == aliasToHashID.end())
			return nullptr;
		return &map.at(it->second);
	}
	

	void ClassManager::PushClass(const std::string& alias, size_t hashID, std::shared_ptr<ClassDscp> dscp) {
		ClassAlias::Register(alias, hashID);
		classMap.emplace(hashID, dscp);
	}

	Field* FieldDscp::Instantiate(void* data,size_t hashID) const {
		for (auto& t : offsetor) {
			if(t.first==hashID){
				Field* field = new Field{ t.second(data),*this,setFun };
				return field;
			}
		}
		return nullptr;
	}

	Method* MethDscp::Instantiate(void* data)const {
		return instantiateFun(this,data);
	}


	std::shared_ptr<Field> SharedObject::GetFieldWithAlias(const std::string& alias) const{
		for (auto& t : fieldInstance) {
			if (t->dscp.alias == alias)
				return t;
		}
		return nullptr;
	}

    Relationship::Relationship(const size_t& hashID,std::string name)
        :hashID(hashID) ,name(name){}

    Object* Inheritance::Convert(Object* obj, const std::string& base){
        const std::string& derived = obj->dscp.alias;
        auto hashBase = ClassAlias::Find(base);
        auto hashDerived = ClassAlias::Find(derived);
        if (!hashBase || !hashDerived|| IsBaseOf(base, derived)) {
			LU_CORE_ERROR_DS(("bad convert:" + base + " to " + derived).c_str());
            return nullptr;
        }
        auto convertObj = Convert(obj->As<void>(), *hashBase, *hashDerived);
        auto res = ClassManager::FindClass(*hashDerived)->MakeWithData(convertObj);
        return res;
    }

    
    std::shared_ptr<SharedObject> Inheritance::Convert(std::shared_ptr<SharedObject> obj, const std::string& base) {
        const std::string& derived = obj->dscp.alias;
        auto hashBase = ClassAlias::Find(base);
        auto hashDerived = ClassAlias::Find(derived);
        if (!hashBase || !hashDerived || !IsBaseOf(base, derived)) {
            LU_CORE_ERROR_DS(("bad convert:" + base + " to " + derived).c_str());
            return nullptr;
        }
        auto convertObj = Convert(obj->As<void>(), *hashBase, *hashDerived);
        auto res = ClassManager::FindClass(*hashDerived)->MakeSharedWithData(obj->objPtr);
        return res;
    }



    bool LuRef::Inheritance::IsBaseOf(const std::string& base, const std::string& sub) {
        auto derivedID = ClassAlias::Find(sub);
        auto baseID = ClassAlias::Find(base);
        if (derivedID == nullptr) {
            LU_LOG_WARN(sub + " not have alias");
            return false;
        }
        if (baseID == nullptr) {
            LU_LOG_WARN(base + " not have alias");
            return false;
        }
        return IsBaseOf(*baseID, *derivedID);
    }

    bool Inheritance::IsBaseOf(size_t base, size_t sub) {
        if (base == sub)
            return true;
        if (cache.find(hash(base, sub)) != cache.end())
            return true;
        auto it1 = rels.find(base);
        auto it2 = rels.find(sub);
        if (it1 == rels.end()) 
            return false;
        if (it2 == rels.end()) 
            return false;
        auto t = rels.find(sub)->second.get();
        if (TraversalToBase(t, base)) {
            //将结果缓存
			cache.emplace(hash(base, sub), std::pair<size_t, size_t>{base, sub});
            return true;
        }
        return false;
    }

    bool Inheritance::CanConvert(const std::string& type1, const std::string& type2) {
        return IsBaseOf(type2, type1);
    }

    bool Inheritance::CanConvert(size_t type1, size_t type2) {
        if (type1 == type2)
            return true;
        return IsBaseOf(type2, type1);
    }
    void* Inheritance::Convert(void* data, size_t base, size_t derived) {
        if (base == derived)
            return data;
        auto it = convertFun.find(hash(base, derived));
        if (it == convertFun.end())
            return nullptr;
        return it->second(data);
    }

    std::optional<std::vector<const ClassDscp*>> LuRef::Inheritance::FindBaseClass(size_t hashID) {
        if (!IsRegister(hashID))
            return std::nullopt;
        std::vector<const ClassDscp*> res;
        for (auto& t : rels.at(hashID)->baseClass) {
            res.push_back(&t->GetClassDscp());
        }
        return res;
    }

    std::optional<std::vector<const ClassDscp*>> Inheritance::FindBaseClass(const std::string& alias)
    {
        auto id = ClassAlias::Find(alias);
        if (id == nullptr || !IsRegister(*id))
            return std::nullopt;
        std::vector<const ClassDscp*> res;
        for (auto& t : rels.at(*id)->baseClass) {
            res.push_back(&t->GetClassDscp());
        }
        return res;
    }

    std::optional<std::vector<const ClassDscp*>> LuRef::Inheritance::FindDerivedClass(size_t hashID) {
        std::vector<const ClassDscp*> res;
        if (!IsRegister(hashID))
            return std::move(res);
        for (auto& t : rels.at(hashID)->subClass) {
            res.push_back(&t->GetClassDscp());
        }
        return res;
    }
    std::optional<std::vector<const ClassDscp*>> Inheritance::FindDerivedClass(const std::string&)
    {
        return std::optional<std::vector<const ClassDscp*>>();
    }
    bool Inheritance::TraversalToBase(Relationship* ship, size_t baseID) {
        if (ship->hashID == baseID)
            return true;
        for (auto& t : ship->baseClass) {
            if (TraversalToBase(t, baseID))
                return true;
        }
        return false;
    }


    std::shared_ptr<SharedObject> ClassDscp::MakeSharedWithData(void* data) const {
        void* objPtr = data;
        std::vector<std::shared_ptr<Field>> fields;
        for (const auto& t : fieldDscps) {
            fields.push_back(std::shared_ptr<Field>{t.second.Instantiate(objPtr, type.hashID)});
        }
        std::vector<std::shared_ptr<Method>> methods;
        for (const auto& t : methodsDscp) {
            void* data = objPtr;
            /*
            *	是否重载
            *		是->转换
            *			是否转换成功
            *				是->构造
            *				否->报错+pass
            *		否->不转换
            */
            if (t.second.needConvert) {
                data = Inheritance::Convert(objPtr, t.second.classType->hashID, type.hashID);
                if (data == nullptr) {
                    LU_LOG_ERROR("error : convert error" << "[at " << __FILE__ << " line: " << __LINE__);
                    continue;
                }
            }
            methods.push_back(std::shared_ptr<Method>{t.second.Instantiate(data)});
        }
        std::shared_ptr<SharedObject> obj{new SharedObject{ *this,objPtr,std::move(fields),std::move(methods) }};
        return obj;
    }

    std::shared_ptr<SharedObject> ClassDscp::MakeSharedWithData(std::shared_ptr<void> data) const {
        void* objPtr = data.get();//需要保证内存的安全
        std::vector<std::shared_ptr<Field>> fields;
        for (const auto& t : fieldDscps) {
            fields.push_back(std::shared_ptr<Field>{t.second.Instantiate(objPtr, type.hashID)});
        }
        std::vector<std::shared_ptr<Method>> methods;
        for (const auto& t : methodsDscp) {
            void* data = objPtr;
            /*
            *	是否重载
            *		是->转换
            *			是否转换成功
            *				是->构造
            *				否->报错+pass
            *		否->不转换
            */
            if (t.second.needConvert) {
                data = Inheritance::Convert(objPtr, t.second.classType->hashID, type.hashID);
                if (data == nullptr) {
                    LU_LOG_ERROR("error : convert error" << "[at " << __FILE__ << " line: " << __LINE__);
                    continue;
                }
            }
            methods.push_back(std::shared_ptr<Method>{t.second.Instantiate(data)});
        }
        std::shared_ptr<SharedObject> obj{new SharedObject{ *this,data,std::move(fields),std::move(methods) }};
        return obj;
    }

    Object* ClassDscp::MakeWithData(void* data) const
    {
        void* objPtr = data;
        std::vector<std::shared_ptr<Field>> fields;
        for (const auto& t : fieldDscps) {
            fields.push_back(std::shared_ptr<Field>{t.second.Instantiate(objPtr, type.hashID)});
        }
        std::vector<std::shared_ptr<Method>> methods;
        for (const auto& t : methodsDscp) {
            void* data = objPtr;
            if (t.second.needConvert) {
                data = Inheritance::Convert(objPtr, t.second.classType->hashID, type.hashID);
                if (data == nullptr) {
                    LU_LOG_ERROR("error : convert error" << "[at " << __FILE__ << " line: " << __LINE__);
                    continue;
                }
            }
            methods.push_back(std::shared_ptr<Method>{t.second.Instantiate(data)});
        } 
        return  new Object{ *this,objPtr,std::move(fields),std::move(methods) };
    }

    std::shared_ptr<Field> Object::GetFieldWithAlias(const std::string& alias) const
    {
		for (auto& t : fieldInstance) {
			if (t->dscp.alias == alias)
				return t;
		}
		return nullptr;
    }

}