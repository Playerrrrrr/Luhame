#include"Reflect/Util.hpp"
#include"glm/glm.hpp"
#include"Reflect/Dscp.h"
#include<iostream>
#include"yaml-cpp/yaml.h"
#include"Reflect/ClassBuilder.hpp"
#include<fstream>

#include"Log/lulog.hpp"
#include"test_define.h"
#include"imgui/imgui.h"
#include"GLFW/glfw3.h"
#include"imgui/backends/imgui_impl_sdl2.h"
#include"imgui/backends/imgui_impl_dx11.h"
#include "SDL/SDL.h"
#include"SDL/SDL_syswm.h"
#include <d3d11.h>
#include"ImGuiWrapper/Drags.h"
#include"ImGuiWrapper/ImGuiSearchBar.h"


bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();



struct TestName {
	int a;
	static int stint;
	std::string b = "sss";
	std::string fun(std::string& str1, const std::string&& str2) {
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

static int x = 0;

struct Subsub {
	std::string name = "Subsub" + std::to_string(x++);
	Subsub(std::string name) :name{ name } {}
	Subsub() {}
	static void* Create() {
		return new Subsub{};
	}
};

struct Sub {
	inline static int x = 0;
	std::unordered_map<int, Subsub> map;

	Sub() {
		for (int i = 0; i < 4; i++) {
			map.emplace(x++, Subsub{ "Subsub" + std::to_string(x++) });
		}
	}

	static Sub* Create() {
		auto res = new Sub{};
		res->map.emplace(x++, Subsub{ "Subsub" + std::to_string(x++) });
		return res;
	}
};

struct Tail {
	std::string name = "defalut";
	int id = 0;
	float k = 0;
	std::vector<Sub> subs{Sub{}, Sub{}, Sub{}};
	Tail(const Tail& oth) {
		this->id = oth.id;
		this->k = oth.k;
		this->name = oth.name;
	}

	Tail() {}

	static void* Create() {
		return new Tail{};
	}
};

struct Vec {
	float x = 0, y = 0;
	Tail tail;
	std::vector<glm::vec3> vecs;
	Vec() = default;
	Vec(float x, float y) {
		this->x = x, this->y = y;
		for (int i = 0; i < 3; i++)
			vecs.push_back(glm::vec3{float(i)});
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
		return { (v.x + x) * 2,(v.y + y) * 2 };
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
		std::cout << "math: " << math << std::endl;
	}
};

struct Shape :public Virtual {
	std::string name{"virtual"};

	bool isIn(std::pair<float, float>) {
		LU_CORE_WARN("in Shape");
		return true;
	}
	void PrintName() {
		auto x = this;
		std::cout << "this: " << this << std::endl;
		std::cout << name << std::endl;
	}
	static void* Create() {
		return new Shape{};
	}
};

struct Rectangle_ :public Shape {
	std::vector<std::pair<float, float>> points;
	Rectangle_(std::vector<std::pair<float, float>> points)
		:points(points) {
		std::cout << "when construct name is " << name << std::endl;
	}
	virtual bool isIn(std::pair<float, float> p) {
		LU_CORE_WARN("in Rectangle_");
		return p.first > points[0].first && p.first < points[1].first &&
			p.second>points[2].second && p.second < points[3].second;
	}
	static void* Create(std::vector<std::pair<float, float>> p) {
		return new Rectangle_{ p };
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

void funptr(int (*ptr)()) {

}

void invoke() {
	std::function<int()> x;
	funptr(x.target<int()>());
}

void Register() {

	{
		LuRef::ClassBuilder<Subsub> builder{"Subsub", true};
		builder.PushField("name", &Subsub::name)
			->PushStaticMethod("construct", &Subsub::Create);
	}

	{
		LuRef::ClassBuilder<Sub> builder{"Sub", true};
		builder.PushField("map", &Sub::map)
			->PushStaticMethod("construct", &Sub::Create);
	}
	{
		LuRef::ClassBuilder<Tail> builder{"Tail", true};
		builder.PushField("id", &Tail::id)
			->PushField("name", &Tail::name)
			->PushField("k", &Tail::k)
			->PushField("subs", &Tail::subs)
			->PushStaticMethod("construct", &Tail::Create);
	}
	{
		LuRef::ClassBuilder<Vec> builder{"Vec", true};
		builder.PushField("x", &Vec::x)
			->PushField("y", &Vec::y)
			->PushField("tail", &Vec::tail)
			->PushField("vecs", &Vec::vecs)
			->PushMethod("Length", &Vec::Length)
			->PushMethod("normal", LuRef::FunPtr<Vec, Vec, int>(&Vec::normal))
			->PushMethod("Add", LuRef::FunPtr<Vec, Vec, Vec&>(&Vec::Add))
			->PushMethod("+", LuRef::FunPtr<Vec, Vec, Vec&>(&Vec::operator+))
			->PushStaticMethod("construct", LuRef::StaticFunPtr<void*, float, float>(&Vec::Create));
	}
	{
		LuRef::ClassBuilder<PStr> builder_{"PStr", true};
		builder_.PushField("strs", &PStr::strs)
			->PushMethod("print", &PStr::print)
			->PushMethod("push", LuRef::FunPtr<PStr, void, const std::string&>(&PStr::push))
			->PushMethod("push_view", LuRef::FunPtr<PStr, void, const std::string_view& >(&PStr::push))
			->PushMethod("reverse", LuRef::FunPtr<PStr, void>(&PStr::reverse))
			->PushStaticMethod("construct", &PStr::Create);
	}
	{
		LuRef::ClassBuilder<Virtual, Shape, Rectangle_> builderVirtual{"Virtual", true};
		builderVirtual.PushField("math", &Virtual::math)
			->PushStaticMethod("construct", &Virtual::Create)
			->PushMethod("PrintMath", &Virtual::printMath);
	}
	{
		LuRef::ClassBuilder<Shape, Rectangle_> builderShape{"Shape", true};
		builderShape.PushField("name", &Shape::name)
			->PushMethod("isIn", &Shape::isIn, LuRef::VirtualFunState::Virtual)
			->PushMethod("PrintName", &Shape::PrintName, LuRef::VirtualFunState::NotVirtual)
			->PushStaticMethod("construct", &Shape::Create);
	}
	{
		LuRef::ClassBuilder<Rectangle_> builderRectangle{"Rectangle", true};
		builderRectangle
			.PushField("points", &Rectangle_::points)
			->PushMethod("isIn", &Rectangle_::isIn, LuRef::VirtualFunState::Virtual)
			->PushStaticMethod("construct", &Rectangle_::Create);
	}
	{
		struct CreateGlmVec3 {
			static void* create(float a, float b, float c) {
				return new glm::vec3{ a,b,c };
			}
		};
		LuRef::ClassBuilder<glm::vec3> builder("vec3", true);
		builder.PushField("x", &glm::vec3::x)
			->PushField("y", &glm::vec3::y)
			->PushField("z", &glm::vec3::z)
			->PushStaticMethod("length", &glm::vec3::length)
			->PushStaticMethod("construct", CreateGlmVec3::create);
	}
}

#include"Readme.cpp"
void test1() {
	auto obj = LuRef::ClassManager::FindClass("Vec")->MakeShared(1.0f, 1.0f);
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
	auto pstr = LuRef::ClassManager::FindClass("PStr")->MakeShared(myName);
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
	auto rectangle = LuRef::ClassManager::FindClass("Rectangle")->MakeShared(
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

	auto animal = LuRef::ClassManager::FindClass("Animal")->MakeShared();
	animal->SetFieldWithAlias("name", std::string{"bob"});
	std::cout << animal->As<Animal>()->name << std::endl;
	animal->InvokeMember("eat", LuRef::voidPtr);

	auto cat = LuRef::ClassManager::FindClass("Cat")->MakeShared(std::string{"ana"});
	cat->SetFieldWithAlias("name", std::string("ash"));
	std::cout << cat->As<Cat>()->name << std::endl;
	cat->InvokeMember("Move", LuRef::voidPtr);
	std::pair<float, float> pos;
	cat->InvokeMember("GetPosition", &pos);
	std::cout << "pos: " << pos.first << " " << pos.second << std::endl;
	cat->InvokeMember("eat", LuRef::voidPtr);

}

void Nodes(YAML::Node) {}

//测试序列化系统
void test3() {
	//创建对象
	YAML::Node node;
	auto cat = LuRef::ClassManager::FindClass("Cat")->MakeShared(std::string("bob"));
	YAML::Node catSubnode = node["cat"];
	cat->SetFieldWithAlias("color", std::string("red"));
	auto catVec = LuRef::ClassManager::FindClass("Cat")->MakeWithData(cat->As<Cat>());
	//序列化对象
	LuRef::SerializationManager::Serialize(catSubnode, *catVec);

	auto vec = LuRef::ClassManager::FindClass("Vec")->MakeShared(876942392.f, 10086.f);
	YAML::Node vecSubnode = node["vec"];
	auto serVec = LuRef::ClassManager::FindClass("Vec")->MakeWithData(vec->As<Vec>());
	LuRef::SerializationManager::Serialize(vecSubnode, *serVec);

	auto data_vec = node["data_vec"];
	Vec* vec_data = new Vec{ 1,1 };
	LuRef::SerializationManager::SerializeWithData(data_vec, vec_data);
	std::ofstream os("output.yaml");
	YAML::Emitter emitter;
	emitter << node;
	os << emitter.c_str();
}

//测试反序列化系统
void test4() {
	YAML::Node node = YAML::LoadFile("./output.yaml");
	Vec vec;
	vec.tail.subs.clear();
	vec.x = -1000;
	vec.y = -1000;
	auto data = node["data_vec"];
	LuRef::SerializationManager::DeserializeWithData(data, &vec);
}

//测试Object类型转换
void test5() {
	std::shared_ptr<LuRef::SharedObject> shape;
	std::pair<float, float> pair{1.0f, 1.0f};
	{
		auto rectangle = LuRef::ClassManager::FindClass("Rectangle")->MakeShared(
			std::vector<std::pair<float, float>>{{-1.0f, 0}, { 1.0f,0 }, { 0,-1.0f }, { 0,1.0f }, { 0.0f,0 }}
		);
		bool res;
		rectangle->InvokeMember("isIn", &res, pair);
		LU_CORE_INFO("{}", LuRef::Inheritance::IsBaseOf("Shape", "Rectangle"));
		LU_CORE_INFO("{}", LuRef::Inheritance::IsBaseOf("Rectangle", "Shape"));
		shape = LuRef::Inheritance::Convert(rectangle, "Shape");

	}
	shape->InvokeMember("isIn", LuRef::voidPtr, pair);
	shape = LuRef::ClassManager::FindClass("Shape")->MakeShared();
	shape->InvokeMember("isIn", LuRef::voidPtr, pair);

}

//测试序列化是否能与外部库兼容
void test_6() {
	std::ofstream os("glm_output.yaml");
	auto vec3_ = LuRef::ClassManager::FindClass("vec3")->MakeShared(1.0f, 2.0f, 3.0f);
	glm::length_t len;
	vec3_->InvokeStatic("length", &len);
	YAML::Node node;
	YAML::Node vec3_node = node["vec3"];
	LuRef::SerializationManager::Serialize(vec3_node, *vec3_.get());
	YAML::Emitter emt;
	emt << node;
	os << emt.c_str();
	os.close();
}
//测试反序列化是否能与外部库兼容
void test_6_() {
	YAML::Node node = YAML::LoadFile("./glm_output.yaml");
	glm::vec3 vec;
	YAML::Node nodedata = node["vec3"];
	LuRef::SerializationManager::DeserializeWithData(nodedata, &vec);
	std::cin >> vec.x;
}


class test_frame {
#define Def inline static
	Def ImGuiWrapper::drag_vec<float,3> drag_bar_3;
	Def float lab_num[5];
	Def ImGuiWrapper::drag_vec<float,5> drag_bar_5;
	Def glm::vec3 vecs[4];
	Def std::shared_ptr<ImGuiWrapper::drag_vec<float, 3>> be_searchs[4];
	Def ImGuiWrapper::ImGuiSearchBar search_bar{"search vec"};
	Def ImGuiWrapper::ImGuiWindow window{"test"};
	Def glm::vec3 vec{0, 0, 0};
	Def ID3D11Device* g_pd3dDevice = nullptr;
	Def ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
	Def IDXGISwapChain* g_pSwapChain = nullptr;
	Def ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;



#undef Def
public:
	std::function<void()> funs;
	static int test_frame_for_Imgui() {


		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
		{
			printf("Error: %s\n", SDL_GetError());
			return -1;
		}

		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+DirectX11 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(window, &wmInfo);
		HWND hwnd = (HWND)wmInfo.info.win.window;

		if (!CreateDeviceD3D(hwnd))
		{
			CleanupDeviceD3D();
			return 1;
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		ImGui_ImplSDL2_InitForD3D(window);
		ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

		bool show_demo_window = true;
		bool show_another_window = false;
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		bool done = false;
		test_frame::init();
		while (!done)
		{
			// Poll and handle events (inputs, window resize, etc.)
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				ImGui_ImplSDL2_ProcessEvent(&event);
				if (event.type == SDL_QUIT)
					done = true;
				if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
					done = true;
				if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED && event.window.windowID == SDL_GetWindowID(window))
				{
					// Release all outstanding references to the swap chain's buffers before resizing.
					CleanupRenderTarget();
					g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
					CreateRenderTarget();
				}
			}

			// Start the Dear ImGui frame
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();


			test_frame::test_7();


			// Rendering
			ImGui::Render();
			const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
			g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
			g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			// Update and Render additional Platform Windows
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}

			g_pSwapChain->Present(1, 0); // Present with vsync
			//g_pSwapChain->Present(0, 0); // Present without vsync
		}

		// Cleanup
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();

		CleanupDeviceD3D();
		SDL_DestroyWindow(window);
		SDL_Quit();

		return 1;
	}
	static bool CreateDeviceD3D(HWND hWnd)
	{
		// Setup swap chain
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 2;
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		UINT createDeviceFlags = 0;
		//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
		if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
			return false;

		CreateRenderTarget();
		return true;
	}
	static void CleanupDeviceD3D()
	{
		CleanupRenderTarget();
		if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
		if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
		if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
	}
	static void CreateRenderTarget()
	{
		ID3D11Texture2D* pBackBuffer;
		g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
	static void CleanupRenderTarget()
	{
		if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
	}

	static void init() {
		drag_bar_3.push_vec(&vec.x);
		drag_bar_5.push_vec(lab_num);
		for (int i = 0; i < 4; i++) {
			be_searchs[i].reset(new ImGuiWrapper::drag_vec<float, 3>{});
			be_searchs[i]->push_vec(&vecs[i].x);
		}
		search_bar.push_kv("luhame", be_searchs[0]);
		search_bar.push_kv("cherno", be_searchs[1]);
		search_bar.push_kv("CCS", be_searchs[2]);
		search_bar.push_kv("Reflect", be_searchs[3]);
		window.push_component(std::shared_ptr<ImGuiWrapper::Component>{&drag_bar_3});
		window.push_component(std::shared_ptr<ImGuiWrapper::Component>{&drag_bar_5});
		window.push_component(std::shared_ptr<ImGuiWrapper::Component>{&search_bar});
	}
	static void test_7() {
		window.update();
	}
};

//测试图形



void set_function_for_imgui() {

}
//测试自定义的ImGui组件


#undef  main;
#ifdef test_ref

int main() {
	Vec(Vec:: * ptr)(int) = &Vec::normal;
	Register();
	Register__();
	test_frame::test_frame_for_Imgui();
	return 0;
}
#endif // test_ref



