#include"TypeID.h"
namespace LuRef {

	struct TypeInfo {
		size_t size;//����ռ�ռ��С
		size_t alignment;//�ڴ����
		size_t hashID;//hash
		std::string_view strID;//name
	};
}