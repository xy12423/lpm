#include "stdafx.h"
#include "package.h"
#include "source.h"
#include "unzip.h"
#include "download.h"

namespace fs = boost::filesystem;

void install_copy(const fs::path &tmpPath, fs::path relaPath, std::ofstream &logOut)
{
	fs::path nativePath = dataPath / DIRNAME_NATIVE;
	for (fs::directory_iterator p(tmpPath), pEnd; p != pEnd; p++)
	{
		fs::path sourcePath = p->path(), newRelaPath = relaPath / sourcePath.filename(), targetPath = localPath / newRelaPath;
		if (fs::is_regular_file(sourcePath))
		{
			if (exists(targetPath))
			{
				if (!exists(nativePath / relaPath))
					fs::create_directories(nativePath / relaPath);
				if (exists(nativePath / newRelaPath))
					throw(msgData[MSGE_OVERWRITE]);
				fs::copy_file(targetPath, nativePath / newRelaPath);
				fs::remove(targetPath);
			}
			logOut << targetPath.string() << std::endl;
			fs::copy_file(sourcePath, targetPath);
		}
		else
		{
			if (!exists(targetPath))
			{
				fs::create_directory(targetPath);
				install_copy(sourcePath, newRelaPath, logOut);
				logOut << targetPath.string() << std::endl;
			}
			else
				install_copy(sourcePath, newRelaPath, logOut);
		}
	}
}

version::version(const std::string &str)
{
	major = minor = revision = 0;
	std::string::const_iterator p = str.cbegin(), pEnd = str.cend();
	for (; p != pEnd; p++)
	{
		if (*p == '.')
		{
			p++;
			break;
		}
		if (!isdigit(*p))
		{
			major = minor = revision = 0;
			return;
		}
		major = major * 10 + static_cast<UINT>((*p) - '0');
	}
	for (; p != pEnd; p++)
	{
		if (*p == '.')
		{
			p++;
			break;
		}
		if (!isdigit(*p))
		{
			major = minor = revision = 0;
			return;
		}
		minor = minor * 10 + static_cast<UINT>((*p) - '0');
	}
	for (; p != pEnd; p++)
	{
		if (!isdigit(*p))
		{
			major = minor = revision = 0;
			return;
		}
		revision = revision * 10 + static_cast<UINT>((*p) - '0');
	}
}

depInfo::depInfo(const std::string &str)
{
	std::string::const_reverse_iterator itr = str.crbegin(), itrEnd = str.crend(), itrP;
	for (; itr != itrEnd && *itr != ':'; itr++);
	if (itr == itrEnd)
	{
		name = str;
		con = NOCON;
		return;
	}
	itrP = itr;
	itrP++;
	while (itrP != itrEnd)
	{
		name = *itrP + name;
		itrP++;
	}
	itrEnd = str.crbegin();
	itr--;
	switch (*itr)
	{
		case '>':
			itr--;
			if (*itr == '=')
				con = BIGEQU;
			else
			{
				itr++;
				con = BIGGER;
			}
			break;
		case '<':
			itr--;
			if (*itr == '=')
				con = LESEQU;
			else
			{
				itr++;
				con = LESS;
			}
			break;
		case '=':
			con = EQU;
			break;
		case '!':
			con = NEQU;
			break;
		default:
			con = EQU;
			itr++;
	}
	std::string verStr;
	do
	{
		itr--;
		verStr.push_back(*itr);
	}
	while (itr != itrEnd);
	ver = version(verStr);
}

std::string depInfo::fullStr()
{
	static const std::string conStrT[] = {
		"",
		">",
		">=",
		"<",
		"<=",
		"=",
		"!"
	};
	return name + (con == NOCON ? "" : ':' + conStrT[con] + ver.toStr());
}

bool depInfo::check(version _ver)
{
	if (con == NOCON)
		return true;
	switch (con)
	{
		case BIGGER:
			return _ver > ver;
		case BIGEQU:
			return _ver >= ver;
		case LESS:
			return _ver < ver;
		case LESEQU:
			return _ver <= ver;
		case EQU:
			return _ver == ver;
		case NEQU:
			return _ver != ver;
		default:
			return false;
	}
}

