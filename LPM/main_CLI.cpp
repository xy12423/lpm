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

#ifndef _LPM_GUI

#include "main.h"

using namespace boost::filesystem;
using namespace std;

int lastProgress = -1;
size_t lastSizeDownloaded = 0;
clock_t lastClock;
const unsigned int B_rate = 1024;
void reportProgress(double progress, size_t size_downloaded)
{
	if (static_cast<int>(progress) != lastProgress)
	{
		double deltaClock = 0;
		if (progress != 0)
			deltaClock = static_cast<double>(clock() - lastClock) / CLOCKS_PER_SEC;
		std::cout << static_cast<int>(progress) << "% ";
		if (progress != 0)
		{
			if (deltaClock != 0)
			{
				double speed = (size_downloaded - lastSizeDownloaded) / deltaClock / B_rate;
				if (speed > B_rate)
				{
					int speedByMB = static_cast<int>(speed / B_rate * 10);
					std::cout << speedByMB / 10 << '.' << speedByMB % 10 << "MB/s";
				}
				else
				{
					int speedByKB = static_cast<int>(speed * 10);
					std::cout << speedByKB / 10 << '.' << speedByKB % 10 << "kB/s";
				}
				lastClock = clock();
			}
		}
		std::cout << "          \r";
		lastSizeDownloaded = size_downloaded;
		lastProgress = static_cast<int>(progress);
	}
	return;
}

