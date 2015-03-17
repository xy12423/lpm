#include "stdafx.h"
#include "main.h"
using namespace boost::filesystem;

bool readConfig()
{
	if (!exists(".config"))
	{
		return false;
	}
	std::ifstream finCfg(".config");
	std::string tmpPath;
	std::getline(finCfg, tmpPath);
	localPath = tmpPath;
	std::getline(finCfg, tmpPath);
	dataPath = tmpPath;
	finCfg.close();
	return true;
}

void writeConfig()
{
	std::ofstream foutCfg(".config");
	foutCfg << localPath.string() << std::endl;
	foutCfg << dataPath.string() << std::endl;
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
	if (!exists(dataPath / DIRNAME_UPGRADE))
		create_directory(dataPath / DIRNAME_UPGRADE);
	else if (!is_directory(dataPath / DIRNAME_UPGRADE))
	{
		remove(dataPath / DIRNAME_UPGRADE);
		create_directory(dataPath / DIRNAME_UPGRADE);
	}
}

bool readSource()
{
	if (!exists(".source"))
	{
		return false;
	}
	std::ifstream finCfg(".source");
	std::string tmpPath, eatCRLF;
	int sourceCount;
	finCfg >> sourceCount;
	std::getline(finCfg, eatCRLF);
	sourceList.clear();
	int i, j, k;
	std::vector<package*> tmpPkgList;
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
			
			for (k = 0; k < depCount; k++)
			{
				std::getline(finCfg, tmpName);
				depList.push_back(tmpName);
			}
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
	std::ofstream foutCfg(".source");
	foutCfg << sourceList.size() << std::endl;
	std::vector<source*>::const_iterator p, pEnd = sourceList.cend();
	std::vector<package*>::const_iterator pP, pPEnd;
	depListTp::const_iterator pD, pDEnd;
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		foutCfg << (*p)->add << std::endl;
		foutCfg << (*p)->pkgList.size() << std::endl;

		for (pP = (*p)->pkgList.cbegin(), pPEnd = (*p)->pkgList.cend(); pP != pPEnd; pP++)
		{
			foutCfg << (*pP)->name << std::endl;
			foutCfg << (*pP)->ver.major << ' ' << (*pP)->ver.minor << ' ' << (*pP)->ver.revision << std::endl;
			foutCfg << (*pP)->depList.size() << ' ' << (*pP)->confList.size() << std::endl;
			foutCfg << (*pP)->extInfo.fname << std::endl << (*pP)->extInfo.author << std::endl << (*pP)->extInfo.info << std::endl;

			for (pD = (*pP)->depList.cbegin(), pDEnd = (*pP)->depList.cend(); pD != pDEnd; pD++)
			{
				foutCfg << *pD << std::endl;
			}
			for (pD = (*pP)->confList.cbegin(), pDEnd = (*pP)->confList.cend(); pD != pDEnd; pD++)
			{
				foutCfg << *pD << std::endl;
			}
		}
	}
	foutCfg.close();
}

errInfo update()
{
	std::vector<source*>::const_iterator p, pEnd = sourceList.cend();
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		infoStream << "I:Updating source " << (*p)->getAdd() << std::endl;
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
		return errInfo("E:Package not found");
	return pkg->upgrade();
}

errInfo upgrade()
{
	std::vector<source*>::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
	errInfo err;
	for (; pSrc != pSrcEnd; pSrc++)
	{
		(*pSrc)->upgradeAll();
	}
	return errInfo();
}

bool check(std::string name)
{
	if (is_installed(name))
		return true;
	package *pkg = find_package(name);
	if (pkg == NULL)
		throw("E:Package not found");
	return pkg->check();
}

void init()
{
	localPath = "./local";
	dataPath = "./data";
	writeConfig();
	writeSource();
}
