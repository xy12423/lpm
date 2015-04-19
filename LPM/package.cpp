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
		std::for_each(depList.begin(), depList.end(), [this, &infOut](std::string pkgName){
			infOut << pkgName << ';';
		});
		infOut << std::endl;
		std::for_each(confList.begin(), confList.end(), [this, &infOut](std::string pkgName){
			infOut << pkgName << ';';
		});
		infOut << std::endl;
		infOut.close();

		depListTp::const_iterator pDep = depList.begin(), pDepEnd = depList.end();
		std::ofstream depOut;
		for (; pDep != pDepEnd; pDep++)
		{
			depOut.open((dataPath / *pDep / FILENAME_BEDEP).string(), std::ios::out | std::ios::app);
			depOut << name << std::endl;
			depOut.close();
		}
		pDep = depList.begin();
		depOut.open((pakPath / FILENAME_DEP).string());
		for (; pDep != pDepEnd; pDep++)
			depOut << *pDep << std::endl;
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

errInfo package::instFull()
{
	depListTp::const_iterator pDep, pDepEnd;
	pDep = confList.begin();
	pDepEnd = confList.end();
	depListTp pakList;
	infoStream << msgData[MSGI_CHECK_REQUIREMENT] << std::endl;
	//Check Confliction
	for (; pDep != pDepEnd; pDep++)
		if (is_installed(*pDep))
			pakList.push_back(*pDep);
	if (!pakList.empty())
	{
		infoStream << msgData[MSGW_CONF] << std::endl;
		while (!pakList.empty())
		{
			infoStream << "\t" << pakList.front() << std::endl;
			pakList.pop_front();
		}
		return errInfo(msgData[MSGE_CONF]);
	}

	//Check dependence:init
	std::list<package *> instList;
	depListTp pakQue;
	depMapTp pakHash;
	depMapTp::const_iterator pakHashEnd = pakHash.cend();
	depListTp::const_iterator pConf, pConfEnd;
	instList.push_front(this);
	pakHash.emplace(name);

	pDep = depList.begin();
	pDepEnd = depList.end();
	for (; pDep != pDepEnd; pDep++)
	{
		if (!is_installed(*pDep))
		{
			package *depPak = find_package(*pDep);
			if (depPak == NULL)
				return errInfo(msgData[MSGE_PAK_NOT_FOUND] + ':' + *pDep);
			pConf = depPak->confList.cbegin();
			pConfEnd = depPak->confList.cend();
			for (; pConf != pConfEnd; pConf++)
				if (is_installed(*pConf) || (pakHash.find(*pConf) != pakHashEnd))
					return errInfo(msgData[MSGE_CONF] + ':' + depPak->name + "<->" + *pConf);
			instList.push_front(depPak);
			pakQue.push_back(*pDep);
			pakHash.emplace(*pDep);
		}
	}

	//Check dependence:BFS
	while (!pakQue.empty())
	{
		package *depPak = find_package(pakQue.front());
		pakQue.pop_front();
		if (depPak == NULL)
			return errInfo(msgData[MSGE_PAK_NOT_FOUND] + ':' + pakQue.front());
		pDep = depPak->depList.begin();
		pDepEnd = depPak->depList.end();
		for (; pDep != pDepEnd; pDep++)
		{
			if (is_installed(*pDep) == false && pakHash.find(*pDep) != pakHash.end())
			{
				pConf = depPak->confList.cbegin();
				pConfEnd = depPak->confList.cend();
				for (; pConf != pConfEnd; pConf++)
					if (is_installed(*pConf) || (pakHash.find(*pConf) != pakHashEnd))
						return errInfo(msgData[MSGE_CONF] + ':' + depPak->name + "<->" + *pConf);
				instList.push_front(depPak);
				pakQue.push_back(*pDep);
				pakHash.emplace(*pDep);
			}
		}
	}

	//Install Packages
	std::list<package *>::iterator instItr, instEnd;
	infoStream << msgData[MSGI_WILL_INST_LIST] << std::endl;
	instItr = instList.begin();
	instEnd = instList.end();
	for (; instItr != instEnd; instItr++)
		infoStream << "\t" << (*instItr)->name << std::endl;
	instItr = instList.begin();
	for (; instItr != instEnd; instItr++)
	{
		infoStream << msgData[MSGI_PAK_INSTALLING] << ':' << (*instItr)->name << std::endl;
		errInfo err = (*instItr)->inst();
		if (err.err)
			return err;
	}
	//Run scripts
	instItr = instList.begin();
	for (; instItr != instEnd; instItr++)
	{
		int ret = (*instItr)->instScript();
		if (ret != EXIT_SUCCESS)
		{
			infoStream << msgData[MSGW_RUNS_ROLL_BACK_1] << ret << msgData[MSGW_RUNS_ROLL_BACK_2] << std::endl;
			std::list<package *>::reverse_iterator rbItr, rbEnd = instList.rend();
			rbItr = instList.rbegin();
			for (; rbItr != rbEnd; rbItr++)
			{
				infoStream << msgData[MSGI_PAK_REMOVING] << ':' << (*rbItr)->name << std::endl;
				errInfo err = uninstall((*rbItr)->name);
				if (err.err)
					return err;
			}
			return errInfo(msgData[MSGE_RUNS] + num2str(ret));
		}
	}

	infoStream << msgData[MSGI_PAKS_INSTALLED] << std::endl;
	return errInfo();
}

bool package::needUpgrade()
{
	if (!is_installed(name))
		return false;

	std::string line;
	std::ifstream infoIn((dataPath / name / FILENAME_INFO).string());
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	infoIn.close();
	version oldver(line);

	return oldver < ver;
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
	err = install(name);
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
		if (is_installed(*pDep))
			return false;
	pDep = depList.begin();
	pDepEnd = depList.end();
	for (; pDep != pDepEnd; pDep++)
		if (!is_installed(*pDep))
			return false;
	return true;
}

bool is_installed(std::string name)
{
	return fs::exists(dataPath / name) && fs::is_directory(dataPath / name);
}

package* find_package(const std::string &name)
{
	srcListTp::const_iterator p, pEnd = sourceList.cend();
	package *pak;
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		pak = (*p)->find_package(name);
		if (pak != NULL)
			return pak;
	}
	return NULL;
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
