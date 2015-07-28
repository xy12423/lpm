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

#ifndef _H_PKG
#define _H_PKG

#include "global.h"

class pakExtInfo
{
public:
	pakExtInfo(){};
	pakExtInfo(const std::wstring &_fname, const  std::wstring &_author, const  std::wstring &_info) :fname(_fname), author(_author), info(_info){};
	pakExtInfo(const std::string &_fname, const  std::string &_author, const std::string &_info){
		wxWCharBuffer tmp = wxConvUTF8.cMB2WC(_fname.c_str());
		fname.assign(tmp.data(), tmp.length());
		tmp = wxConvUTF8.cMB2WC(_author.c_str());
		author.assign(tmp.data(), tmp.length());
		tmp = wxConvUTF8.cMB2WC(_info.c_str());
		info.assign(tmp.data(), tmp.length());
	};

	std::string getFName() const{ wxCharBuffer tmp = wxConvUTF8.cWC2MB(fname.c_str()); return std::string(tmp.data(), tmp.length()); }
	std::string getAuthor() const{ wxCharBuffer tmp = wxConvUTF8.cWC2MB(author.c_str()); return std::string(tmp.data(), tmp.length()); }
	std::string getInfo() const{ wxCharBuffer tmp = wxConvUTF8.cWC2MB(info.c_str()); return std::string(tmp.data(), tmp.length()); }

	std::string getFName_Local() const{ wxCharBuffer tmp = wxConvLocal.cWC2MB(fname.c_str()); return std::string(tmp.data(), tmp.length()); }
	std::string getAuthor_Local() const{ wxCharBuffer tmp = wxConvLocal.cWC2MB(author.c_str()); return std::string(tmp.data(), tmp.length()); }
	std::string getInfo_Local() const{ wxCharBuffer tmp = wxConvLocal.cWC2MB(info.c_str()); return std::string(tmp.data(), tmp.length()); }

	const std::wstring& getFNameW() const{ return fname; }
	const std::wstring& getAuthorW() const{ return author; }
	const std::wstring& getInfoW() const{ return info; }
private:
	std::wstring fname, author, info;
};

struct version
{
	version(){ major = 0; minor = 0; revision = 0; };
	version(UINT _major, USHORT _minor, USHORT _revision){ major = _major; minor = _minor; revision = _revision; };
	version(const std::string &str);
	UINT major;
	USHORT minor, revision;
	std::string toStr(){ return std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(revision); };
	friend inline bool operator>(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? false : a.revision > b.revision) : a.minor > b.minor) : a.major > b.major; };
	friend inline bool operator>=(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? true : a.revision > b.revision) : a.minor > b.minor) : a.major > b.major; };
	friend inline bool operator<(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? false : a.revision < b.revision) : a.minor < b.minor) : a.major < b.major; };
	friend inline bool operator<=(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? true : a.revision < b.revision) : a.minor < b.minor) : a.major < b.major; };
	friend inline bool operator==(const version &a, const version &b){ return a.major == b.major && a.minor == b.minor && a.revision == b.revision; };
	friend inline bool operator!=(const version &a, const version &b){ return a.major != b.major || a.minor != b.minor || a.revision != b.revision; };
};

struct depInfo
{
	enum verCon{ ALL, BIGGER, BIGEQU, LESS, LESEQU, EQU, NEQU, NONE };
	depInfo(){ con = ALL; };
	depInfo(const std::string _name, version _ver, verCon _con, bool _isDep) :name(_name), ver(_ver), con(_con){ isDep = _isDep; };
	depInfo(std::string _str);
	std::string name;
	version ver;
	verCon con;
	bool isDep;

	std::string conStr();
	std::string fullStr();
	bool check(version _ver);
	bool check();

	inline depInfo operator~(){
		static const depInfo::verCon transTable[] = { depInfo::NONE, depInfo::LESEQU, depInfo::LESS, depInfo::BIGEQU, depInfo::BIGGER, depInfo::NEQU, depInfo::EQU, depInfo::ALL };
		return depInfo(name, ver, transTable[con], isDep);
	};
};
typedef std::list<depInfo> depListTp;

class package
{
public:
	struct instItem
	{
		enum opType{ INST, UPG, REMOVE };
		instItem(){ pak = NULL; }
		instItem(package *_pak, opType _oper) :name(_pak->name){ pak = _pak; oper = _oper; };
		instItem(const std::string &_name) :name(_name){ oper = REMOVE; };
		package* pak;
		std::string name;
		opType oper;
	};
	typedef std::list<instItem> pakIListTp;

	package(const std::string &_source, const std::string &_name, version _ver, depListTp &_depList, depListTp &_confList, pakExtInfo _extInfo) :
		source(_source), name(_name), ver(_ver), depList(_depList), confList(_confList), extInfo(_extInfo)
	{};

	const std::string &getName() const{ return name; };
	const pakExtInfo &getExtInfo() const{ return extInfo; };
	version getVer() const{ return ver; };

	//Check requirements then install them using instList
	errInfo instFull(bool force = false);
	//Try to upgrade the package
	errInfo upgrade(bool checked = false, bool force = false);

	bool needUpgrade();
	bool check();

	friend void writeSource();
	friend void printInfo(package *pkg);
private:
	//Install packages
	errInfo instList(pakIListTp &instList);
	//Install package without checking requirements
	errInfo inst();
	//Check if requirements are OK
	void checkDep(pakIListTp &instList, const depListTp &extraDep, bool force = false);
	//Run post-installation script
	int instScript(bool upgrade = false);

	std::string source;
	std::string name;
	version ver;
	pakExtInfo extInfo;
	depListTp depList;
	depListTp confList;
};
typedef std::vector<package*> pakListTp;

enum remove_level{
	REMOVE_NORMAL,
	REMOVE_FORCE,
	REMOVE_RECURSIVE
};
package* find_package(const std::string &name);
package* find_package(const std::string &name, depInfo con);
package* find_package(const std::string &name, std::unordered_multimap<int, depInfo> &con, bool force, std::list<int> &removeList);
bool is_installed(const std::string &name);
version cur_version(const std::string &name);
errInfo install(const std::string &name, bool force = false);
errInfo uninstall(const std::string &name, bool upgrade = false, remove_level level = REMOVE_NORMAL);
errInfo backup(const std::string &name, bool force = false);
errInfo recover_from_backup(const std::string &name);

#endif
