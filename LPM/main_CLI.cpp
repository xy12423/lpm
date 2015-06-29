#include "stdafx.h"

#ifndef _LPM_GUI

#include "main.h"

using namespace boost::filesystem;
using namespace std;

int lastProgress = -1;
void reportProgress(double progress)
{
	if (static_cast<int>(progress) != lastProgress)
	{
		std::cout << static_cast<int>(progress) << "%    \r";
		lastProgress = static_cast<int>(progress);
	}
	return ;
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
	std::for_each(pkg->depList.begin(), pkg->depList.end(), [](depInfo dpInf){
		std::cout << dpInf.fullStr() << ';';
	});
	std::cout << std::endl << "Conflict:";
	std::for_each(pkg->confList.begin(), pkg->confList.end(), [](depInfo dpInf){
		std::cout << dpInf.fullStr() << ';';
	});
	std::cout << std::endl;
	std::cout << "Is installed:";
	if (is_installed(pkg->name))
		std::cout << "Y";
	else
		std::cout << "N";
	std::cout << std::endl;
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
	std::cout << "Package:" << name << std::endl;
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
	std::cout << std::endl;

	infoIn.close();
	return true;
}
void printAvailable(source *src, bool ignoreInstalled = true)
{
	if (src == NULL)
		return;
	pakListTp::const_iterator p = src->pakList.cbegin(), pEnd = src->pakList.cend();
	for (; p != pEnd; p++)
		printInfo(*p);
}
void printAvailableShort(source *src, bool ignoreInstalled = true)
{
	if (src == NULL)
		return;
	pakListTp::const_iterator p = src->pakList.cbegin(), pEnd = src->pakList.cend();
	for (; p != pEnd; p++)
		std::cout << (*p)->getName() << std::endl;
}

