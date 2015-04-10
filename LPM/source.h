#pragma once

#ifndef _H_SRC
#define _H_SRC

#include "errInfo.h"

typedef std::vector<package*> pakListTp;

class source
{
public:
	source(std::string _add){ add = _add; if (add.back() == '/') add.pop_back(); };
	~source(){ std::for_each(pkgList.begin(), pkgList.end(), [this](package* arg){ delete arg; }); };
	std::string getAdd(){ return add; };
	errInfo loadRemote();
	void loadLocal(pakListTp &_pkgList);
	package* find_package(std::string name);
	errInfo upgradeAll();

	friend void writeSource();
#ifdef _LPM_GUI
	friend void getSrcNameList(wxArrayString &ret);
	friend void getPakList(source *src, pakListTp &ret);
#else
	friend void printAvailable(source *src, bool ignoreInstalled);
	friend void printAvailableShort(source *src, bool ignoreInstalled);
#endif
private:
	std::string add;
	pakListTp pkgList;
	std::unordered_map<std::string, int> pkgMap;
};

typedef std::vector<source*> srcListTp;
extern srcListTp sourceList;

#endif
