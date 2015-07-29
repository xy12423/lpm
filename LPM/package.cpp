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
#include "stdafx.h"
#include "package.h"
#include "package_core.h"
#include "source.h"
#include "unzip.h"
#include "download.h"

depListTp emptyDepList;

version::version(const std::string &str)
{
	major = minor = revision = 0;
	int tmp[3] = { 0, 0, 0 };
	std::string::const_iterator p = str.cbegin(), pEnd = str.cend();
	for (int i = 0; i < 3; i++)
	{
		for (; p != pEnd; p++)
		{
			if (*p == '.' && i < 2)
			{
				p++;
				break;
			}
			if (!isdigit(*p))
			{
				major = minor = revision = 0;
				return;
			}
			tmp[i] = tmp[i] * 10 + static_cast<UINT>((*p) - '0');
		}
	}
	major = tmp[0]; minor = tmp[1]; revision = tmp[2];
}

depInfo::depInfo(std::string str)
{
	if (str.front() == '&')
		isDep = true;
	else if (str.front() == '!')
		isDep = false;
	str.erase(0, 1);

	std::string::iterator itr = str.begin(), itrEnd = str.end();
	for (; itr != itrEnd; itr++)
	{
		if (*itr == ':')
			break;
		else
			name.push_back(*itr);
	}
	if (itr == itrEnd)
	{
		con = ALL;
		return;
	}
	else
	{
		std::string conStr;
		itr++;
		for (; itr != itrEnd; itr++)
			conStr.push_back(*itr);
		switch (conStr.front())
		{
			case '>':
			{
				conStr.erase(0, 1);
				if (conStr.front() == '=')
				{
					conStr.erase(0, 1);
					con = BIGEQU;
				}
				else
					con = BIGGER;
				break;
			}
			case '<':
			{
				conStr.erase(0, 1);
				if (conStr.front() == '=')
				{
					conStr.erase(0, 1);
					con = LESEQU;
				}
				else
					con = LESS;
				break;
			}
			case '=':
			{
				con = EQU;
				break;
			}
			case '!':
			{
				conStr.erase(0, 1);
				if (conStr.front() == '!')
				{
					con = NONE;
					return;
				}
				else
					con = NEQU;
				break;
			}
			default:
				con = EQU;
		}
		ver = conStr;
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
	return (isDep ? '&' : '!') + name + conStr();
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
	}
	return false;
}

bool depInfo::check()
{
	if ((!is_installed(name)) ^ (con == NONE))
		return false;
	return check(cur_version(name));
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
	fs::path tmpPath = dataPath / DIRNAME_TEMP / name, pakPath = dataPath / name;
	errInfo err = download(source + "/" + name + ".lpm", tmpPath.string() + ".lpm");
	if (err.err)
		return err;

	bool flag1 = false, flag2 = false, flag3 = false;
	errInfo errInTry;
	try
	{
		fs::create_directory(tmpPath);
		flag1 = true;
		fs::create_directory(pakPath);
		flag2 = true;
		infoStream << msgData[MSGI_DECOMPRESSING] << std::endl;
		errInfo err = unzip(tmpPath.string() + ".lpm", tmpPath);
		fs::remove(tmpPath.string() + ".lpm");
		if (err.err)
			throw(err.info);
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
		infOut << extInfo.getFName() << std::endl;
		infOut << extInfo.getInfo() << std::endl;
		infOut << extInfo.getAuthor() << std::endl;
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
			depOut << '&' << name << pDep->conStr() << std::endl;
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
		errInTry = errInfo(msgData[MSGE_FS] + err.what());
	}
	catch (std::string err)
	{
		errInTry = errInfo(err);
	}
	catch (std::exception ex)
	{
		errInTry = errInfo(ex.what());
	}
	catch (...)
	{
		throw;
	}

	if (errInTry.err)
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
		return errInTry;
	}

	infoStream << msgData[MSGI_PAK_INSTALLED] << std::endl;
	return errInfo();
}

