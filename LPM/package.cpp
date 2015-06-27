#include "stdafx.h"
#include "package.h"
#include "source.h"
#include "unzip.h"
#include "download.h"

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
		con = ALL;
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
	if (itr == itrEnd)
		con = ALL;
	else
	{
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
				itr--;
				if (*itr == '!')
					con = NONE;
				else
				{
					itr++;
					con = NEQU;
				}
				break;
			default:
				con = EQU;
				itr++;
		}
		if (con != NONE)
		{
			std::string verStr;
			while (itr != itrEnd)
			{
				itr--;
				verStr.push_back(*itr);
			}
			if (!verStr.empty())
				ver = version(verStr);
		}
	}
}

std::string depInfo::conStr()
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
	return con == ALL ? "" : (con == NONE ? ":!!" : ':' + conStrT[con] + ver.toStr());
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
	return name + conStr();
}

bool depInfo::check(version _ver)
{
	switch (con)
	{
		case ALL:
			return true;
		case NONE:
			return false;
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
	if ((!is_installed(name)) ^ (con == NONE))
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
		case depInfo::ALL:
			b.con = depInfo::NONE;
			break;
		case depInfo::NONE:
			b.con = depInfo::ALL;
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

void install_copy(const fs::path &tmpPath, fs::path relaPath, std::ofstream &logOut, bool nbackup)
{
	fs::path nativePath = dataPath / DIRNAME_NATIVE;
	for (fs::directory_iterator p(tmpPath), pEnd; p != pEnd; p++)
	{
		fs::path sourcePath = p->path(), newRelaPath = relaPath / sourcePath.filename(), targetPath = localPath / newRelaPath;
		if (fs::is_regular_file(sourcePath))
		{
			if (exists(targetPath))
			{
				if (!nbackup)
				{
					if (!exists(nativePath / relaPath))
						fs::create_directories(nativePath / relaPath);
					if (exists(nativePath / newRelaPath))
						throw(msgData[MSGE_OVERWRITE]);
					fs::copy_file(targetPath, nativePath / newRelaPath);
				}
				fs::remove(targetPath);
			}
			logOut << newRelaPath.string() << std::endl;
			fs::copy_file(sourcePath, targetPath);
			fs::remove(sourcePath);
		}
		else
		{
			if (!exists(targetPath))
			{
				fs::create_directory(targetPath);
				install_copy(sourcePath, newRelaPath, logOut, nbackup);
				logOut << newRelaPath.string() << std::endl;
			}
			else
				install_copy(sourcePath, newRelaPath, logOut, nbackup);
		}
	}
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

		if (fs::exists(tmpPath / DIRNAME_PATH))
		{
			fs::path pathPath = dataPath / DIRNAME_PATH;
			for (fs::directory_iterator p(tmpPath / DIRNAME_PATH), pEnd; p != pEnd; p++)
			{
				if (!fs::exists(pathPath / p->path().filename()))
					fs::copy(p->path(), pathPath / p->path().filename());
				else
					throw(msgData[MSGE_OVERWRITE]);
			}
			fs::remove_all(tmpPath / DIRNAME_PATH);
		}

		std::ofstream infOut((pakPath / FILENAME_INST).string());
		infOut << fs::system_complete(localPath).string() << std::endl;
		infoStream << msgData[MSGI_COPYING] << std::endl;
		install_copy(tmpPath, fs::path(), infOut, true);
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

		depListTp::iterator pDep = depList.begin(), pDepEnd = depList.end();
		std::ofstream depOut;
		for (; pDep != pDepEnd; pDep++)
		{
			if (!fs::exists(dataPath / pDep->name))
				fs::create_directory(dataPath / pDep->name);
			depOut.open((dataPath / pDep->name / FILENAME_BEDEP).string(), std::ios::out | std::ios::app);
			depOut << name << pDep->conStr() << std::endl;
			depOut.close();
		}
		pDep = depList.begin();
		depOut.open((pakPath / FILENAME_DEP).string());
		for (; pDep != pDepEnd; pDep++)
			depOut << pDep->name << std::endl;
		depOut.close();
		pDep = confList.begin();
		pDepEnd = confList.end();
		depOut.open((pakPath / FILENAME_CONF).string());
		for (; pDep != pDepEnd; pDep++)
			depOut << (~(*pDep)).fullStr() << std::endl;
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
			std::getline(infIn, tmpPath);
			while (!infIn.eof())
			{
				std::getline(infIn, tmpPath);
				if (!tmpPath.empty())
					fs::remove(localPath / tmpPath);
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
			std::getline(infIn, tmpPath);
			while (!infIn.eof())
			{
				std::getline(infIn, tmpPath);
				if (!tmpPath.empty())
					fs::remove(localPath / tmpPath);
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
			std::getline(infIn, tmpPath);
			while (!infIn.eof())
			{
				std::getline(infIn, tmpPath);
				if (!tmpPath.empty())
					fs::remove(localPath / tmpPath);
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
	fs::path pakPath = dataPath / name,
		scriptPath = pakPath / SCRIPT_INST,
		currentPath = fs::current_path(),
		pathPath = fs::system_complete(dataPath / DIRNAME_PATH);
	if (exists(scriptPath))
	{
		infoStream << msgData[MSGI_RUNS_INST] << std::endl;
		scriptPath = fs::system_complete(scriptPath);
		fs::current_path(localPath);
#ifdef WIN32
		int ret = system(("set \"PATH=" + pathPath.string() + ";%PATH%\" & \"" + scriptPath.string() + "\"").c_str());
#endif
#ifdef __linux__
		int ret = system(("PATH=\"" + pathPath.string() + ":$PATH\" ; bash \"" + scriptPath.string() + "\"").c_str());
#endif
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
#ifdef WIN32
			int ret = system(("set \"PATH=" + pathPath.string() + ";%PATH%\" & \"" + scriptPath.string() + "\"").c_str());
#endif
#ifdef __linux__
			int ret = system(("PATH=\"" + pathPath.string() + ":$PATH\" ; bash \"" + scriptPath.string() + "\"").c_str());
#endif
			fs::current_path(currentPath);
			if (ret != EXIT_SUCCESS)
				return ret;
			infoStream << msgData[MSGI_DONE] << std::endl;
		}
	}
	return 0;
}

errInfo package::instFull()
{
	infoStream << msgData[MSGI_CHECK_REQUIREMENT] << std::endl;

	pakIListTp instList;
	try
	{
		checkDep(instList, depListTp());
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
	try
	{
		for (; instItr != instEnd; instItr++)
		{
			switch (instItr->oper)
			{
				case instItem::INST:
				{
					infoStream << msgData[MSGI_PAK_INSTALLING] << ':' << instItr->pak->name << std::endl;
					errInfo err = instItr->pak->inst();
					if (err.err)
						throw(err);
					break;
				}
				case instItem::UPG:
				{
					errInfo err = instItr->pak->upgrade(true);
					if (err.err)
						throw(err);
					break;
				}
			}
		}
	}
	catch (std::exception ex)
	{
		instEnd = instList.begin();
		while (true)
		{
			if (instItr->oper == instItem::INST)
			{
				infoStream << msgData[MSGI_PAK_REMOVING] << ':' << instItr->pak->name << std::endl;
				errInfo err = uninstall(instItr->pak->name);
				if (err.err)
					return err;
			}
			if (instItr == instEnd)
				break;
			else
				instItr--;
		}
		return errInfo(msgData[MSGE_STD] + ex.what());
	}
	catch (errInfo err)
	{
		instEnd = instList.begin();
		while (true)
		{
			if (instItr->oper == instItem::INST)
			{
				infoStream << msgData[MSGI_PAK_REMOVING] << ':' << instItr->pak->name << std::endl;
				errInfo err = uninstall(instItr->pak->name);
				if (err.err)
					return err;
			}
			if (instItr == instEnd)
				break;
			else
				instItr--;
		}
		return err;
	}
	catch (...)
	{
		throw;
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
						errInfo err = uninstall(rbItr->pak->name, false, true);
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
	if (!checked)
	{
		if (!needUpgrade())
			return errInfo(msgData[MSGE_PAK_LATEST]);

		std::ifstream bedepIn((dataPath / name / FILENAME_BEDEP).string());
		std::string line;
		depListTp depList;
		while (!bedepIn.eof())
		{
			std::getline(bedepIn, line);
			if (!line.empty())
				depList.push_back(depInfo(line));
		}
		bedepIn.close();

		try
		{
			checkDep(pakIListTp(), depList);
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
	}

	infoStream << msgData[MSGI_PAK_UPGRADING] << ':' << name << std::endl;
	errInfo err = backup(name, true);
	if (err.err)
		return err;
	
	infoStream << msgData[MSGI_PAK_REMOVING] << std::endl;
	err = uninstall(name, true);
	if (err.err)
		return err;
	infoStream << msgData[MSGI_PAK_REINSTALLING] << std::endl;
	err = inst();
	if (err.err)
		return err;
	int ret = instScript(true);
	if (ret != EXIT_SUCCESS)
	{
		infoStream << msgData[MSGW_RUNS_ROLL_BACK_1] << ret << msgData[MSGW_RUNS_ROLL_BACK_2] << std::endl;
		uninstall(name, false, true);
		recover_from_backup(name);
		return errInfo(msgData[MSGE_RUNS] + num2str(ret));
	}
	return errInfo();
}

bool package::check()
{
	try
	{
		checkDep(pakIListTp(), depListTp());
	}
	catch (std::exception ex)
	{
		return false;
	}
	catch (std::string err)
	{
		return false;
	}
	catch (...)
	{
		throw;
	}
	return true;
}

typedef std::unordered_map<std::string, version> depMapTp;
typedef std::list<int> pakQListTp;
struct depNode
{
	depNode(){ pak = NULL; }
	depNode(package *_pak){ pak = _pak; }
	bool processed = true;
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
				if (node.ancestor.find(id) == node.ancestor.end())
					que.push_back(id);
			});
		}
	}
}

void add_ancestor(depMap &pakMap, depHash &pakHash, int dst, int src)
{
	depNode &srcN = pakMap.at(src), &dstN = pakMap.at(dst);
	std::for_each(srcN.ancestor.cbegin(), srcN.ancestor.cend(), [&dstN](const std::pair<int, int>& p){
		dstN.ancestor[p.first] += p.second;
	});
	dstN.ancestor[src]++;
	if (dstN.processed)
	{
		std::unordered_map<int, int>::iterator ancEnd = srcN.ancestor.end();
		std::for_each(srcN.dep.begin(), srcN.dep.end(), [&](int depID){
			if (srcN.ancestor.find(depID) == ancEnd)
				add_ancestor(pakMap, pakHash, depID, src);
		});
	}
}

void package::checkDep(pakIListTp &instList, depListTp &extraDep)
{
	//Check requirement:init
	depListTp::iterator pDep, pDepEnd;

	depMap pakMap;
	depHash pakHash;
	depHash::iterator itrHash, itrHashEnd = pakHash.end();
	int nextID = 1;
	pakMap.emplace(0, this);
	std::for_each(extraDep.begin(), extraDep.end(), [&](const depInfo& inf){
		pakMap[0].con.emplace(-1, inf);
	});
	pakHash.emplace(name, 0);
	pakQListTp pakQue;
	pakQue.push_back(0);

	//Check requirement:add local info
	fs::directory_iterator p(dataPath), pEnd;
	fs::path name;
	std::string buf;
	for (; p != pEnd; p++)
	{
		name = p->path().filename();
		if (name.string().front() != '$')
		{
			std::ifstream fin((dataPath / name / FILENAME_CONF).string());
			while (!fin.eof())
			{
				std::getline(fin, buf);
				if (!buf.empty())
				{
					depInfo conf(buf);
					itrHash = pakHash.find(conf.name);
					if (itrHash != itrHashEnd)
					{
						depNode &confN = pakMap.at(itrHash->second);
						confN.con.emplace(-1, conf);
					}
					else
					{
						int newID = nextID;
						nextID++;
						pakMap.emplace(newID, depNode());
						pakHash.emplace(conf.name, newID);
						itrHash = pakHash.find(conf.name);
						depNode &confN = pakMap.at(newID);
						confN.con.emplace(-1, conf);
					}
				}
			}
			fin.close();
		}
	}
	
	{
		depNode &instN = pakMap.at(0);
		if (!instN.con.empty())
		{
			std::for_each(instN.con.begin(), instN.con.end(), [this](std::pair<int, depInfo> p){
				if (!p.second.check(ver))
					throw(msgData[MSGE_CONF] + ':' + p.second.fullStr());
			});
		}
	}

	//Check requirement:BFS
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
			if (itrHash != itrHashEnd)	//The package is in the map
			{
				confID = itrHash->second;
				depNode &confN = pakMap.at(confID);
				confN.con.emplace(id, ~(*pDep));
				if (node.ancestor.find(confID) == node.ancestor.end())
					add_ancestor(pakMap, pakHash, confID, id);
				node.dep.emplace(confID);
			}
			else	//Add the package to the map
			{
				int newID = nextID;
				nextID++;
				confID = newID;
				node.dep.emplace(newID);
				pakMap.emplace(newID, depNode());
				pakHash.emplace(pDep->name, newID);
				itrHash = pakHash.find(pDep->name);
				depNode &confN = pakMap.at(newID);
				confN.con.emplace(id, ~(*pDep));
				std::for_each(node.ancestor.cbegin(), node.ancestor.cend(), [&confN](const std::pair<int, int>& p){
					confN.ancestor[p.first] += p.second;
				});
				confN.ancestor[id]++;
			}
			if (pDep->check())	//Conflict with installed package
			{
				depNode &confN = pakMap.at(confID);
				clean_dep(pakMap, pakHash, confID);
				package *pak = find_package(pDep->name, confN.con);	//Try to find a correct package
				if (pak == NULL)
					throw(msgData[MSGE_CONF] + ':' + node.pak->name + ":" + pDep->fullStr());
				confN.pak = pak;
				if (confN.processed)
				{
					confN.processed = false;
					pakQue.push_back(confID);
				}
			}
			else
			{
				depNode &confN = pakMap.at(confID);
				if (confN.pak != NULL && pDep->check(confN.pak->ver))	//Conflict with installing package
				{
					if (node.ancestor.find(confID) != node.ancestor.end())	//Loop in map
						throw(msgData[MSGE_DEP] + ':' + node.pak->name + ":" + pDep->fullStr());
					clean_dep(pakMap, pakHash, confID);
					package *pak = find_package(pDep->name, confN.con);
					if (pak == NULL)
						throw(msgData[MSGE_PAK_NOT_FOUND] + ':' + pDep->fullStr());
					confN.pak = pak;
					if (confN.processed)
					{
						confN.processed = false;
						pakQue.push_back(confID);
					}
				}
			}
		}
		pDep = node.pak->depList.begin();
		pDepEnd = node.pak->depList.end();
		for (; pDep != pDepEnd; pDep++)
		{
			int depID;
			itrHash = pakHash.find(pDep->name);
			if (itrHash != itrHashEnd)	//The package is in the map
			{
				depID = itrHash->second;
				depNode &depN = pakMap.at(depID);
				depN.con.emplace(id, *pDep);
				if (node.ancestor.find(depID) == node.ancestor.end())
					add_ancestor(pakMap, pakHash, depID, id);
				node.dep.emplace(depID);
				if ((depN.pak == NULL && !pDep->check()) || (depN.pak != NULL && !pDep->check(depN.pak->ver)))	//Checking failed
				{
					if (node.ancestor.find(itrHash->second) != node.ancestor.end())	//Loop in map
						throw(msgData[MSGE_DEP] + ':' + node.pak->name + ":" + pDep->fullStr());
					clean_dep(pakMap, pakHash, depID);
					package *pak = find_package(pDep->name, depN.con);
					if (pak == NULL)
						throw(msgData[MSGE_PAK_NOT_FOUND] + ':' + pDep->fullStr());
					depN.pak = pak;
					if (depN.processed)
					{
						depN.processed = false;
						pakQue.push_back(itrHash->second);
					}
				}
			}
			else	//Add the package to the map
			{
				int newID = nextID;
				nextID++;
				depID = newID;
				package *pak;
				if (pDep->check())	//Installed correct package
					pak = NULL;
				else
				{
					pak = find_package(pDep->name, *pDep);
					if (pak == NULL)
						throw(msgData[MSGE_PAK_NOT_FOUND] + ':' + pDep->fullStr());
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
				if (depN.processed)
				{
					depN.processed = false;
					pakQue.push_back(newID);
				}
			}
		}
		node.processed = true;
	}

	//Check which package need to be installed or upgraded
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
		if (pak != NULL)
		{
			if (is_installed(pak->name))
			{
				if (cur_version(pak->name) != pak->ver)
					instList.push_front(instItem(pak, instItem::UPG));
			}
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
		if (pak != NULL)
		{
			newVer = pak->getVer();
			if (con.check(newVer) && newVer > ver)
			{
				ret = pak;
				ver = newVer;
			}
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
		if (pak != NULL)
		{
			newVer = pak->getVer();
			if (newVer > ver)
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
	}
	return ret;
}

errInfo install(const std::string &name)
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

errInfo uninstall(const std::string &name, bool upgrade, bool force)
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
			fs::path upgradePath = dataPath / DIRNAME_UPGRADE / (name + ".inf");
			if (fs::exists(upgradePath))
				fs::remove(upgradePath);
			fs::copy_file(pakPath / FILENAME_BEDEP, upgradePath);
		}
		else if (!force)
		{
			infoStream << msgData[MSGW_PAK_BE_DEP] << std::endl;
			depIn.open((pakPath / FILENAME_BEDEP).string());
			while (!depIn.eof())
			{
				std::getline(depIn, line);
				infoStream << "\t" << depInfo(line).name << std::endl;
			}
			depIn.close();
			return errInfo(msgData[MSGE_PAK_BE_DEP]);
		}
	}

	{
		if (!fs::exists(pakPath / FILENAME_DEP))
			return errInfo(msgData[MSGE_PAK_DEPINFO_NOT_FOUND]);
		depIn.open((pakPath / FILENAME_DEP).string());
		depListTp bedepTmp;
		std::string bedepFile;
		std::fstream bedepFS;
		while (!depIn.eof())
		{
			std::getline(depIn, bedepFile);
			if (!bedepFile.empty())
			{
				bedepFile = (dataPath / bedepFile / FILENAME_BEDEP).string();
				bedepFS.open(bedepFile, std::ios::in);
				depInfo dep;
				while (!bedepFS.eof())
				{
					std::getline(bedepFS, line);
					if (!line.empty())
					{
						dep = line;
						if (dep.name != name)
							bedepTmp.push_back(dep);
					}
				}
				bedepFS.close();
				if (bedepTmp.empty())
					fs::remove(bedepFile);
				else
				{
					bedepFS.open(bedepFile, std::ios::out);
					while (!bedepTmp.empty())
					{
						bedepFS << bedepTmp.front().fullStr() << std::endl;
						bedepTmp.pop_front();
					}
					bedepFS.close();
				}
			}
		}
		depIn.close();
	}

	fs::path logPath = pakPath / FILENAME_INST;
	std::ifstream infIn(logPath.string());
	std::string tmpPathStr;
	std::getline(infIn, tmpPathStr);
	fs::path tmpPath, instPath(tmpPathStr);

	fs::path scriptPath,
		currentPath = fs::current_path(),
		pathPath = fs::system_complete(dataPath / DIRNAME_PATH);
	if (!upgrade)
	{
		scriptPath = pakPath / SCRIPT_PURGE;
		if (exists(scriptPath))
		{
			infoStream << msgData[MSGI_RUNS_PURGE] << std::endl;
			fs::current_path(instPath);
#ifdef WIN32
			int ret = system(("set \"PATH=" + pathPath.string() + ";%PATH%\" & \"" + scriptPath.string() + "\"").c_str());
#endif
#ifdef __linux__
			int ret = system(("PATH=\"" + pathPath.string() + ":$PATH\" ; bash \"" + scriptPath.string() + "\"").c_str());
#endif
			if (ret != 0)
				return errInfo(msgData[MSGE_RUNS] + num2str(ret));
			infoStream << msgData[MSGI_DONE] << std::endl;
		}
	}
	scriptPath = pakPath / SCRIPT_REMOVE;
	if (exists(scriptPath))
	{
		infoStream << msgData[MSGI_RUNS_REMOVE] << std::endl;
		fs::current_path(instPath);
#ifdef WIN32
		int ret = system(("set \"PATH=" + pathPath.string() + ";%PATH%\" & \"" + scriptPath.string() + "\"").c_str());
#endif
#ifdef __linux__
		int ret = system(("PATH=\"" + pathPath.string() + ":$PATH\" ; bash \"" + scriptPath.string() + "\"").c_str());
#endif
		if (ret != 0)
			return errInfo(msgData[MSGE_RUNS] + num2str(ret));
		infoStream << msgData[MSGI_DONE] << std::endl;
	}
	fs::current_path(currentPath);

	try
	{
		infoStream << msgData[MSGI_DELETING] << std::endl;
		while (!infIn.eof())
		{
			std::getline(infIn, tmpPathStr);
			if (!tmpPathStr.empty())
			{
				tmpPath = instPath / tmpPathStr;
				if (!fs::is_directory(tmpPath) || fs::is_empty(tmpPath))
					fs::remove(tmpPath);
			}
		}
		infIn.close();
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

errInfo backup(const std::string &name, bool force)
{
	if (!is_installed(name))
		return errInfo(msgData[MSGE_PAK_NOT_INSTALLED]);
	fs::path pakPath = dataPath / name;
	fs::path backupPath = dataPath / DIRNAME_BACKUP / name;
	if (fs::exists(backupPath))
	{
		if (force)
			fs::remove_all(backupPath);
		else
			return errInfo(msgData[MSGE_BACKUP_EXISTS]);
	}
	fs::create_directory(backupPath);

	fs::path logPath = pakPath / FILENAME_INST;
	std::ifstream infIn(logPath.string());
	std::string tmpPathStr;
	std::getline(infIn, tmpPathStr);
	fs::path tmpPath, instPath(tmpPathStr);

	try
	{
		infoStream << msgData[MSGI_BACKUPING] << std::endl;
		while (!infIn.eof())
		{
			std::getline(infIn, tmpPathStr);
			if (!tmpPathStr.empty())
			{
				tmpPath = tmpPathStr;
				fs::create_directories(backupPath / tmpPath.parent_path());
				if (fs::is_regular_file(instPath / tmpPath))
					fs::copy_file(instPath / tmpPath, backupPath / tmpPath);
			}
		}
		infIn.close();
		
		fs::path newInfoPath = backupPath / DIRNAME_INFO;
		fs::create_directory(newInfoPath);
		for (fs::directory_iterator p(pakPath), pEnd; p != pEnd; p++)
			fs::copy_file(p->path(), newInfoPath / p->path().filename());
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
	infoStream << msgData[MSGI_BACKUPED] << std::endl;
	return errInfo();
}

errInfo recover_from_backup(const std::string &name)
{
	if (is_installed(name))
		return errInfo(msgData[MSGE_PAK_INSTALLED]);

	fs::path backupPath = dataPath / DIRNAME_BACKUP / name, pakPath = dataPath / name;	
	bool flag1 = false, flag2 = false;
	try
	{
		infoStream << msgData[MSGI_RECOVERING] << std::endl;

		fs::create_directory(pakPath);
		flag1 = true;

		for (fs::directory_iterator p(backupPath / DIRNAME_INFO), pEnd; p != pEnd; p++)
			fs::copy(p->path(), pakPath / p->path().filename());
		fs::remove_all(backupPath / DIRNAME_INFO);

		std::ifstream infIn((pakPath / FILENAME_INST).string());
		std::string tmpPath;
		std::getline(infIn, tmpPath);
		fs::path instPath(tmpPath);
		infIn.close();

		std::ofstream infOut((pakPath / FILENAME_INST).string());
		infoStream << msgData[MSGI_COPYING] << std::endl;
		fs::path currentLocalPath = localPath;
		localPath = instPath;
		try
		{
			install_copy(backupPath, fs::path(), infOut, true);
		}
		catch (...)
		{
			localPath = currentLocalPath;
			throw;
		}
		localPath = currentLocalPath;
		infoStream << msgData[MSGI_COPIED] << std::endl;
		infOut.close();
		flag2 = true;

		fs::remove_all(backupPath);
	}
	catch (fs::filesystem_error err)
	{
		if (flag2)
		{
			std::ifstream infIn((pakPath / FILENAME_INST).string());
			std::string tmpPath;
			std::getline(infIn, tmpPath);
			fs::path instPath(tmpPath);
			while (!infIn.eof())
			{
				std::getline(infIn, tmpPath);
				if (!tmpPath.empty())
					fs::remove(instPath / tmpPath);
			}
		}
		if (flag1)
			fs::remove_all(pakPath);
		return errInfo(msgData[MSGE_FS] + err.what());
	}
	catch (std::string err)
	{
		if (flag2)
		{
			std::ifstream infIn((pakPath / FILENAME_INST).string());
			std::string tmpPath;
			std::getline(infIn, tmpPath);
			fs::path instPath(tmpPath);
			while (!infIn.eof())
			{
				std::getline(infIn, tmpPath);
				if (!tmpPath.empty())
					fs::remove(instPath / tmpPath);
			}
		}
		if (flag1)
			fs::remove_all(pakPath);
		return errInfo(err);
	}
	catch (std::exception ex)
	{
		if (flag2)
		{
			std::ifstream infIn((pakPath / FILENAME_INST).string());
			std::string tmpPath;
			std::getline(infIn, tmpPath);
			fs::path instPath(tmpPath);
			while (!infIn.eof())
			{
				std::getline(infIn, tmpPath);
				if (!tmpPath.empty())
					fs::remove(instPath / tmpPath);
			}
		}
		if (flag1)
			fs::remove_all(pakPath);
		return errInfo(ex.what());
	}
	catch (...)
	{
		throw;
	}
	infoStream << msgData[MSGI_RECOVERED] << std::endl;
	return errInfo();
}