bool depInfo::check()
{
	if (!is_installed(name))
		return false;
	return check(cur_version(name));
}

inline depInfo operator~(const depInfo &a)
{
	depInfo b = a;
	switch (a.con)
	{
		case depInfo::BIGGER:
			b.con = depInfo::LESEQU;
			break;
		case depInfo::BIGEQU:
			b.con = depInfo::LESS;
			break;
		case depInfo::LESS:
			b.con = depInfo::BIGEQU;
			break;
		case depInfo::LESEQU:
			b.con = depInfo::BIGGER;
			break;
		case depInfo::EQU:
			b.con = depInfo::NEQU;
			break;
		case depInfo::NEQU:
			b.con = depInfo::EQU;
			break;
	}
	return b;
}

package::package(std::string _source, std::string &_name, version _ver, depListTp &_depList, depListTp &_confList, pakExtInfo _extInfo)
{
	source = _source;
	name = _name;
	ver = _ver;
	depList = _depList;
	confList = _confList;
	extInfo = _extInfo;
}

errInfo package::inst()
{
	dataBuf buf;
	errInfo err = download(source + "/" + name + ".lpm", &buf);
	if (err.err)
		return err;

	fs::path tmpPath = dataPath / DIRNAME_TEMP / name, pakPath = dataPath / name;
	bool flag1 = false, flag2 = false, flag3 = false;
	try
	{
		fs::create_directory(tmpPath);
		flag1 = true;
		fs::create_directory(pakPath);
		flag2 = true;
		infoStream << msgData[MSGI_DECOMPRESSING] << std::endl;
		errInfo err = unzip(buf.begin(), buf.end(), tmpPath);
		if (err.err)
			return err;
		infoStream << msgData[MSGI_DECOMPRESSED] << std::endl;

		if (fs::exists(tmpPath / DIRNAME_INFO))
		{
			for (fs::directory_iterator p(tmpPath / DIRNAME_INFO), pEnd; p != pEnd; p++)
			{
				if (fs::is_regular_file(p->path()))
					fs::copy_file(p->path(), pakPath / p->path().filename());
				else
					throw(msgData[MSGE_ILLEGAL_PAK]);
			}
			fs::remove_all(tmpPath / DIRNAME_INFO);
		}

		std::ofstream infOut((pakPath / FILENAME_INST).string());
		infoStream << msgData[MSGI_COPYING] << std::endl;
		install_copy(tmpPath, fs::path(), infOut);
		infoStream << msgData[MSGI_COPIED] << std::endl;
		infOut.close();
		flag3 = true;

		fs::remove_all(tmpPath);

		infOut.open((pakPath / FILENAME_INFO).string());
		infOut << extInfo.fname << std::endl;
		infOut << extInfo.info << std::endl;
		infOut << extInfo.author << std::endl;
		infOut << ver.major << '.' << ver.minor << '.' << ver.revision << std::endl;
		std::for_each(depList.begin(), depList.end(), [this, &infOut](depInfo dpInf){
			infOut << dpInf.fullStr() << ';';
		});
		infOut << std::endl;
		std::for_each(confList.begin(), confList.end(), [this, &infOut](depInfo dpInf){
			infOut << dpInf.fullStr() << ';';
		});
		infOut << std::endl;
		infOut.close();

		depListTp::const_iterator pDep = depList.begin(), pDepEnd = depList.end();
		std::ofstream depOut;
		for (; pDep != pDepEnd; pDep++)
		{
			depOut.open((dataPath / pDep->name / FILENAME_BEDEP).string(), std::ios::out | std::ios::app);
			depOut << name << std::endl;
			depOut.close();
		}
		pDep = depList.begin();
		depOut.open((pakPath / FILENAME_DEP).string());
		for (; pDep != pDepEnd; pDep++)
			depOut << pDep->name << std::endl;
		depOut.close();
		fs::path backupPath = dataPath / DIRNAME_UPGRADE / (name + ".inf");
		if (exists(backupPath))
		{
			fs::copy_file(backupPath, pakPath / FILENAME_BEDEP);
			fs::remove(backupPath);
		}
	}
	catch (fs::filesystem_error err)
	{
		if (flag3)
		{
			std::ifstream infIn((pakPath / FILENAME_INST).string());
			std::string tmpPath;
			while (!infIn.eof())
			{
				std::getline(infIn, tmpPath);
				if (!tmpPath.empty())
					fs::remove(tmpPath);
			}
		}
		if (flag2)
			fs::remove_all(pakPath);
		if (flag1)
			fs::remove_all(tmpPath);
		return errInfo(msgData[MSGE_FS] + err.what());
	}
	catch (std::string err)
	{
		if (flag3)
		{
			std::ifstream infIn((pakPath / FILENAME_INST).string());
			std::string tmpPath;
			while (!infIn.eof())
			{
				std::getline(infIn, tmpPath);
				if (!tmpPath.empty())
					fs::remove(tmpPath);
			}
		}
		if (flag2)
			fs::remove_all(pakPath);
		if (flag1)
			fs::remove_all(tmpPath);
		return errInfo(err);
	}
	catch (std::exception ex)
	{
		if (flag3)
		{
			std::ifstream infIn((pakPath / FILENAME_INST).string());
			std::string tmpPath;
			while (!infIn.eof())
			{
				std::getline(infIn, tmpPath);
				if (!tmpPath.empty())
					fs::remove(tmpPath);
			}
		}
		if (flag2)
			fs::remove_all(pakPath);
		if (flag1)
			fs::remove_all(tmpPath);
		return errInfo(ex.what());
	}
	catch (...)
	{
		throw;
	}
	infoStream << msgData[MSGI_PAK_INSTALLED] << std::endl;
	return errInfo();
}