int package::instScript(bool upgrade)
{
	fs::path pakPath = dataPath / name,
		scriptPath = pakPath / SCRIPT_INST;
	if (exists(scriptPath))
	{
		infoStream << msgData[MSGI_RUNS_INST] << ':' << name << std::endl;
		int ret = run_script(scriptPath, localPath);
		if (ret != EXIT_SUCCESS)
			return ret;
		infoStream << msgData[MSGI_DONE] << std::endl;
	}
	if (!upgrade)
	{
		scriptPath = pakPath / SCRIPT_INIT;
		if (exists(scriptPath))
		{
			infoStream << msgData[MSGI_RUNS_INIT] << ':' << name << std::endl;
			int ret = run_script(scriptPath, localPath);
			if (ret != EXIT_SUCCESS)
				return ret;
			infoStream << msgData[MSGI_DONE] << std::endl;
		}
	}
	return 0;
}

errInfo package::instList(pakIListTp &instList)
{
	//Install/Upgrade Packages
	pakIListTp::iterator instItr, instEnd;
	infoStream << msgData[MSGI_WILL_DO_LIST] << std::endl;
	instItr = instList.begin();
	instEnd = instList.end();
	for (; instItr != instEnd; instItr++)
	{
		infoStream << "\t";
		switch (instItr->oper)
		{
			case instItem::INST:
				infoStream << msgData[MSGN_INST];
				break;
			case instItem::UPG:
				infoStream << msgData[MSGN_UPG];
				break;
			case instItem::REMOVE:
				infoStream << msgData[MSGN_REMOVE];
				break;
		}
		infoStream << '\t' << instItr->name << std::endl;
	}
	instItr = instList.begin();
	errInfo errInTry;
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
				case instItem::REMOVE:
				{
					errInfo err = uninstall(instItr->name);
					if (err.err)
						throw(err);
				}
			}
		}
	}
	catch (std::exception ex)
	{
		errInTry = errInfo(msgData[MSGE_STD] + ex.what());
	}
	catch (errInfo err)
	{
		errInTry = err;
	}
	catch (...)
	{
		throw;
	}

	if (errInTry.err)
	{
		instEnd = instList.begin();
		while (true)
		{
			if (instItr->oper == instItem::INST)
			{
				if (is_installed(instItr->pak->name))
					uninstall(instItr->pak->name);
			}
			if (instItr == instEnd)
				break;
			else
				instItr--;
		}
		return errInTry;
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
						if (is_installed(rbItr->pak->name))
						{
							errInfo err = uninstall(rbItr->pak->name, false, REMOVE_FORCE);
							if (err.err)
								return err;
						}
					}
				}
				return errInfo(msgData[MSGE_RUNS] + std::to_string(ret));
			}
		}
	}

	return errInfo();
}

