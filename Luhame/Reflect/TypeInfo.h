#include"TypeID.h"
namespace LuRef {

	struct TypeInfo {
		size_t size;//类型占空间大小
		size_t alignment;//内存对其
		size_t hashID;//hash
		std::string_view strID;//name
	};
}