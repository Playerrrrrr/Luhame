#pragma once
#include<unordered_map>
#include<string>
class ClassAlias{
public:
	static void Register(const std::string& alias,const size_t hashID);
	//查询成功返回指针，否则nullptr
	static size_t* Find(const std::string&)  ;
	static const std::string* Find(size_t) ;
private:
	inline static std::unordered_map<std::string, size_t> aliasToHashID;
	inline static std::unordered_map<size_t, std::string> hashIDToAlias;
};