errInfo package::instFull(bool force)
{
	infoStream << msgData[MSGI_CHECK_REQUIREMENT] << std::endl;

	pakIListTp instL;
	try
	{
		checkDep(instL, emptyDepList, force);
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

	errInfo err = instList(instL);
	if (err.err)
		return err;

	infoStream << msgData[MSGI_PAKS_INSTALLED] << std::endl;
	return errInfo();
}

bool package::needUpgrade()
{
	if (!is_installed(name))
		return false;
	return cur_version(name) < ver;
}

errInfo package::upgrade(bool checked, bool force)
{
	if (!is_installed(name))
		return errInfo(msgData[MSGE_PAK_NOT_INSTALLED] + ':' + name);

	infoStream << msgData[MSGI_PAK_UPGRADING] << ':' << name << std::endl;

	pakIListTp instL;
	if (!checked)
	{
		if (!needUpgrade())
			return errInfo(msgData[MSGE_PAK_LATEST]);

		depListTp depList;
		if (fs::exists(dataPath / name / FILENAME_BEDEP))
		{
			std::ifstream bedepIn((dataPath / name / FILENAME_BEDEP).string());
			std::string line;
			while (!bedepIn.eof())
			{
				std::getline(bedepIn, line);
				if (!line.empty())
				{
					depInfo dep(line);
					dep.name = name;
					depList.push_back(dep);
				}
			}
			bedepIn.close();
		}

		try
		{
			checkDep(instL, depList, force);
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

	errInfo err = backup(name, true);
	if (err.err)
		return err;
	
	infoStream << msgData[MSGI_PAK_REMOVING] << std::endl;
	err = uninstall(name, true);
	if (err.err)
		return err;
	infoStream << msgData[MSGI_PAK_REINSTALLING] << std::endl;
	if (checked)
	{
		err = inst();
		if (err.err)
			return err;
		int ret = instScript(true);
		if (ret != EXIT_SUCCESS)
		{
			infoStream << msgData[MSGW_RUNS_ROLL_BACK_1] << ret << msgData[MSGW_RUNS_ROLL_BACK_2] << std::endl;
			uninstall(name, false, REMOVE_FORCE);
			recover_from_backup(name);
			return errInfo(msgData[MSGE_RUNS] + std::to_string(ret));
		}
	}
	else
	{
		if (instL.front().pak == this)
			instL.pop_front();
		if (!instL.empty())
		{
			err = instList(instL);
			if (err.err)
				return err;
		}
		err = inst();
		if (err.err)
			return err;
		int ret = instScript(true);
		if (ret != EXIT_SUCCESS)
		{
			infoStream << msgData[MSGW_RUNS_ROLL_BACK_1] << ret << msgData[MSGW_RUNS_ROLL_BACK_2] << std::endl;
			uninstall(name, false, REMOVE_FORCE);
			recover_from_backup(name);
			return errInfo(msgData[MSGE_RUNS] + std::to_string(ret));
		}
	}
	return errInfo();
}

bool package::check()
{
	try
	{
		pakIListTp willBeIgnored;
		checkDep(willBeIgnored, emptyDepList);
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

package* find_package(const std::string &name, std::unordered_multimap<int, depInfo> &con, bool force, std::list<int> &removeList)
{
	removeList.clear();
	size_t removeCount = removeList.max_size();
	srcListTp::const_iterator p, pEnd = sourceList.cend();
	package *pak, *ret = NULL;
	version ver, newVer;
	std::list<int> tmpRemoveList;
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		pak = (*p)->find_package(name);
		if (pak != NULL)
		{
			newVer = pak->getVer();
			if (newVer > ver)
			{
				tmpRemoveList.clear();
				bool flag = true;
				for (std::unordered_multimap<int, depInfo>::iterator itr = con.begin(), itrEnd = con.end(); itr != itrEnd; itr++)
				{
					if (!itr->second.check(newVer))
					{
						if (force && !itr->second.isDep)
							tmpRemoveList.push_back(itr->first);
						else
						{
							flag = false;
							break;
						}
					}
				}
				if (flag && (!force || (tmpRemoveList.size() <= removeCount)))
				{
					ret = pak;
					ver = newVer;
					removeList = tmpRemoveList;
					removeCount = tmpRemoveList.size();
				}
			}
		}
	}
	return ret;
}

errInfo install(const std::string &name, bool force)
{
	if (is_installed(name))
		return errInfo(msgData[MSGE_PAK_INSTALLED]);

	infoStream << msgData[MSGI_PAK_SEARCHING] << std::endl;
	package *pak = find_package(name);
	if (pak == NULL)
		return errInfo(msgData[MSGE_PAK_NOT_FOUND]);
	infoStream << msgData[MSGI_PAK_FOUND] << ':' << name << std::endl;
	
	return pak->instFull(force);
}

void uninst_list(const std::string &name, pakRListTp &removeList, pakRHashTp &removeHash)
{
	std::ifstream depIn;
	std::string line;

	pakRHashTp::iterator depHashEnd = removeHash.end();
	pakRListTp depQue;
	if (removeHash.find(name) != depHashEnd)
		return;
	removeHash.emplace(name);
	depQue.push_back(name);
	std::string nextName;

	while (!depQue.empty())
	{
		std::string &thisName = depQue.front();
		if (fs::exists(dataPath / thisName / FILENAME_BEDEP))
		{
			depIn.open((dataPath / thisName / FILENAME_BEDEP).string());
			while (!depIn.eof())
			{
				std::getline(depIn, line);
				if (!line.empty())
				{
					nextName = depInfo(line).name;
					if (removeHash.find(nextName) == depHashEnd)
					{
						removeHash.emplace(nextName);
						depQue.push_back(nextName);
						removeList.push_front(nextName);
					}
				}
			}
			depIn.close();
		}
		depQue.pop_front();
	}
}

errInfo uninstall(const std::string &name, bool upgrade, remove_level level)
{
	if (!is_installed(name))
		return errInfo(msgData[MSGE_PAK_NOT_INSTALLED] + ':' + name);
	fs::path pakPath = dataPath / name;

	if (fs::exists(pakPath / FILENAME_BEDEP))
	{
		if (upgrade)
		{
			fs::path upgradePath = dataPath / DIRNAME_UPGRADE / (name + ".inf");
			if (fs::exists(upgradePath))
				fs::remove(upgradePath);
			fs::copy_file(pakPath / FILENAME_BEDEP, upgradePath);
		}
		else if (level == REMOVE_RECURSIVE)
		{
			pakRListTp removeQue;
			pakRHashTp removeHash;
			uninst_list(name, removeQue, removeHash);

			if (!removeQue.empty())
			{
				infoStream << msgData[MSGI_WILL_REMOVE_LIST] << std::endl;
				std::for_each(removeQue.begin(), removeQue.end(), [](std::string &pak){
					infoStream << "\t" << pak << std::endl;
				});
				infoStream << "\t" << name << std::endl;

				while (!removeQue.empty())
				{
					uninstall(removeQue.front(), false, REMOVE_FORCE);
					removeQue.pop_front();
				}
			}
		}
		else if (level < REMOVE_FORCE)
		{
			infoStream << msgData[MSGW_PAK_BE_DEP] << std::endl;
			pakRListTp depList;
			pakRHashTp depHash;
			uninst_list(name, depList, depHash);
			while (!depList.empty())
			{
				infoStream << "\t" << depList.front() << std::endl;
				depList.pop_front();
			}
			return errInfo(msgData[MSGE_PAK_BE_DEP]);
		}
	}

	if (!upgrade)
		infoStream << msgData[MSGI_PAK_REMOVING] << ':' << name << std::endl;

	{
		std::ifstream depIn;
		std::string line;
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
				if (fs::exists(bedepFile))
				{
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
		}
		depIn.close();
	}

	fs::path logPath = pakPath / FILENAME_INST;
	std::ifstream infIn(logPath.string());
	std::string tmpPathStr;
	std::getline(infIn, tmpPathStr);
	fs::path tmpPath, instPath(tmpPathStr);

	fs::path scriptPath;
	if (!upgrade)
	{
		scriptPath = pakPath / SCRIPT_PURGE;
		if (exists(scriptPath))
		{
			infoStream << msgData[MSGI_RUNS_PURGE] << std::endl;
			int ret = run_script(scriptPath, instPath);
			if (ret != 0)
				return errInfo(msgData[MSGE_RUNS] + std::to_string(ret));
			infoStream << msgData[MSGI_DONE] << std::endl;
		}
	}
	scriptPath = pakPath / SCRIPT_REMOVE;
	if (exists(scriptPath))
	{
		infoStream << msgData[MSGI_RUNS_REMOVE] << std::endl;
		int ret = run_script(scriptPath, instPath);
		if (ret != 0)
			return errInfo(msgData[MSGE_RUNS] + std::to_string(ret));
		infoStream << msgData[MSGI_DONE] << std::endl;
	}

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
		return errInfo(msgData[MSGE_PAK_NOT_INSTALLED] + ':' + name);
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

	errInfo err;
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
	catch (fs::filesystem_error ex)
	{
		err = msgData[MSGE_FS] + ex.what();
	}
	catch (std::string ex)
	{
		err = ex;
	}
	catch (std::exception ex)
	{
		err = msgData[MSGE_STD] + ex.what();
	}
	catch (...)
	{
		throw;
	}

	if (err.err)
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
				if (!tmpPath.empty() && fs::exists(instPath / tmpPath))
					fs::remove(instPath / tmpPath);
			}
		}
		if (flag1)
			fs::remove_all(pakPath);
		return err;
	}
	infoStream << msgData[MSGI_RECOVERED] << std::endl;
	return errInfo();
}
