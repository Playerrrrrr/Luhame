#pragma once

#include<typeinfo>
#include<unordered_map>
#include<string>
#include<optional>
#include<iostream>

namespace LuRef {
	struct DTypeID;
	enum class RefProperty {
		Non, LRef, RRef
	};
	enum class VisitProperty {
		Non, Const
	};

	template<typename ...T>
	struct Flag{};

	
	/*
	* 判断type是否能转化位type2
	*/
	bool MatchProperty(const DTypeID& type1, const DTypeID& type2);// type1 convert to type2
	const char* GetPropertyChar(VisitProperty pro);
	const char* GetPropertyChar(RefProperty pro);

	template<typename T>
	struct STypeID {

		const static inline size_t  hashID = typeid(T).hash_code();
		const static inline std::string strID = typeid(T).name();
		const static inline VisitProperty isConst
			= std::is_const_v<std::conditional_t<std::is_reference_v<T>,
			std::remove_reference_t<T>, T>> ?
			VisitProperty::Const : VisitProperty::Non;
		const static inline RefProperty isRef = std::is_lvalue_reference_v<T> ? 
			RefProperty::LRef : 
				std::is_rvalue_reference_v<T> ?
				RefProperty::RRef : RefProperty::Non;
		const constexpr static inline size_t size = sizeof(T);
		const static inline bool isPtr = std::is_pointer_v<T>;

		const inline static size_t UUID = hashID + 
			(isConst == VisitProperty::Const ? 10:100) +
			(isRef == RefProperty::Non?1000:(isRef==RefProperty::LRef?10000:100000));

	};

	template<>
	struct STypeID<void> {
		const static inline size_t  hashID = typeid(void).hash_code();
		const static inline std::string strID = typeid(void).name();
		const static inline VisitProperty isConst = VisitProperty::Non;
		const static inline RefProperty isRef = RefProperty::Non;
		const constexpr static inline size_t size = 0;
		const static inline bool isPtr = false;
		const inline static size_t UUID = hashID;
	};


	struct DTypeID {
	private:
		DTypeID(){}
		template<class T>
		DTypeID(Flag<T>)
			:hashID(STypeID<T>::hashID),
			 strID(STypeID<T>::strID),
			 isConst(STypeID<T>::isConst),
			 isRef(STypeID<T>::isRef),
			 size(STypeID<T>::size),
			isPtr(STypeID<T>::isPtr){}
		DTypeID(size_t hashID, const std::string_view& strID, LuRef::VisitProperty isConst, 
			LuRef::RefProperty isRef, size_t size, bool isPtr)
			: hashID(hashID),
			strID(strID),
			isConst(isConst),
			isRef(isRef),
			size(size),
			isPtr(isPtr)
		{}
		DTypeID(const DTypeID& type)
			: hashID(type.hashID),
			strID(type.strID),
			isConst(type.isConst),
			isRef(type.isRef),
			size(type.size),
			isPtr(type.isPtr)
		{}
	public:
		const size_t hashID = 0;
		const std::string_view strID;
		const LuRef::VisitProperty isConst = LuRef::VisitProperty::Non;
		const LuRef::RefProperty isRef = LuRef::RefProperty::Non;
		const size_t size = 0;
		const bool isPtr = 0;

		template<typename T>
		bool isSame()const  {
			return hashID == STypeID<T>::hashID &&
				isConst == STypeID<T>::isConst &&
				isRef == STypeID<T>::isRef;
		}

		template<typename T>
		bool CanConvert()const;

		bool CanConvert(const DTypeID& oth) const {
			return MatchProperty(oth, *this);
		}

		bool operator ==(const DTypeID& oth) const {
			return oth.hashID == this->hashID&&
				isConst == oth.isConst &&
				isRef == oth.isRef;;
		}
		friend class DTypeFlyweight;
	};

	class DTypeFlyweight {
	public:
		template<typename T>
		static const DTypeID& FindRef() {
			if (map.find(STypeID<T>::UUID) == map.end())
				map.emplace(STypeID<T>::UUID, new DTypeID{ Flag<T>{} });
			return *map.at(STypeID<T>::UUID);
		}

		template<typename T>
		static const DTypeID* FindPtr() {
			if (map.find(STypeID<T>::UUID) == map.end())
				map.emplace(STypeID<T>::UUID, new DTypeID{ Flag<T>{} });
			return map.at(STypeID<T>::UUID);
		}
	private:
		inline static std::unordered_map<size_t, DTypeID*> map;
	};

	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////


	class IDRegistry {
	public:
		IDRegistry() {}
		static bool IsRegistered(size_t);
		static const std::string* NameOf(size_t);
		static void Register(size_t hashID, const std::string& alias);
	private:
		inline static std::unordered_map<size_t, std::string> hashToStr;
	};


	template<typename T>
	inline bool DTypeID::CanConvert()const
	{
		return MatchProperty(DTypeFlyweight::FindRef<T>(), *this);
	}
}