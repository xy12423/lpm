#pragma once

#ifndef _H_PKG
#define _H_PKG

#include "errInfo.h"
#include "global.h"

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
	std::string toStr(){ return num2str(major) + '.' + num2str(minor) + '.' + num2str(revision); };
	friend inline bool operator>(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? false : a.revision > b.revision) : a.minor > b.minor) : a.major > b.major; };
	friend inline bool operator>=(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? true : a.revision > b.revision) : a.minor > b.minor) : a.major > b.major; };
	friend inline bool operator<(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? false : a.revision < b.revision) : a.minor < b.minor) : a.major < b.major; };
	friend inline bool operator<=(const version &a, const version &b){ return a.major == b.major ? (a.minor == b.minor ? (a.revision == b.revision ? true : a.revision < b.revision) : a.minor < b.minor) : a.major < b.major; };
	friend inline bool operator==(const version &a, const version &b){ return a.major == b.major && a.minor == b.minor && a.revision == b.revision; };
	friend inline bool operator!=(const version &a, const version &b){ return a.major != b.major || a.minor != b.minor || a.revision != b.revision; };
};

struct depInfo
{
	enum verCon{ NOCON, BIGGER, BIGEQU, LESS, LESEQU, EQU, NEQU };
	depInfo(){ con = NOCON; };
	depInfo(const std::string &_name, version _ver, verCon _con){ name = _name; ver = _ver; con = _con; };
	depInfo(const std::string &_str);
	std::string name;
	version ver;
	verCon con;
	std::string fullStr();
	bool check(version _ver);
	bool check();

	friend inline depInfo operator~(const depInfo &a);
};
typedef std::list<depInfo> depListTp;

class package
{
public:
	package(std::string _source, std::string &_name, version _ver, depListTp &_depList, depListTp &_confList, pakExtInfo _extInfo);
	const std::string &getName(){ return name; };
	const pakExtInfo &getExtInfo(){ return extInfo; };
	version getVer(){ return ver; };
	errInfo instFull();
	bool needUpgrade();
	errInfo upgrade(bool checked = false);
	bool check();

	friend void writeSource();
	friend errInfo install(std::string name);
	friend void printInfo(package *pkg);
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
package* find_package(const std::string &name, depInfo con);
package* find_package(const std::string &name, std::unordered_multimap<int, depInfo> &con);
bool is_installed(const std::string &name);
version cur_version(const std::string &name);
errInfo install(std::string name);
errInfo uninstall(std::string name, bool upgrade = false);

#endif