int package::instScript(bool upgrade)
{
	fs::path pakPath = dataPath / name, scriptPath = pakPath / SCRIPT_INST, currentPath = fs::current_path();
	if (exists(scriptPath))
	{
		infoStream << msgData[MSGI_RUNS_INST] << std::endl;
		scriptPath = fs::system_complete(scriptPath);
		fs::current_path(localPath);
		int ret = system(("\"" + scriptPath.string() + "\"").c_str());
		fs::current_path(currentPath);
		if (ret != EXIT_SUCCESS)
			return ret;
		infoStream << msgData[MSGI_DONE] << std::endl;
	}
	if (!upgrade)
	{
		scriptPath = pakPath / SCRIPT_INIT;
		if (exists(scriptPath))
		{
			infoStream << msgData[MSGI_RUNS_INIT] << std::endl;
			scriptPath = fs::system_complete(scriptPath);
			fs::current_path(localPath);
			int ret = system(scriptPath.string().c_str());
			fs::current_path(currentPath);
			if (ret != EXIT_SUCCESS)
				return ret;
			infoStream << msgData[MSGI_DONE] << std::endl;
		}
	}
	return 0;
}

typedef std::unordered_map<std::string, version> depMapTp;
struct instItem
{
	enum opType{ INST, UPG };
	instItem(){ pak = NULL; }
	instItem(package *_pak, opType _oper){ pak = _pak; oper = _oper; };
	package* pak;
	opType oper;
};
typedef std::list<instItem> pakIListTp;
typedef std::list<int> pakQListTp;
struct depNode
{
	depNode(){ pak = NULL; }
	depNode(package *_pak){ pak = _pak; }
	package *pak;
	std::unordered_multimap<int, depInfo> con;
	std::unordered_map<int, int> ancestor;
	std::unordered_set<int> dep;
};
typedef std::unordered_map<int, depNode> depMap;
typedef std::unordered_map<std::string, int> depHash;

void clean_dep(depMap &pakMap, depHash &pakHash, int nodeID)
{
	pakQListTp que;
	depNode &node = pakMap.at(nodeID);
	std::for_each(node.dep.begin(), node.dep.end(), [&](int id){
		pakMap.at(id).con.erase(nodeID);
		que.push_back(id);
	});
	node.dep.clear();
	while (!que.empty())
	{
		int id = que.front();
		depNode depN = pakMap.at(id);
		que.pop_front();
		std::for_each(node.ancestor.cbegin(), node.ancestor.cend(), [&depN](const std::pair<int, int>& p){
			depN.ancestor.at(p.first) -= p.second;
			if (depN.ancestor.at(p.first) <= 0)
				depN.ancestor.erase(p.first);
		});
		depN.ancestor.at(nodeID)--;
		if (depN.ancestor.at(nodeID) == 0)
			depN.ancestor.erase(nodeID);
		if (depN.ancestor.empty() && id != 0)
		{
			clean_dep(pakMap, pakHash, id);
			pakHash.erase(depN.pak->getName());
			pakMap.erase(id);
		}
		else
		{
			std::for_each(depN.dep.begin(), depN.dep.end(), [&](int id){
				que.push_back(id);
			});
		}
	}
}

