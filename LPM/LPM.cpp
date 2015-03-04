#include "stdafx.h"
#include "globalVar.h"
#include "unzip.h"
#include "package.h"
#include "source.h"
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
			foutCfg << (*pP)->extInfo.fname << (*pP)->extInfo.author << std::endl << (*pP)->extInfo.info << std::endl;

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

void printInfo(package *pkg)
{
	if (pkg == NULL)
		return;
	std::cout << "Name:" << pkg->extInfo.fname << std::endl;
	std::cout << "Package:" << pkg->name << std::endl;
	std::cout << "Description:" << pkg->extInfo.info << std::endl;
	std::cout << "Author:" << pkg->extInfo.author << std::endl;
	std::cout << "Version:" << pkg->ver.major << '.' << pkg->ver.minor << '.' << pkg->ver.revision << std::endl;
	std::cout << "Required:";
	std::for_each(pkg->depList.begin(), pkg->depList.end(), [](std::string pkgName){
		std::cout << pkgName << ';';
	});
	std::cout << std::endl << "Conflict:";
	std::for_each(pkg->confList.begin(), pkg->confList.end(), [](std::string pkgName){
		std::cout << pkgName << ';';
	});
	std::cout << std::endl;
	std::cout << "Is installed:";
	if (is_installed(pkg->name))
		std::cout << "Y";
	else
		std::cout << "N";
	std::cout << std::endl;
}

bool printInfoFromFile(const std::string &name)
{
	if (!is_installed(name))
		return false;
	std::ifstream infoIn((dataPath / name / FILENAME_INFO).string());
	std::string line;

	std::getline(infoIn, line);
	std::cout << "Name:" << line << std::endl;
	std::getline(infoIn, line);
	std::cout << "Package:" << line << std::endl;
	std::getline(infoIn, line);
	std::cout << "Description:" << line << std::endl;
	std::getline(infoIn, line);
	std::cout << "Author:" << line << std::endl;
	std::getline(infoIn, line);
	std::cout << "Version:" << line << std::endl;
	std::getline(infoIn, line);
	std::cout << "Required:" << line << std::endl;
	std::getline(infoIn, line);
	std::cout << "Conflict:" << line << std::endl;
	std::cout << "Is installed:Y" << std::endl;

	infoIn.close();
	return true;
}

void printAvaliable(source *src, bool ignoreInstalled = true)
{
	if (src == NULL)
		return;
	std::vector<package*>::const_iterator p = src->pkgList.cbegin(), pEnd = src->pkgList.cend();
	for (; p != pEnd; p++)
		if (!is_installed((*p)->getName()))
			printInfo(*p);
}
void printAvaliableShort(source *src, bool ignoreInstalled = true)
{
	if (src == NULL)
		return;
	std::vector<package*>::const_iterator p = src->pkgList.cbegin(), pEnd = src->pkgList.cend();
	for (; p != pEnd; p++)
		if (!is_installed((*p)->getName()))
			std::cout << (*p)->getName() << std::endl;
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

using namespace std;

void printUsage()
{
	cout << "Usage:" << endl;
	cout << "\tlpm init" << endl;
	cout << "\tlpm update" << endl;
	cout << "\tlpm install <package name>" << endl;
	cout << "\tlpm remove <package name>" << endl;
	cout << "\tlpm info <package name>" << endl;
	cout << "\tlpm check <package name>" << endl;
	cout << "\tlpm list" << endl;
	cout << "\tlpm list-short" << endl;
	cout << "\tlpm avaliable" << endl;
	cout << "\tlpm avaliable-short" << endl;
	cout << "\tlpm listsrc" << endl;
	cout << "\tlpm addsrc <source address>" << endl;
	cout << "\tlpm delsrc <source address>" << endl;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printUsage();
		return 0;
	}
	
	string cmd(argv[1]);
	if (cmd == "init")
	{
		init();
	}
	else
	{
		if (readConfig())
			checkPath();
		else
		{
			init();
			checkPath();
		}
		readSource();
		if (cmd == "install")
		{
			if (argc < 3)
			{
				printUsage();
				return 0;
			}
			errInfo err = install(argv[2]);
			if (err.err)
			{
				cout << err.info << endl;
				return 0;
			}
		}
		else if (cmd == "remove")
		{
			if (argc < 3)
			{
				printUsage();
				return 0;
			}
			errInfo err = uninstall(argv[2]);
			if (err.err)
			{
				cout << err.info << endl;
				return 0;
			}
		}
		else if (cmd == "info")
		{
			if (argc < 3)
			{
				printUsage();
				return 0;
			}
			string name = std::string(argv[2]);
			if (is_installed(name))
				printInfoFromFile(name);
			else
			{
				package *pkg = find_package(name);
				if (pkg == NULL)
				{
					cout << "E:Package not found" << endl;
					return 0;
				}
				printInfo(pkg);
			}
		}
		else if (cmd == "check")
		{
			if (argc < 3)
			{
				printUsage();
				return 0;
			}
			try
			{
				if (check(argv[2]))
					cout << "OK" << endl;
				else
					cout << "NO" << endl;
			}
			catch (const char* err)
			{
				cout << err << endl;
			}
			catch (...)
			{
				throw;
			}
		}
		else if (cmd == "update")
		{
			errInfo err = update();
			if (err.err)
			{
				cout << err.info << endl;
				return 0;
			}
		}
		else if (cmd == "list")
		{
			directory_iterator p(dataPath), pEnd;
			string name;
			for (; p != pEnd; p++)
			{
				name = p->path().filename().string();
				if (name.front() != '$')
					printInfoFromFile(name);
			}
		}
		else if (cmd == "list-short")
		{
			directory_iterator p(dataPath), pEnd;
			string name;
			for (; p != pEnd; p++)
			{
				name = p->path().filename().string();
				if (name.front() != '$')
					cout << name << endl;
			}
		}
		else if (cmd == "avaliable")
		{
			std::vector<source*>::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
			for (; pSrc != pSrcEnd; pSrc++)
			{
				source* src = *pSrc;
				printAvaliable(*pSrc);
			}
		}
		else if (cmd == "avaliable-short")
		{
			std::vector<source*>::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
			for (; pSrc != pSrcEnd; pSrc++)
			{
				source* src = *pSrc;
				printAvaliableShort(*pSrc);
			}
		}
		else if (cmd == "listsrc")
		{
			vector<source*>::const_iterator p = sourceList.cbegin(), pEnd = sourceList.cend();
			for (; p != pEnd; p++)
				cout << (*p)->getAdd() << endl;
		}
		else if (cmd == "addsrc")
		{
			if (argc < 3)
			{
				printUsage();
				return 0;
			}
			source *newSrc = new source(argv[2]);
			newSrc->loadRemote();
			sourceList.push_back(newSrc);
			writeSource();
		}
		else if (cmd == "delsrc")
		{
			if (argc < 3)
			{
				printUsage();
				return 0;
			}
			string name(argv[2]);
			vector<source*>::const_iterator p = sourceList.cbegin(), pEnd = sourceList.cend();
			bool found = false;
			for (; p != pEnd; p++)
			{
				if ((*p)->getAdd() == name)
				{
					found = true;
					sourceList.erase(p);
					break;
				}
			}
			writeSource();
			if (found)
				cout << "E:Source not found" << endl;
			else
				cout << "I:Deleted" << endl;
		}
	}

	return 0;
}
