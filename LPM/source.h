/*
Live Package Manager, Package Manager for LBLive
Copyright (C) <2015>  <xy12423>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