errInfo package::instFull()
{
	depListTp::iterator pDep, pDepEnd;
	pDep = confList.begin();
	pDepEnd = confList.end();
	depListTp pakList;
	infoStream << msgData[MSGI_CHECK_REQUIREMENT] << std::endl;

	//Check requirement:init
	pakIListTp instList;
	depMap pakMap;
	depHash pakHash;
	depHash::iterator itrHash, itrHashEnd = pakHash.end();
	int nextID = 1;
	pakMap.emplace(0, this);
	pakHash.emplace(name, 0);
	pakQListTp pakQue;
	pakQue.push_back(0);

	//Check requirement:BFS
	try
	{
		while (!pakQue.empty())
		{
			int id = pakQue.front();
			depNode &node = pakMap.at(id);
			pakQue.pop_front();
			pDep = node.pak->confList.begin();
			pDepEnd = node.pak->confList.end();
			for (; pDep != pDepEnd; pDep++)
			{
				int confID;
				itrHash = pakHash.find(pDep->name);
				if (itrHash != itrHashEnd)
				{
					confID = itrHash->second;
					depNode &confN = pakMap.at(confID);
					confN.con.emplace(id, ~(*pDep));
					std::for_each(node.ancestor.cbegin(), node.ancestor.cend(), [&confN](const std::pair<int, int>& p){
						confN.ancestor[p.first] += p.second;
					});
					confN.ancestor[id]++;
					node.dep.emplace(confID);
				}
				else
				{
					int newID = nextID;
					nextID++;
					confID = newID;
					node.dep.emplace(newID);
					pakMap.emplace(newID, depNode());
					pakHash.emplace(pDep->name, newID);
					itrHash = pakHash.find(pDep->name);
					depNode &confN = pakMap.at(newID);
					confN.con.emplace(id, *pDep);
					std::for_each(node.ancestor.cbegin(), node.ancestor.cend(), [&confN](const std::pair<int, int>& p){
						confN.ancestor[p.first] += p.second;
					});
					confN.ancestor[id]++;
					pakQue.push_back(newID);
				}
				if (pDep->check())
				{
					depNode &confN = pakMap.at(confID);
					clean_dep(pakMap, pakHash, confID);
					package *pak = find_package(pDep->name, confN.con);
					if (pak == NULL)
						throw(msgData[MSGE_CONF] + ':' + node.pak->name + ":" + pDep->fullStr());
					confN.pak = pak;
					pakQue.push_back(confID);
				}
				else
				{
					depNode &confN = pakMap.at(confID);
					if (confN.pak != NULL && pDep->check(confN.pak->ver))
					{
						if (node.ancestor.find(confID) != node.ancestor.end())
							throw(msgData[MSGE_DEP] + ':' + node.pak->name + ":" + pDep->fullStr());
						clean_dep(pakMap, pakHash, confID);
						package *pak = find_package(pDep->name, confN.con);
						if (pak == NULL)
							throw(msgData[MSGE_PAK_NOT_FOUND] + ':' + pDep->name);
						confN.pak = pak;
						pakQue.push_back(confID);
					}
				}
			}
			pDep = node.pak->depList.begin();
			pDepEnd = node.pak->depList.end();
			for (; pDep != pDepEnd; pDep++)
			{
				int depID;
				itrHash = pakHash.find(pDep->name);
				if (itrHash != itrHashEnd)
				{
					depID = itrHash->second;
					depNode &depN = pakMap.at(depID);
					depN.con.emplace(id, *pDep);
					std::for_each(node.ancestor.cbegin(), node.ancestor.cend(), [&depN](const std::pair<int, int>& p){
						depN.ancestor[p.first] += p.second;
					});
					depN.ancestor[id]++;
					node.dep.emplace(depID);
					if (!pDep->check() && (depN.pak == NULL || !pDep->check(depN.pak->ver)))
					{
						if (node.ancestor.find(itrHash->second) != node.ancestor.end())
							throw(msgData[MSGE_DEP] + ':' + node.pak->name + ":" + pDep->fullStr());
						clean_dep(pakMap, pakHash, depID);
						package *pak = find_package(pDep->name, depN.con);
						if (pak == NULL)
							throw(msgData[MSGE_PAK_NOT_FOUND] + ':' + pDep->name);
						depN.pak = pak;
						pakQue.push_back(itrHash->second);
					}
				}
				else
				{
					int newID = nextID;
					nextID++;
					depID = newID;
					package *pak;
					if (pDep->check())
						pak = NULL;
					else
					{
						pak = find_package(pDep->name, *pDep);
						if (pak == NULL)
							throw(msgData[MSGE_PAK_NOT_FOUND] + ':' + pDep->name);
					}
					node.dep.emplace(newID);
					pakMap.emplace(newID, depNode(pak));
					pakHash.emplace(pDep->name, newID);
					depNode &depN = pakMap.at(newID);
					depN.con.emplace(id, *pDep);
					std::for_each(node.ancestor.cbegin(), node.ancestor.cend(), [&depN](const std::pair<int, int>& p){
						depN.ancestor[p.first] += p.second;
					});
					depN.ancestor[id]++;
					pakQue.push_back(newID);
				}
			}
		}

		std::unordered_set<int> pakDiff;
		std::unordered_set<int>::iterator itrDiffEnd = pakDiff.end();
		pakQue.push_back(0);
		pakDiff.emplace(0);
		while (!pakQue.empty())
		{
			int id = pakQue.front();
			depNode &node = pakMap.at(id);
			pakQue.pop_front();
			package *pak = node.pak;
			if (is_installed(pak->name))
				instList.push_front(instItem(pak, instItem::UPG));
			else
				instList.push_front(instItem(pak, instItem::INST));
			pDep = node.pak->depList.begin();
			pDepEnd = node.pak->depList.end();
			for (; pDep != pDepEnd; pDep++)
			{
				int depID = pakHash.at(pDep->name);
				if (pakDiff.find(depID) == itrDiffEnd)
				{
					pakQue.push_back(depID);
					pakDiff.emplace(depID);
				}
			}
		}
	}
	catch (std::exception ex)
	{
		return errInfo(msgData[MSGE_STD] + ex.what());
	}
	catch (std::string err)
	{
		return errInfo(err);
	}
	catch (...)
	{
		throw;
	}

	//Install/Upgrade Packages
	pakIListTp::iterator instItr, instEnd;
	infoStream << msgData[MSGI_WILL_INST_LIST] << std::endl;
	instItr = instList.begin();
	instEnd = instList.end();
	for (; instItr != instEnd; instItr++)
		infoStream << "\t" << instItr->pak->name << std::endl;
	instItr = instList.begin();
	for (; instItr != instEnd; instItr++)
	{
		switch (instItr->oper)
		{
			case instItem::INST:
			{
				infoStream << msgData[MSGI_PAK_INSTALLING] << ':' << instItr->pak->name << std::endl;
				errInfo err = instItr->pak->inst();
				if (err.err)
					return err;
				break;
			}
			case instItem::UPG:
			{
				errInfo err = instItr->pak->upgrade();
				if (err.err)
					return err;
				break;
			}
		}
	}

	//Run scripts
	instItr = instList.begin();
	for (; instItr != instEnd; instItr++)
	{
		if (instItr->oper == instItem::INST)
		{
			int ret = instItr->pak->instScript();
			if (ret != EXIT_SUCCESS)
			{
				infoStream << msgData[MSGW_RUNS_ROLL_BACK_1] << ret << msgData[MSGW_RUNS_ROLL_BACK_2] << std::endl;
				pakIListTp::reverse_iterator rbItr, rbEnd = instList.rend();
				rbItr = instList.rbegin();
				for (; rbItr != rbEnd; rbItr++)
				{
					if (rbItr->oper == instItem::INST)
					{
						infoStream << msgData[MSGI_PAK_REMOVING] << ':' << rbItr->pak->name << std::endl;
						errInfo err = uninstall(rbItr->pak->name);
						if (err.err)
							return err;
					}
				}
				return errInfo(msgData[MSGE_RUNS] + num2str(ret));
			}
		}
	}

	infoStream << msgData[MSGI_PAKS_INSTALLED] << std::endl;
	return errInfo();
}