void printInfo(package *pkg)
{
	if (pkg == NULL)
		return;
	std::cout << "Name:" << pkg->extInfo.getFName_Local() << std::endl;
	std::cout << "Package:" << pkg->name << std::endl;
	std::string desc = pkg->extInfo.getInfo_Local();
	processEscChar(desc);
	std::cout << "Description:\n" << desc << std::endl;
	std::cout << "Author:" << pkg->extInfo.getAuthor_Local() << std::endl;
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
	std::cout << "Name:" << wxConvLocal.cWC2MB(wxConvUTF8.cMB2WC(line.c_str())) << std::endl;
	std::cout << "Package:" << name << std::endl;
	std::getline(infoIn, line);
	processEscChar(line);
	std::cout << "Description:\n" << wxConvLocal.cWC2MB(wxConvUTF8.cMB2WC(line.c_str())) << std::endl;
	std::getline(infoIn, line);
	std::cout << "Author:" << wxConvLocal.cWC2MB(wxConvUTF8.cMB2WC(line.c_str())) << std::endl;
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
	cout << "Usage:\n\n";
	cout << "    lpm [--lpmdir=TARGET_LPM] [--local=TARGET_LOCAL] [--force] COMMAND\n";
	cout << "      \n";
	cout << "      Options:\n";
	cout << "        --lpmdir    Specific a target LPM deploy directory\n";
	cout << "        --local     Specific where package should unpack to.\n";
	cout << "        --force     Force continue, will remove all packages that noticed.\n";
	cout << "                     Usually required when removing with dependency,\n";
	cout << "                     installing or update with removing conflicted packages.\n\n";
	cout << "        COMMAND     Specific things that LPM should to do.\n";
	cout << "      \n";
	cout << "      Commands:\n";
	cout << "        init                            Create inital configuration files.\n";
	cout << "        update                          Check for updates from source.\n";
	cout << "        install <package name>          Install a specific package.\n";
	cout << "        remove <package name>           Remove a specific package.\n";
	cout << "        reconf <package name>        Run initalize process of a package.\n";
	cout << "                                         Will clear package configuration\n";
	cout << "                                         and reset package settings.\n\n";
	cout << "        upgrade [package name]          Upgrade a specific package or all\n";
	cout << "                                         out-of-date packages.\n\n";
	cout << "        info <package name>             Display information of a package.\n";
	cout << "        check <package name>            Check dependency of a specific\n";
	cout << "                                         package.\n\n";
	cout << "        list                            List all packages that installed.\n";
	cout << "        list-short                      List installed packages only in names.\n";
	cout << "        available                       Show available packages in source.\n";
	cout << "        available-short                 Show available packages only in names.\n";
	cout << "        listsrc                         List all sources in use.\n";
	cout << "        addsrc <source address>         Add a package installing source.\n";
	cout << "        delsrc <source address>         Remove a package installing source.\n";
	cout << "        release                         Try to release the management lock\n";
	cout << "                                         of LPM. (Usually use in abnormal\n";
	cout << "                                         situation, such as a crash.)\n\n\n";
	cout << "Live Package Manager v1.2\n";
	cout << "    Made by xy12423. Licensed in GPLv3,\n";
	cout << "    Copyleft xy12423 & Little Busters! Live Project.\n";
	cout << "    Source code available at https://github.com/xy12423/lpm" << endl;
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
		else if (cmd == "release")
		{
			locked = true;
		}
		else
		{
			if (!newLocal.empty())
				newLocal = fs::system_complete(newLocal).string();	//Get absolute path of new local path(if has)
			if (!newPath.empty())
				fs::current_path(newPath);	//Switch to new path to read config

			if (readConfig())
			{
				localPath = fs::system_complete(localPath);
				checkPath();
			}
			else
			{
				init();
				localPath = fs::system_complete(localPath);
				checkPath();
			}

			if (!newLocal.empty())
				localPath = newLocal;
			if (!newData.empty())
				dataPath = newData;

			if (!readLang())
				loadDefaultLang();

			if (!newPath.empty())
				cout << msgData[MSGI_USING_LPMDIR] << newPath << endl;
			if (!newLocal.empty())
				cout << msgData[MSGI_USING_LOCAL] << newLocal << endl;

			readSource();
			readLocal();
			if (!lock())
			{
				cout << msgData[MSGE_LOCK] << endl;
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
				errInfo err = install(argv[argp], force);
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
					errInfo err = upgrade(argv[argp], force);
					if (err.err)
					{
						cout << err.info << endl;
						throw(0);
					}
				}
			}
			else if (cmd == "reconf")
			{
				if (argc - argp < 1)
				{
					printUsage();
					throw(0);
				}
				string name(argv[argp]);
				if (!is_installed(name))
				{
					cout << msgData[MSGE_PAK_NOT_INSTALLED] << endl;
					throw(0);
				}

				path pakPath = dataPath / name,
					scriptPath = pakPath / SCRIPT_INST,
					logPath = pakPath / FILENAME_INST;
				ifstream infIn(logPath.string());
				string instPathStr;
				getline(infIn, instPathStr);
				infIn.close();
				path instPath(instPathStr);

				if (exists(scriptPath))
				{
					cout << msgData[MSGI_RUNS_PURGE] << endl;
					int ret = run_script(scriptPath.string(), instPath);
					if (ret != 0)
						cout << msgData[MSGE_RUNS] << to_string(ret) << endl;
					else
					{
						cout << msgData[MSGI_DONE] << endl;
						scriptPath = pakPath / SCRIPT_INIT;
						if (exists(scriptPath))
						{
							cout << msgData[MSGI_RUNS_INIT] << endl;
							ret = run_script(scriptPath.string(), instPath);
							if (ret != 0)
								cout << msgData[MSGE_RUNS] << to_string(ret) << endl;
							else
								cout << msgData[MSGI_DONE] << endl;
						}
					}
				}
			}
			else if (cmd == "info")
			{
				if (argc - argp < 1)
				{
					printUsage();
					throw(0);
				}
				string name(argv[argp]);
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
					cout << msgData[MSGI_NO_UPGRADE] << endl;
				}
				else
				{
					cout << msgData[MSGI_UPGRADE] << endl;
					std::for_each(upgradeList.begin(), upgradeList.end(), [](package *pak){
						cout << '\t' << pak->getName() << endl;
					});
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
					cout << msgData[MSGI_NO_UPGRADE] << endl;
				}
				else
				{
					cout << msgData[MSGI_UPGRADE] << endl;
					std::for_each(upgradeList.begin(), upgradeList.end(), [](package *pak){
						cout << '\t' << pak->getName() << endl;
					});
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

	writeLocal();
	if (locked)
		unlock();
	return 0;
}

#endif
