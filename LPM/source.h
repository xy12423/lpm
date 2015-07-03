#pragma once

#ifndef _H_SRC
#define _H_SRC

class source
{
public:
	source(std::string _add){ add = _add; if (add.back() == '/') add.pop_back(); };
	~source(){ std::for_each(pakList.begin(), pakList.end(), [this](package* arg){ delete arg; }); };

	std::string getAdd(){ return add; };

	//Load source info from remote server
	errInfo loadRemote();
	//Load source info from local
	void loadLocal(pakListTp &_pakList);
	//Find package in source
	package* find_package(std::string name);
	//Check upgrade in source and add them to ret
	void checkUpgrade(pakListTp &ret);
	//Upgrade all package in source
	void upgradeAll();

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
	pakListTp pakList;
	std::unordered_map<std::string, int> pakMap;
};

typedef std::vector<source*> srcListTp;
extern srcListTp sourceList;

#endif