bool package::needUpgrade()
{
	if (!is_installed(name))
		return false;
	return cur_version(name) < ver;
}

errInfo package::upgrade(bool checked)
{
	if (!is_installed(name))
		return errInfo(msgData[MSGE_PAK_NOT_INSTALLED]);
	if (!(checked || needUpgrade()))
		return errInfo(msgData[MSGE_PAK_LATEST]);

	infoStream << msgData[MSGI_PAK_UPGRADING] << ':' << name << std::endl;
	infoStream << msgData[MSGI_PAK_REMOVING] << std::endl;
	errInfo err = uninstall(name, true);
	if (err.err)
		return err;
	infoStream << msgData[MSGI_PAK_REINSTALLING] << std::endl;
	err = instFull();
	if (err.err)
		return err;
	int ret = instScript(true);
	if (ret != EXIT_SUCCESS)
		return errInfo(msgData[MSGE_RUNS] + num2str(ret));
	return errInfo();
}

bool package::check()
{
	depListTp::const_iterator pDep, pDepEnd;
	pDep = confList.begin();
	pDepEnd = confList.end();
	for (; pDep != pDepEnd; pDep++)
		if (is_installed(pDep->name))
			return false;
	pDep = depList.begin();
	pDepEnd = depList.end();
	for (; pDep != pDepEnd; pDep++)
		if (!is_installed(pDep->name))
			return false;
	return true;
}

