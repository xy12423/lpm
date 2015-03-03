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
	pakExtInfo(std::string &_author, std::string &_info){ author = _author; info = _info; };
	std::string author, info;
};

struct version
{
	version(){ major = 0; minor = 0; revision = 0; };
	version(UINT _major, USHORT _minor, USHORT _revision){ major = _major; minor = _minor; revision = _revision; };
	UINT major;
	USHORT minor, revision;
};

typedef std::list<std::string> depListTp;

const int PKG_INSTALL_FAILED = -1;

const int PKG_UNINSTALL_FAILED = -1;

class package
{
public:
	package(std::string _source, std::string &_name, version _ver, depListTp &_depList, depListTp &_confList, pakExtInfo _extInfo);
	std::string getName(){ return name; };
	void rename(std::string &newName){ name = newName; };
	errInfo inst();

	friend void writeSource();
private:
	std::string source;
	std::string name;
	version ver;
	pakExtInfo extInfo;
	depListTp depList;
	depListTp confList;
};

errInfo install(std::string name);
errInfo uninstall(std::string name);
bool is_installed(std::string name);

#endif
