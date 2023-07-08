#include"Reflect/Util.hpp"
#include"Reflect/Dscp.h"
#include<iostream>

struct TestName {
	int a;
	static int stint;
	std::string b ="sss";
	std::string fun(std::string& str1,const std::string&& str2 ) { 
		return str1 + str2;
	};
	void funSize(std::string str1, std::string str2) {
		std::cout << "str length: " << str1.size() + str2.size();
	};
};

template<class T>
void Traits(T res) {
	std::cout << std::is_reference_v<T> << std::endl;
}

struct Tail {
	std::string name = "defalut";
	int id =0;
	float k = 0;
	Tail(const Tail& oth) {
		this->id = oth.id;
		this->k = oth.k;
		this->name = oth.name;
	}

	Tail(){}
};

struct Vec {
	float x, y;
	Tail tail;
	Vec(float x, float y) {
		this->x = x, this->y = y;
	}
	float Length() {
		return std::sqrt(x * x + y * y);
	}
	Vec normal() {
		return Vec{ x / Length(),y / Length() };
	}
	Vec normal(int) {
		return Vec{ x / Length(),y / Length() };
	}
	Vec Add(Vec& v) {
		std::cout << "invoke Add" << std::endl;
		return { v.x + x,v.y + y };
	}
	Vec operator + (Vec& v) {
		std::cout << "invoke +" << std::endl;
		return { (v.x + x)*2,(v.y + y)*2 };
	}
	static void* Create(float x, float y) {
		return new Vec{ x,y };
	}
};

struct Virtual {
	int math = 1;
	static void* Create() {
		return new Virtual{};
	}
	void printMath() {
		std::cout<<"math: "<<math<<std::endl;
	}
};

struct Shape :public Virtual {
	std::string name{"virtual"};

	bool isIn(std::pair<float,float>) { return true; }
	void PrintName() {
		auto x = this;
		std::cout << "this: " << this << std::endl;
		std::cout << name << std::endl;
	}
	static void* Create() {
		return new Shape{};
	}
};

struct Rectangle :public Shape{
	std::vector<std::pair<float, float>> points;
	Rectangle(std::vector<std::pair<float, float>> points)
		:points(points){
		std::cout << "when construct name is " << name << std::endl;
	}
	virtual bool isIn(std::pair<float, float> p) {
		return p.first > points[0].first && p.first < points[1].first &&
			p.second>points[2].second && p.second < points[3].second;
	}
	static void* Create(std::vector<std::pair<float, float>> p) {
		return new Rectangle{p};
	}
};

struct PStr {
	std::vector<std::string> strs;
	void print() {
		for (auto& t : strs)
			std::cout << t << ' ';
		std::cout << std::endl;
	}
	void reverse() {
		std::reverse(strs.begin(), strs.end());
	}
	void push(const std::string& str) {
		strs.push_back(str);
	}
	void push(const std::string_view& str) {
		strs.push_back(std::string{str});
	}
	static void* Create(const std::vector<std::string>& strs) {
		auto res = new PStr{};
		res->strs = strs;
		return res;
	}
};

void Register(){
	{
		LuRef::ClassBuilder<Vec> builder{"Vec"};
		builder.PushField("x", &Vec::x)
			->PushField("y", &Vec::y)
			->PushField("tail", &Vec::tail)
			->PushMethod("Length", &Vec::Length)
			->PushMethod("normal", LuRef::FunPtr<Vec, Vec, int>(&Vec::normal))
			->PushMethod("Add", LuRef::FunPtr<Vec, Vec, Vec&>(&Vec::Add))
			->PushMethod("+", LuRef::FunPtr<Vec, Vec, Vec&>(&Vec::operator+))
			->PushStaticMethod("construct", LuRef::StaticFunPtr<void*, float, float>(&Vec::Create));
	}
	{
		LuRef::ClassBuilder<PStr> builder_{"PStr"};
		builder_.PushField("strs", &PStr::strs)
			->PushMethod("print", &PStr::print)
			->PushMethod("push", LuRef::FunPtr<PStr, void, const std::string&>(&PStr::push))
			->PushMethod("push_view", LuRef::FunPtr<PStr, void, const std::string_view& >(&PStr::push))
			->PushMethod("reverse", LuRef::FunPtr<PStr, void>(&PStr::reverse))
			->PushStaticMethod("construct", &PStr::Create);
	}
	{
		LuRef::ClassBuilder<Virtual, Shape, Rectangle> builderVirtual{"Virtual"};
		builderVirtual.PushField("math", &Virtual::math)
			->PushStaticMethod("construct", &Virtual::Create)
			->PushMethod("PrintMath", &Virtual::printMath);
	}
	{
		LuRef::ClassBuilder<Shape, Rectangle> builderShape{"Shape"};
		builderShape.PushField("name", &Shape::name)
			->PushMethod("isIn", &Shape::isIn, LuRef::VirtualFunState::Virtual)
			->PushMethod("PrintName", &Shape::PrintName, LuRef::VirtualFunState::NotVirtual)
			->PushStaticMethod("construct", &Shape::Create);
	}
	{
		LuRef::ClassBuilder<Rectangle> builderRectangle{"Rectangle"};
		builderRectangle
			.PushField("points", &Rectangle::points)
			->PushMethod("isIn", &Rectangle::isIn, LuRef::VirtualFunState::Virtual)
			->PushStaticMethod("construct", &Rectangle::Create);
	}
}

