#include"Reflect/Dscp.h"
#include<string>
struct Animal {
	std::string name = "none";
	float x=1, y=1;
	virtual void eat() {
		std::cout << "not thing" << std::endl;
	}
	static void* Create() {
		return new Animal{};
	}
};

struct Cat : public Animal {
	std::string color;
	int hp = 200;
	virtual void eat() {
		std::cout << "fish" << std::endl;
	}
	void Move() {
		x += 1, y += 1;
	}
	bool IsHappy(){
		return hp > 100;
	}
	std::pair<float,float> GetPosition() {
		return { x,y };
	}
	void SetName(const std::string& name) {
		this->name = name;
	}
	Cat(const std::string& name){
		this->name = name;
	}
	static void* Create(const std::string& name) {
		return new Cat{ name };
	}
};

static void Register__(){
	{
		LuRef::ClassBuilder<Animal, Cat> buiilderAnimal("Animal");//构造描述类工具类
		buiilderAnimal
			.PushField("name", &Animal::name)
			->PushField("x", &Animal::x)
			->PushField("y", &Animal::y)
			->PushMethod("eat", &Animal::eat)
			->PushStaticMethod("construct", &Animal::Create);
	}
	{
		LuRef::ClassBuilder<Cat> buiilderCat("Cat");//构造描述类工具类
		buiilderCat
			.PushField("color", &Cat::color)
			->PushField("hp", &Cat::hp)
			->PushMethod("eat", &Cat::eat, LuRef::VirtualFunState::Virtual)
			->PushMethod("Move", &Cat::Move)
			->PushMethod("IsHappy", &Cat::IsHappy)
			->PushMethod("GetPosition", &Cat::GetPosition)
			->PushMethod("SetName", &Cat::SetName)
			->PushStaticMethod("construct", &Cat::Create);
	}
}