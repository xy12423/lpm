#pragma once

#ifndef _H_SRC
#define _H_SRC

#include "errInfo.h"

class source
{
public:
	source(std::string _add){ add = _add; if (add.back() == '/') add.pop_back(); };
	std::string getAdd(){ return add; };
	errInfo loadRemote();
	void loadLocal(std::vector<package*> &_pkgList);
	package* find_package(std::string name);
	errInfo upgradeAll();

	friend void writeSource();
	friend void printAvaliable(source *src, bool ignoreInstalled);
	friend void printAvaliableShort(source *src, bool ignoreInstalled);
private:
	std::string add;
	std::vector<package*> pkgList;
	std::unordered_map<std::string, int> pkgMap;
};

extern std::vector<source*> sourceList;

#endif
