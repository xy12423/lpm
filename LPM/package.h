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
	enum verCon{ ALL, BIGGER, BIGEQU, LESS, LESEQU, EQU, NEQU, NONE };
	depInfo(){ con = ALL; };
	depInfo(const std::string &_name, version _ver, verCon _con){ name = _name; ver = _ver; con = _con; };
	depInfo(const std::string &_str);
	std::string name;
	version ver;
	verCon con;

	std::string conStr();
	std::string fullStr();
	bool check(version _ver);
	bool check();

	friend inline depInfo operator~(const depInfo &a);
};
typedef std::list<depInfo> depListTp;

class package
{
	struct instItem
	{
		enum opType{ INST, UPG };
		instItem(){ pak = NULL; }
		instItem(package *_pak, opType _oper){ pak = _pak; oper = _oper; };
		package* pak;
		opType oper;
	};
	typedef std::list<instItem> pakIListTp;
public:
	package(std::string _source, std::string &_name, version _ver, depListTp &_depList, depListTp &_confList, pakExtInfo _extInfo);

	const std::string &getName(){ return name; };
	const pakExtInfo &getExtInfo(){ return extInfo; };
	version getVer(){ return ver; };

	//Check requirements then install and run scripts
	errInfo instFull();
	//Try to upgrade the package
	errInfo upgrade(bool checked = false);

	bool needUpgrade();
	bool check();

	friend void writeSource();
	friend void printInfo(package *pkg);
private:
	//Install package without checking requirements
	errInfo inst();
	//Check if requirements are OK
	void checkDep(pakIListTp &instList, depListTp &extraDep);
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

package* find_package(const std::string &name);
package* find_package(const std::string &name, depInfo con);
package* find_package(const std::string &name, std::unordered_multimap<int, depInfo> &con);
bool is_installed(const std::string &name);
version cur_version(const std::string &name);
errInfo install(const std::string &name);
errInfo uninstall(const std::string &name, bool upgrade = false, bool force = false);
errInfo backup(const std::string &name, bool force = false);
errInfo recover_from_backup(const std::string &name);

#endif
