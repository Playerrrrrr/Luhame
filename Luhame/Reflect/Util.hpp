#pragma once
#include"TypeID.h"
#include<functional>
#include<iostream>
#include"yaml-cpp/node/convert.h"

namespace LuRef {

#define LU_LOG_WARN(x)\
	std::cout<<"waring: "<<x<<std::endl;
#define LU_LOG_ERROR(x)\
	std::cout<<"waring: "<<x<<std::endl;

	using Offsetor = std::function<void* (void*)>;

	template<typename C, typename T>
	std::size_t FieldOffset(T C::* ptr) noexcept {
		return reinterpret_cast<std::size_t>(
			(&(reinterpret_cast<C const volatile*>(nullptr)->*ptr))
			);
	}
	template<typename C,typename T>
	constexpr auto FieldOffsetor(T C::* fieldPtr) noexcept {
		return [fieldPtr](void* objPtr)noexcept-> void* {
			return &(reinterpret_cast<C*>(objPtr)->*fieldPtr);
		};
	}
	template<typename C, typename R,typename... Args>
	auto FunPtr(R(C::* value)(Args...)) {
		std::vector<std::pair<VisitProperty, std::string>> test;
		(
			test.push_back({ STypeID<Args>::isConst, STypeID<Args>::strID })
			, ...);
		return std::forward<decltype(value)>(value);
	}
	template<typename R, typename... Args>
	auto StaticFunPtr(R(* value)(Args...)) {
		return value;
	}
	static void* voidPtr = nullptr;


	template<typename T>
	struct FunTrait{};

	template<typename C_,typename R_,typename ...Args_>
	struct FunTrait<R_(C_::*)(Args_...)> {
		using C = C_;
		using R = R_;
		using Args = std::tuple < Args_...>;
	};

	template<typename C_, typename R_>
	struct FunTrait<R_(C_::*)()> {
		using C = C_;
		using R = R_;
	};

	template<typename R_, typename ...Args_>
	struct FunTrait<R_(*)(Args_...)> {
		using R = R_;
		using Args = std::tuple<Args_...>;
	};

	template<typename R_>
	struct FunTrait<R_(*)()> {
		using R = R_;
	};


	template<typename T>
	struct IsMemberFun{};

	template<typename T,typename C,typename...Args>
	struct IsMemberFun <T(C::*)(Args...)> {
		inline static const constexpr bool value = true;
	};
	template<typename T, typename C, typename...Args>
	struct IsMemberFun <T(C::*)(Args...)const> {
		inline static const constexpr bool value = true;
	};

	template<typename T, typename...Args>
	struct IsMemberFun <T(*)(Args...)> {
		inline static const constexpr bool value = false;
	};
	template<typename T, typename...Args>
	struct IsMemberFun <const T(*)(Args...)> {
		inline static const constexpr bool value = false;
	};




}