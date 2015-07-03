#include "stdafx.h"
#include "main.h"

const std::string FILENAME_SOURCE = "$source";

bool readConfig()
{
	if (!fs::exists(".config"))
	{
		return false;
	}
	std::ifstream finCfg(".config");
	std::string tmpPath;
	std::getline(finCfg, tmpPath);
	localPath = tmpPath;
	std::getline(finCfg, tmpPath);
	dataPath = tmpPath;
	std::getline(finCfg, tmpPath);
	langPath = tmpPath;
	finCfg.close();
	return true;
}

void writeConfig()
{
	std::ofstream foutCfg(".config");
	foutCfg << localPath.string() << std::endl;
	foutCfg << dataPath.string() << std::endl;
	foutCfg << langPath.string() << std::endl;
	foutCfg.close();
}

void checkPath()
{
	if (!exists(dataPath))
		create_directory(dataPath);
	else if (!is_directory(dataPath))
	{
		remove(dataPath);
		create_directory(dataPath);
	}
	if (!exists(dataPath / DIRNAME_TEMP))
		create_directory(dataPath / DIRNAME_TEMP);
	else if (!is_directory(dataPath / DIRNAME_TEMP))
	{
		remove(dataPath / DIRNAME_TEMP);
		create_directory(dataPath / DIRNAME_TEMP);
	}
	if (!exists(dataPath / DIRNAME_NATIVE))
		create_directory(dataPath / DIRNAME_NATIVE);
	else if (!is_directory(dataPath / DIRNAME_NATIVE))
	{
		remove(dataPath / DIRNAME_NATIVE);
		create_directory(dataPath / DIRNAME_NATIVE);
	}
	if (!exists(dataPath / DIRNAME_BACKUP))
		create_directory(dataPath / DIRNAME_BACKUP);
	else if (!is_directory(dataPath / DIRNAME_BACKUP))
	{
		remove(dataPath / DIRNAME_BACKUP);
		create_directory(dataPath / DIRNAME_BACKUP);
	}
	if (!exists(dataPath / DIRNAME_UPGRADE))
		create_directory(dataPath / DIRNAME_UPGRADE);
	else if (!is_directory(dataPath / DIRNAME_UPGRADE))
	{
		remove(dataPath / DIRNAME_UPGRADE);
		create_directory(dataPath / DIRNAME_UPGRADE);
	}
	if (!exists(dataPath / DIRNAME_PATH))
		create_directory(dataPath / DIRNAME_PATH);
	else if (!is_directory(dataPath / DIRNAME_PATH))
	{
		remove(dataPath / DIRNAME_PATH);
		create_directory(dataPath / DIRNAME_PATH);
	}
	if (!exists(dataPath / FILENAME_SOURCE))
		writeSource();
	else if (!is_directory(dataPath / DIRNAME_PATH))
	{
		remove(dataPath / DIRNAME_PATH);
		writeSource();
	}
}

bool readSource()
{
	if (!exists(dataPath / FILENAME_SOURCE))
	{
		return false;
	}
	std::ifstream finCfg((dataPath / FILENAME_SOURCE).string());
	std::string tmpPath, eatCRLF;
	int sourceCount;
	finCfg >> sourceCount;
	std::getline(finCfg, eatCRLF);
	sourceList.clear();
	int i, j, k;
	pakListTp tmpPkgList;
	for (i = 0; i < sourceCount; i++)
	{
		std::getline(finCfg, tmpPath);
		source* src = new source(tmpPath);
		int pkgCount;
		finCfg >> pkgCount;
		std::getline(finCfg, eatCRLF);

		tmpPkgList.clear();
		std::string name, fname, tmpName;
		std::string author, info;
		version ver;
		depListTp depList, confList;
		int depCount, confCount;

		for (j = 0; j < pkgCount; j++)
		{
			std::getline(finCfg, name);
			finCfg >> ver.major >> ver.minor >> ver.revision;
			finCfg >> depCount >> confCount;
			std::getline(finCfg, eatCRLF);
			std::getline(finCfg, fname);
			std::getline(finCfg, author);
			std::getline(finCfg, info);
			
			depList.clear();
			for (k = 0; k < depCount; k++)
			{
				std::getline(finCfg, tmpName);
				depList.push_back(tmpName);
			}
			confList.clear();
			for (k = 0; k < confCount; k++)
			{
				std::getline(finCfg, tmpName);
				confList.push_back(tmpName);
			}
			tmpPkgList.push_back(new package(tmpPath, name, ver, depList, confList, pakExtInfo(fname, author, info)));
		}
		
		src->loadLocal(tmpPkgList);
		sourceList.push_back(src);
	}
	finCfg.close();
	return true;
}

