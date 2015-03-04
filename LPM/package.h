#pragma once

#ifndef _H_PKG
#define _H_PKG

#include "errInfo.h"
#include "globalVar.h"

typedef unsigned short int USHORT;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef unsigned long long ULONGLONG;
typedef unsigned char BYTE;

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
	bool operator>(const version &b){ return major == b.major ? (minor == b.minor ? (revision == b.revision ? false : revision > b.revision) : minor > b.minor) : major > b.major; };
	bool operator>=(const version &b){ return major == b.major ? (minor == b.minor ? (revision == b.revision ? true : revision > b.revision) : minor > b.minor) : major > b.major; };
	bool operator<(const version &b){ return major == b.major ? (minor == b.minor ? (revision == b.revision ? false : revision < b.revision) : minor < b.minor) : major < b.major; };
	bool operator<=(const version &b){ return major == b.major ? (minor == b.minor ? (revision == b.revision ? true : revision < b.revision) : minor < b.minor) : major < b.major; };
	UINT major;
	USHORT minor, revision;
};

typedef std::list<std::string> depListTp;

class package
{
public:
	package(std::string _source, std::string &_name, version _ver, depListTp &_depList, depListTp &_confList, pakExtInfo _extInfo);
	std::string getName(){ return name; };
	version getVer(){ return ver; };
	errInfo inst(bool upgrade = false);
	errInfo upgrade();
	bool check();

	friend void writeSource();
	friend void printInfo(package *pkg);
private:
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