bool is_installed(const std::string &name)
{
	return fs::exists(dataPath / name) && fs::is_directory(dataPath / name);
}

version cur_version(const std::string &name)
{
	if (!is_installed(name))
		return version(0, 0, 0);

	std::string line;
	std::ifstream infoIn((dataPath / name / FILENAME_INFO).string());
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	infoIn.close();

	return version(line);
}

package* find_package(const std::string &name)
{
	srcListTp::const_iterator p, pEnd = sourceList.cend();
	package *pak, *ret = NULL;
	version ver(0, 0, 0);
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		pak = (*p)->find_package(name);
		if (pak != NULL && pak->getVer() > ver)
		{
			ret = pak;
			ver = pak->getVer();
		}
	}
	return ret;
}

package* find_package(const std::string &name, depInfo con)
{
	srcListTp::const_iterator p, pEnd = sourceList.cend();
	package *pak, *ret = NULL;
	version ver, newVer;
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		pak = (*p)->find_package(name);
		newVer = pak->getVer();
		if (pak != NULL && con.check(newVer) && newVer > ver)
		{
			ret = pak;
			ver = newVer;
		}
	}
	return ret;
}

package* find_package(const std::string &name, std::unordered_multimap<int, depInfo> &con)
{
	srcListTp::const_iterator p, pEnd = sourceList.cend();
	package *pak, *ret = NULL;
	version ver, newVer;
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		pak = (*p)->find_package(name);
		newVer = pak->getVer();
		if (pak != NULL && newVer > ver)
		{
			bool flag = true;
			for (std::unordered_multimap<int, depInfo>::iterator itr = con.begin(), itrEnd = con.end(); itr != itrEnd; itr++)
			{
				if (!itr->second.check(newVer))
				{
					flag = false;
					break;
				}
			}
			if (flag)
			{
				ret = pak;
				ver = newVer;
			}
		}
	}
	return ret;
}

errInfo install(std::string name)
{
	if (is_installed(name))
		return errInfo(msgData[MSGE_PAK_INSTALLED]);

	infoStream << msgData[MSGI_PAK_SEARCHING] << std::endl;
	package *pak = find_package(name);
	if (pak == NULL)
		return errInfo(msgData[MSGE_PAK_NOT_FOUND]);
	infoStream << msgData[MSGI_PAK_FOUND] << ':' << name << std::endl;
	
	return pak->instFull();
}

