#include "stdafx.h"

#ifndef _LPM_GUI

#include "main.h"

using namespace boost::filesystem;
using namespace std;

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
	std::vector<package*>::const_iterator p = src->pkgList.cbegin(), pEnd = src->pkgList.cend();
	for (; p != pEnd; p++)
		printInfo(*p);
}
void printAvailableShort(source *src, bool ignoreInstalled = true)
{
	if (src == NULL)
		return;
	std::vector<package*>::const_iterator p = src->pkgList.cbegin(), pEnd = src->pkgList.cend();
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

	try
	{
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
			else if (cmd == "upgrade")
			{
				if (argc < 3)
				{
					errInfo err = upgrade();
					if (err.err)
					{
						cout << err.info << endl;
						return 0;
					}
				}
				else
				{
					errInfo err = upgrade(argv[2]);
					if (err.err)
					{
						cout << err.info << endl;
						return 0;
					}
				}
			}
			else if (cmd == "dataclear")
			{
				if (argc < 3)
				{
					printUsage();
					return 0;
				}
				string name = std::string(argv[2]);
				if (!is_installed(name))
				{
					cout << "E:Package not installed" << endl;
					return 0;
				}
				path scriptPath = dataPath / name / SCRIPT_PURGE;
				if (exists(scriptPath))
				{
					path currentPath = current_path();
					infoStream << "I:Running purge script" << std::endl;
					current_path(localPath);
					int ret = system(scriptPath.string().c_str());
					if (ret != 0)
						cout << "E:Installation script exited with code" << num2str(ret) << endl;
					else
						infoStream << "I:Done" << std::endl;
					current_path(currentPath);
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
			else if (cmd == "available")
			{
				std::vector<source*>::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
				for (; pSrc != pSrcEnd; pSrc++)
					printAvailable(*pSrc);
			}
			else if (cmd == "available-short")
			{
				std::vector<source*>::const_iterator pSrc = sourceList.begin(), pSrcEnd = sourceList.end();
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
				if (argc < 3)
				{
					printUsage();
					return 0;
				}
				string name(argv[2]);
				vector<source*>::const_iterator p = sourceList.cbegin(), pEnd = sourceList.cend();
				for (; p != pEnd; p++)
				{
					if ((*p)->getAdd() == name)
					{
						cout << "E:Source already added" << endl;
						return 0;
					}
				}
				source *newSrc = new source(name);
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
					cout << "I:Deleted" << endl;
				else
					cout << "E:Source not found" << endl;
			}
			else
			{
				cout << "E:Command incorrect" << endl;
			}
		}
	}
	catch (exception ex)
	{
		cout << "E:" << ex.what() << endl;
	}

	return 0;
}

#endif
