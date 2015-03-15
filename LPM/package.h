#pragma once

#ifndef _H_PKG
#define _H_PKG

#include "errInfo.h"
#include "globalVar.h"

struct pakExtInfo
{
	pakExtInfo(){};
	pakExtInfo(std::string &_fname, std::string &_author, std::string &_info){ fname = _fname; author = _author; info = _info; };
	std::string fname, author, info;
};

struct version
{
	version(){ major = 0; minor = 0; revision = 0; };
	version(UINT _major, USHORT _minor, USHORT _revision){ major = _major; minor = _minor; revision = _revision; };
	version(const std::string &str);
	UINT major;
	USHORT minor, revision;
	friend inline bool operator>(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? false : a.revision > b.revision) : a.minor > b.minor) : a.major > b.major; };
	friend inline bool operator>=(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? true : a.revision > b.revision) : a.minor > b.minor) : a.major > b.major; };
	friend inline bool operator<(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? false : a.revision < b.revision) : a.minor < b.minor) : a.major < b.major; };
	friend inline bool operator<=(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? true : a.revision < b.revision) : a.minor < b.minor) : a.major < b.major; };
};

typedef std::list<std::string> depListTp;
typedef std::unordered_set<std::string> depMapTp;

class package
{
public:
	package(std::string _source, std::string &_name, version _ver, depListTp &_depList, depListTp &_confList, pakExtInfo _extInfo);
	std::string getName(){ return name; };
	version getVer(){ return ver; };
	errInfo upgrade();
	bool check();

	friend void writeSource();
	friend void printInfo(package *pkg);
	friend errInfo install(std::string name);
private:
	errInfo inst();
	int instScript(bool upgrade = false);
	std::string source;
	std::string name;
	version ver;
	pakExtInfo extInfo;
	depListTp depList;
	depListTp confList;
};

package* find_package(const std::string &name);
bool is_installed(std::string name);
errInfo install(std::string name);
errInfo uninstall(std::string name, bool upgrade = false);

#endif
