#include"CCS/LuCcs.h"
#include"test_define.h"
#include"Log/lulog.hpp"
LuCcs::registry reg;
namespace Test_CCS {
	struct vec2 {
		float x, y;
		float get_length() {
			return std::sqrt(x * x + y * y);
		}
	};

	struct animation {
		std::vector<vec2> points;
		void play() {
			for (auto [x, y] : points) {
				LU_CORE_INFO("x:{},y{}", x, y);
			}
		}
	};
	using namespace LuCcs;
	//测试基本的get和emplace
	void test1() {
		carrier_id id = reg.create();
		reg.on_construct<vec2>([](registry& reg, carrier_id id) {
			LU_CORE_INFO("carrier:{} add the vec2 component",id);
		});
		reg.emplace<vec2>(id, 1.0f, 1.0f);
		auto& t = reg.get<vec2>(id);
		LU_CORE_INFO("x:{},y{}", t.x, t.y);
		reg.remove<vec2>(id);
		auto& t1 = reg.get<vec2>(id);
		if(&t1==nullptr)
			LU_CORE_INFO("remove successfully");

		reg.check();
	}

}
#ifdef test_ccs

int main() {
	Test_CCS::test1();
}

#endif // test_ccs