void writeSource()
{
	std::ofstream foutCfg((dataPath / FILENAME_SOURCE).string());
	foutCfg << sourceList.size() << std::endl;
	srcListTp::const_iterator p, pEnd = sourceList.cend();
	pakListTp::const_iterator pP, pPEnd;
	depListTp::iterator pD, pDEnd;
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		foutCfg << (*p)->add << std::endl;
		foutCfg << (*p)->pakList.size() << std::endl;

		for (pP = (*p)->pakList.cbegin(), pPEnd = (*p)->pakList.cend(); pP != pPEnd; pP++)
		{
			foutCfg << (*pP)->name << std::endl;
			foutCfg << (*pP)->ver.major << ' ' << (*pP)->ver.minor << ' ' << (*pP)->ver.revision << std::endl;
			foutCfg << (*pP)->depList.size() << ' ' << (*pP)->confList.size() << std::endl;
			foutCfg << (*pP)->extInfo.getFName() << std::endl << (*pP)->extInfo.getAuthor() << std::endl << (*pP)->extInfo.getInfo() << std::endl;

			for (pD = (*pP)->depList.begin(), pDEnd = (*pP)->depList.end(); pD != pDEnd; pD++)
			{
				foutCfg << pD->fullStr() << std::endl;
			}
			for (pD = (*pP)->confList.begin(), pDEnd = (*pP)->confList.end(); pD != pDEnd; pD++)
			{
				foutCfg << pD->fullStr() << std::endl;
			}
		}
	}
	foutCfg.close();
}

bool readLocal()
{
	return true;
}

void writeLocal()
{
	
}

void loadDefaultLang()
{
	for (int i = 0; i < msgCount; i++)
		msgData[i] = msgDataDefault[i];
}

bool readLang()
{
	if (!exists(langPath) || is_directory(langPath))
		return false;
	std::ifstream fin(langPath.string());
	if (!fin.is_open())
		return false;
	for (int i = 0; i < msgCount; i++)
	{
		if (fin.eof())
			return false;
		std::getline(fin, msgData[i]);
	}
	fin.close();
	return true;
}

errInfo update()
{
	srcListTp::const_iterator p, pEnd = sourceList.cend();
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		infoStream << msgData[MSGI_SRCINFO_REFING] << ':' << (*p)->getAdd() << std::endl;
		errInfo err = (*p)->loadRemote();
		if (err.err)
			return err;
	}
	writeSource();
	return errInfo();
}

errInfo upgrade(std::string name)
{
	package *pkg = find_package(name);
	if (pkg == NULL)
		return errInfo(msgData[MSGE_PAK_NOT_FOUND]);
	return pkg->upgrade();
}

errInfo upgrade()
{
	srcListTp::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
	errInfo err;
	for (; pSrc != pSrcEnd; pSrc++)
	{
		(*pSrc)->upgradeAll();
	}
	return errInfo();
}

void checkUpgrade(pakListTp &ret)
{
	srcListTp::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
	errInfo err;
	for (; pSrc != pSrcEnd; pSrc++)
	{
		(*pSrc)->checkUpgrade(ret);
	}
}

bool check(std::string name)
{
	if (is_installed(name))
		return true;
	package *pkg = find_package(name);
	if (pkg == NULL)
		throw(msgData[MSGE_PAK_NOT_FOUND]);
	return pkg->check();
}

int getState(std::string name)
{
	int state = PAK_STATE_DEFAULT;
	if (is_installed(name))
		state |= PAK_STATE_INSTALLED;
	package *pak = find_package(name);
	if (pak != NULL)
	{
		state |= PAK_STATE_IN_SOURCE;
		if (pak->needUpgrade())
			state |= PAK_STATE_NEED_UPGRADE;
	}
	return state;
}

void init()
{
	localPath = "./local";
	dataPath = "./data";
	langPath = "./lpm-lang";
	checkPath();
	writeConfig();
	writeSource();
}

bool lock()
{
	fs::path lockPath = localPath / FILENAME_LOCK;
	if (fs::exists(lockPath))
		return false;
	std::ofstream lockOut(lockPath.string());
	lockOut.close();
	return true;
}

void unlock()
{
	fs::path lockPath = localPath / FILENAME_LOCK;
	if (fs::exists(lockPath))
		remove(lockPath);
}