void printUsage()
{
	cout << "Usage:" << endl;
	cout << "\tlpm init" << endl;
	cout << "\tlpm update" << endl;
	cout << "\tlpm install <package name>" << endl;
	cout << "\tlpm remove <package name>" << endl;
	cout << "\tlpm dataclear <package name>" << endl;
	cout << "\tlpm upgrade [package name]" << endl;
	cout << "\tlpm info <package name>" << endl;
	cout << "\tlpm check <package name>" << endl;
	cout << "\tlpm list" << endl;
	cout << "\tlpm list-short" << endl;
	cout << "\tlpm available" << endl;
	cout << "\tlpm available-short" << endl;
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
	bool locked = false;

	try
	{
		int argp = 1;
		string cmd;
		string newPath, newLocal, newData;
		bool force = false;
		while (argp < argc)
		{
			cmd = argv[argp];
			if (cmd.front() != '-')
				break;
			cmd.erase(0, 1);
			switch (cmd.front())
			{
				case '-':
				{
					cmd.erase(0, 1);
					if (cmd.substr(0, 7) == "lpmdir=")
						newPath = cmd.substr(7);
					else if (cmd.substr(0, 6) == "local=")
						newLocal = cmd.substr(6);
					else if (cmd.substr(0, 5) == "data=")
						newData = cmd.substr(5);
					else if (cmd.substr(0, 5) == "force")
						force = true;
					else
					{
						printUsage();
						throw(0);
					}
					break;
				}
				default:
					printUsage();
					throw(0);
			}
			argp++;
		}
		
		if (cmd == "init")
		{
			if (!newPath.empty())
				current_path(newPath);
			init();
		}
		else
		{
			path oldPath = current_path();	//Save current path
			if (!newPath.empty())
				current_path(newPath);	//Switch to new path to read config

			if (readConfig())
				checkPath();
			else
			{
				init();
				checkPath();
			}
			if (!newPath.empty())	//Switch back to old path to get absolute path of local path
				current_path(oldPath);

			if (!newLocal.empty())
				localPath = newLocal;
			localPath = system_complete(localPath);
			if (!newData.empty())
				dataPath = newData;
			if (!newPath.empty())
				current_path(newPath);

			if (!readLang())
				loadDefaultLang();
			readSource();
			readLocal();
			if (!lock())
			{
				std::cout << msgData[MSGE_LOCK] << std::endl;
				throw(0);
			}
			locked = true;
			prCallbackP = reportProgress;

			argp++;
			if (cmd == "install")
			{
				if (argc - argp < 1)
				{
					printUsage();
					throw(0);
				}
				errInfo err = install(argv[argp]);
				if (err.err)
				{
					cout << err.info << endl;
					throw(0);
				}
			}
			else if (cmd == "remove")
			{
				if (argc - argp < 1)
				{
					printUsage();
					throw(0);
				}
				errInfo err = uninstall(argv[argp], false, (force ? REMOVE_RECURSIVE : REMOVE_NORMAL));
				if (err.err)
				{
					cout << err.info << endl;
					throw(0);
				}
			}
			else if (cmd == "upgrade")
			{
				if (argc - argp < 1)
				{
					errInfo err = upgrade();
					if (err.err)
					{
						cout << err.info << endl;
						throw(0);
					}
				}
				else
				{
					errInfo err = upgrade(argv[argp]);
					if (err.err)
					{
						cout << err.info << endl;
						throw(0);
					}
				}
			}
			else if (cmd == "dataclear")
			{
				if (argc - argp < 1)
				{
					printUsage();
					throw(0);
				}
				string name = std::string(argv[argp]);
				if (!is_installed(name))
				{
					cout << msgData[MSGE_PAK_NOT_INSTALLED] << endl;
					throw(0);
				}
				path scriptPath = dataPath / name / SCRIPT_PURGE;
				if (exists(scriptPath))
				{
					path currentPath = current_path();
					cout << msgData[MSGI_RUNS_PURGE] << endl;
					current_path(localPath);
					int ret = system(scriptPath.string().c_str());
					if (ret != 0)
						cout << msgData[MSGE_RUNS] << num2str(ret) << endl;
					else
						cout << msgData[MSGI_DONE] << endl;
					current_path(currentPath);
				}
			}
			else if (cmd == "info")
			{
				if (argc - argp < 1)
				{
					printUsage();
					throw(0);
				}
				string name = std::string(argv[argp]);
				if (is_installed(name))
					printInfoFromFile(name);
				else
				{
					package *pkg = find_package(name);
					if (pkg == NULL)
					{
						cout << msgData[MSGE_PAK_NOT_FOUND] << endl;
						throw(0);
					}
					printInfo(pkg);
				}
			}
			else if (cmd == "check")
			{
				if (argc - argp < 1)
				{
					printUsage();
					throw(0);
				}
				try
				{
					if (check(argv[argp]))
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
					throw(0);
				}
				pakListTp upgradeList;
				checkUpgrade(upgradeList);
				if (upgradeList.empty())
				{
					cout << msgData[MSGI_UPGRADE] << endl;
					std::for_each(upgradeList.begin(), upgradeList.end(), [](package *pak){
						cout << '\t' << pak->getName() << endl;
					});
				}
				else
				{
					cout << msgData[MSGI_NO_UPGRADE] << endl;
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
			else if (cmd == "available")
			{
				srcListTp::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
				for (; pSrc != pSrcEnd; pSrc++)
					printAvailable(*pSrc);
			}
			else if (cmd == "available-short")
			{
				srcListTp::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
				for (; pSrc != pSrcEnd; pSrc++)
					printAvailableShort(*pSrc);
			}
			else if (cmd == "listsrc")
			{
				vector<source*>::const_iterator p = sourceList.cbegin(), pEnd = sourceList.cend();
				for (; p != pEnd; p++)
					cout << (*p)->getAdd() << endl;
			}
			else if (cmd == "addsrc")
			{
				if (argc - argp < 1)
				{
					printUsage();
					throw(0);
				}
				string name(argv[argp]);
				vector<source*>::const_iterator p = sourceList.cbegin(), pEnd = sourceList.cend();
				for (; p != pEnd; p++)
				{
					if ((*p)->getAdd() == name)
					{
						cout << "E:Source already added" << endl;
						throw(0);
					}
				}
				source *newSrc = new source(name);
				newSrc->loadRemote();
				sourceList.push_back(newSrc);
				writeSource();

				pakListTp upgradeList;
				newSrc->checkUpgrade(upgradeList);
				if (upgradeList.empty())
				{
					cout << msgData[MSGI_UPGRADE] << endl;
					std::for_each(upgradeList.begin(), upgradeList.end(), [](package *pak){
						cout << '\t' << pak->getName() << endl;
					});
				}
				else
				{
					cout << msgData[MSGI_NO_UPGRADE] << endl;
				}
			}
			else if (cmd == "delsrc")
			{
				if (argc - argp < 1)
				{
					printUsage();
					throw(0);
				}
				string name(argv[argp]);
				vector<source*>::iterator p = sourceList.begin(), pEnd = sourceList.end();
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
					cout << msgData[MSGI_DELETED] << endl;
				else
					cout << "E:Source not found" << endl;
			}
			else
			{
				printUsage();
			}
			writeLocal();
		}
	}
	catch (boost::filesystem::filesystem_error ex)
	{
		cout << "E:" << ex.what() << endl;

	}
	catch (exception ex)
	{
		cout << "E:" << ex.what() << endl;
	}
	catch (...)
	{
		
	}
	
	if (locked)
		unlock();
	return 0;
}

#endif
