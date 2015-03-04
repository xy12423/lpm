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
		std::string name, tmpName;
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
			tmpPkgList.push_back(new package(tmpPath, name, ver, depList, confList, pakExtInfo(author, info)));
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
			foutCfg << (*pP)->extInfo.author << std::endl << (*pP)->extInfo.info << std::endl;

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

using namespace std;

void printUsage()
{
	cout << "Usage:" << endl;
	cout << "\tlpm init" << endl;
	cout << "\tlpm update" << endl;
	cout << "\tlpm install <package name>" << endl;
	cout << "\tlpm remove <package name>" << endl;
	cout << "\tlpm list" << endl;
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
	if (readConfig())
		checkPath();
	readSource();

	string cmd(argv[1]);
	if (cmd == "init")
	{
		localPath = "./local";
		dataPath = "./data";
		writeConfig();
		writeSource();
	}
	else if (cmd == "install")
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
				cout << name << endl;
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

	return 0;
}