errInfo uninstall(std::string name, bool upgrade)
{
	if (!is_installed(name))
		return errInfo(msgData[MSGE_PAK_NOT_INSTALLED]);
	fs::path pakPath = dataPath / name;

	std::ifstream depIn;
	std::string line;
	if (fs::exists(pakPath / FILENAME_BEDEP))
	{
		if (upgrade)
		{
			fs::copy_file(pakPath / FILENAME_BEDEP, dataPath / DIRNAME_UPGRADE / (name + ".inf"));
		}
		else
		{
			infoStream << msgData[MSGW_PAK_BE_DEP] << std::endl;
			depIn.open((pakPath / FILENAME_BEDEP).string());
			while (!depIn.eof())
			{
				std::getline(depIn, line);
				infoStream << "\t" << line << std::endl;
			}
			depIn.close();
			return errInfo(msgData[MSGE_PAK_BE_DEP]);
		}
	}

	{
		if (!fs::exists(pakPath / FILENAME_DEP))
			return errInfo(msgData[MSGE_PAK_DEPINFO_NOT_FOUND]);
		depIn.open((pakPath / FILENAME_DEP).string());
		std::list<std::string> bedepTmp;
		std::string bedepFile;
		std::fstream bedepFS;
		while (!depIn.eof())
		{
			std::getline(depIn, bedepFile);
			if (!bedepFile.empty())
			{
				bedepFile = (dataPath / bedepFile / FILENAME_BEDEP).string();
				bedepFS.open(bedepFile, std::ios::in);
				while (!bedepFS.eof())
				{
					std::getline(bedepFS, line);
					if (line == name)
						break;
					bedepTmp.push_back(line);
				}
				while (!bedepFS.eof())
				{
					std::getline(bedepFS, line);
					if (!line.empty())
						bedepTmp.push_back(line);
				}
				bedepFS.close();
				if (bedepTmp.empty())
					fs::remove(bedepFile);
				else
				{
					bedepFS.open(bedepFile, std::ios::out);
					while (!bedepTmp.empty())
					{
						bedepFS << bedepTmp.front() << std::endl;
						bedepTmp.pop_front();
					}
					bedepFS.close();
				}
			}
		}
		depIn.close();
	}

	fs::path scriptPath, currentPath = fs::current_path();
	if (!upgrade)
	{
		scriptPath = pakPath / SCRIPT_PURGE;
		if (exists(scriptPath))
		{
			infoStream << msgData[MSGI_RUNS_PURGE] << std::endl;
			fs::current_path(localPath);
			int ret = system(scriptPath.string().c_str());
			if (ret != 0)
				return errInfo(msgData[MSGE_RUNS] + num2str(ret));
			infoStream << msgData[MSGI_DONE] << std::endl;
		}
	}
	scriptPath = pakPath / SCRIPT_REMOVE;
	if (exists(scriptPath))
	{
		infoStream << msgData[MSGI_RUNS_REMOVE] << std::endl;
		fs::current_path(localPath);
		int ret = system(scriptPath.string().c_str());
		if (ret != 0)
			return errInfo(msgData[MSGE_RUNS] + num2str(ret));
		infoStream << msgData[MSGI_DONE] << std::endl;
	}
	fs::current_path(currentPath);

	fs::path logPath = pakPath / FILENAME_INST;
	std::ifstream logIn(logPath.string());
	std::string tmpPath;

	try
	{
		infoStream << msgData[MSGI_DELETING] << std::endl;
		while (!logIn.eof())
		{
			std::getline(logIn, tmpPath);
			if (!tmpPath.empty())
			{
				if (!fs::is_directory(tmpPath) || fs::is_empty(tmpPath))
					fs::remove(tmpPath);
			}
		}
		logIn.close();
		fs::remove_all(pakPath);
	}
	catch (fs::filesystem_error err)
	{
		return errInfo(msgData[MSGE_FS] + err.what());
	}
	catch (const char* err)
	{
		return errInfo(std::string(err));
	}
	catch (...)
	{
		throw;
	}
	infoStream << msgData[MSGI_PAK_REMOVED] << std::endl;
	return errInfo();
}
