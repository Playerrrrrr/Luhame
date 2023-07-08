#include "Alias.h"

void ClassAlias::Register(const std::string& alias, const size_t hashID){
	auto it = hashIDToAlias.find(hashID);
	if (it != hashIDToAlias.end())
		return;
	hashIDToAlias.emplace(hashID, alias);
	aliasToHashID.emplace(alias,hashID);
}

size_t* ClassAlias::Find(const std::string& alias) {
	auto it = aliasToHashID.find(alias);
	if (it == aliasToHashID.end())
		return nullptr;
	return &it->second;
}

const std::string* ClassAlias::Find(size_t hashID) {
	auto it = hashIDToAlias.find(hashID);
	if (it == hashIDToAlias.end())
		return nullptr;
	return &it->second;
}