#include"Readme.cpp"


void test1() {
	auto obj = LuRef::ClassManager::FindClass("Vec")->Instantiate(1.0f, 1.0f);
	auto data = obj->As<Vec>();
	//测试拷贝字段的通用性
	obj->SetFieldWithAlias("tail", Tail{});
	std::cout << data->tail.id << " " << data->tail.k << " " << data->tail.name << std::endl;
	//调用无参函数
	float length = 0;
	float* ptrl = &length;
	obj->InvokeMember("Length", ptrl);
	std::cout << length << std::endl;
	//调用有参函数

	Vec v{ 1,1 };
	Vec res{ 0,0 };
	obj->InvokeMember("Add", &res, v);
	std::cout << res.x << " " << res.y << std::endl;

	//调用重载函数符

	Vec v_{ 3,4 };
	Vec res_{ 0,0 };
	obj->InvokeMember("+", &res_, v_);
	std::cout << res_.x << " " << res_.y << std::endl;

	//运行时动态类型检测
	Vec v_1{ 3,4 };
	Vec res_1{ 0,0 };
	auto x = LuRef::STypeID<const Vec&>::isConst;
	obj->InvokeMember("+", &res_1, v_1);

	//测试多个类
	std::vector<std::string> myName{"liu", "huan", "ming"};
	auto pstr = LuRef::ClassManager::FindClass("PStr")->Instantiate(myName);
	std::string love = "love";
	std::string cpp = "cpp";
	std::string_view view{cpp};
	pstr->InvokeMember("push", LuRef::voidPtr, love);
	pstr->InvokeMember("push_view", LuRef::voidPtr, view);
	pstr->InvokeMember("print", LuRef::voidPtr);
	pstr->InvokeMember("reverse", LuRef::voidPtr);
	pstr->InvokeMember("print", LuRef::voidPtr);

	//测试继承关系
	std::cout << "Rectangle base on Shape :" << LuRef::Inheritance::IsBaseOf("Shape", "Rectangle") << std::endl;
	std::cout << "Shape base on Rectangle :" << LuRef::Inheritance::IsBaseOf("Rectangle", "Shape") << std::endl;
	std::cout << "Rectangle base on Virtual :" << LuRef::Inheritance::IsBaseOf("Virtual", "Rectangle") << std::endl;

	//测试通过继承关系获取的子类信息
	auto rectangle = LuRef::ClassManager::FindClass("Rectangle")->Instantiate(
		std::vector<std::pair<float, float>>{{-1.0f, 0}, { 1.0f,0 }, { 0,-1.0f }, { 0,1.0f }, { 0.0f,0 }}
	);
	bool intersectRes = false;
	rectangle->InvokeMember("isIn", &intersectRes, std::pair<float, float>{-1000, 0});
	std::cout << "isIn: " << intersectRes << std::endl;
	std::cout << "before set field" << std::endl;
	rectangle->InvokeMember("PrintName", LuRef::voidPtr);
	rectangle->InvokeMember("PrintMath", LuRef::voidPtr);
	rectangle->SetFieldWithAlias("name", std::string{"liu huan ming"});
	rectangle->SetFieldWithAlias("math", 10086);
	std::cout << "after set field" << std::endl;
	rectangle->InvokeMember("PrintName", LuRef::voidPtr);
	rectangle->InvokeMember("PrintMath", LuRef::voidPtr);
}

void test2() {

	auto animal = LuRef::ClassManager::FindClass("Animal")->Instantiate();
	animal->SetFieldWithAlias("name", std::string{"bob"});
	std::cout << animal->As<Animal>()->name << std::endl;
	animal->InvokeMember("eat", LuRef::voidPtr);

	auto cat = LuRef::ClassManager::FindClass("Cat")->Instantiate(std::string{"ana"});
	cat->SetFieldWithAlias("name", std::string("ash"));
	std::cout << cat->As<Cat>()->name << std::endl;
	cat->InvokeMember("Move",LuRef::voidPtr);
	std::pair<float, float> pos;
	cat->InvokeMember("GetPosition", &pos);
	std::cout << "pos: " << pos.first << " " << pos.second << std::endl;
	cat->InvokeMember("eat", LuRef::voidPtr);
}

int main() {
	Vec(Vec::*ptr)(int) = &Vec::normal;
	Register();
	Register__();

	test2();


	//LuRef::DTypeFlyweight::FindRef<int>().hashID;
	/*
	TestName ts;
	LuRef::Method me{&TestName::fun};
	LuRef::Method me2{&TestName::funSize};
	std::vector<int*> a;
	const auto& t = LuRef::DTypeFlyweight::FindRef<int>();
	std::string res = "error";
	std::string para = "xxx";
	std::string para2 = "sss";
	std::cout << LuRef::STypeID<std::string&>::strID << std::endl;
	Traits(std::ref(para));
	try {
		me2.invoke(
			&ts, LuRef::voidPtr, para, std::move(para2)
		);
	}
	catch (std::exception e) {
		std::cout << e.what();
	}
	std::cout << "\n" << res;
	*/
}