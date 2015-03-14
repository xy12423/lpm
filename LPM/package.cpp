#include "stdafx.h"
#include "package.h"
#include "source.h"
#include "unzip.h"

namespace fs = boost::filesystem;

size_t write_data_pkg(void *buffer, size_t size, size_t nmemb, void *userp)
{
	dataBuf *myBuf = static_cast<dataBuf*>(userp);
	char *dataBuf = static_cast<char*>(buffer);
	size_t sizeAll = size * nmemb;
	for (size_t i = 0; i < sizeAll; i++)
	{
		myBuf->push_back(*dataBuf);
		dataBuf++;
	}
	return sizeAll;
}

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
					throw("E:Multi overwriting");
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

errInfo package::inst(bool upgrade)
{
	CURL *handle = curl_easy_init();
	char *addCStr = str2cstr(source + "/" + name + ".lpm");
	dataBuf buf;
	char *errBuf = new char[2048];
	curl_easy_setopt(handle, CURLOPT_URL, addCStr);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_pkg);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buf);
	curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errBuf);
	infoStream << "I:Connecting" << std::endl;
	CURLcode success = curl_easy_perform(handle);
	if (success != CURLcode::CURLE_OK)
	{
		return errInfo(std::string("E:network:") + errBuf);
	}
	curl_easy_cleanup(handle);
	infoStream << "I:Data downloaded" << std::endl;

	fs::path tmpPath = dataPath / DIRNAME_TEMP / name, pakPath = dataPath / name;
	bool flag1 = false, flag2 = false, flag3 = false;
	try
	{
		fs::create_directory(tmpPath);
		flag1 = true;
		fs::create_directory(pakPath);
		flag2 = true;
		infoStream << "I:Decompressing package" << std::endl;
		errInfo err = unzip(buf.begin(), buf.end(), tmpPath);
		if (err.err)
			return err;
		infoStream << "I:Decompressed" << std::endl;

		if (fs::exists(tmpPath / DIRNAME_INFO))
		{
			for (fs::directory_iterator p(tmpPath / DIRNAME_INFO), pEnd; p != pEnd; p++)
			{
				if (fs::is_regular_file(p->path()))
					fs::copy_file(p->path(), pakPath / p->path().filename());
				else
					throw("E:Illegal package:dir in $info");
			}
			fs::remove_all(tmpPath / DIRNAME_INFO);
		}

		std::ofstream infOut((pakPath / FILENAME_INST).string());
		infoStream << "I:Copying files" << std::endl;
		install_copy(tmpPath, fs::path(), infOut);
		infoStream << "I:File copied" << std::endl;
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

		fs::path scriptPath = pakPath / SCRIPT_INST, currentPath = fs::current_path();
		if (exists(scriptPath))
		{
			infoStream << "I:Running installation script" << std::endl;
			fs::current_path(localPath);
			int ret = system(scriptPath.string().c_str());
			if (ret != 0)
				throw(std::string("E:Installation script exited with code") + num2str(ret));
			infoStream << "I:Done" << std::endl;
		}
		if (!upgrade)
		{
			scriptPath = pakPath / SCRIPT_INIT;
			if (exists(scriptPath))
			{
				infoStream << "I:Running initialization script" << std::endl;
				fs::current_path(localPath);
				int ret = system(scriptPath.string().c_str());
				if (ret != 0)
					throw(std::string("E:Initialization script exited with code") + num2str(ret));
				infoStream << "I:Done" << std::endl;
			}
		}
		fs::current_path(currentPath);
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
		return errInfo(std::string("E:filesystem:") + err.what());
	}
	catch (const char* err)
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
		return errInfo(std::string(err));
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
	catch (...)
	{
		throw;
	}
	infoStream << "I:Package installed" << std::endl;
	return errInfo();
}

errInfo package::upgrade()
{
	if (!is_installed(name))
		return errInfo("E:Package not installed");

	std::string line;
	std::ifstream infoIn((dataPath / name / FILENAME_INFO).string());
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	infoIn.close();
	version oldver(line);
	if (oldver >= ver)
		return errInfo("W:Needn't upgrade");

	infoStream << "I:Removing..." << std::endl;
	errInfo err = uninstall(name, true);
	if (err.err)
		return err;
	infoStream << "I:Reinstalling..." << std::endl;
	err = inst(true);
	if (err.err)
		return err;
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
	std::vector<source*>::const_iterator p, pEnd = sourceList.cend();
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
		return errInfo(std::string("E:Package already installed"));

	infoStream << "I:Searching package..." << std::endl;
	package *pak = find_package(name);
	if (pak == NULL)
		return errInfo(std::string("E:Package not found"));
	infoStream << "I:Package found:" << name << std::endl;

	depListTp::const_iterator pDep, pDepEnd;
	pDep = pak->confList.begin();
	pDepEnd = pak->confList.end();
	depListTp pakList;
	infoStream << "I:Checking requirement..." << std::endl;
	for (; pDep != pDepEnd; pDep++)
		if (is_installed(*pDep))
			pakList.push_back(*pDep);
	if (!pakList.empty())
	{
		infoStream << "W:Conflict Package" << std::endl;
		while (!pakList.empty())
		{
			infoStream << "\t" << pakList.front() << std::endl;
			pakList.pop_front();
		}
		return errInfo("E:Confliction found");
	}

	std::list<package *> depList;
	depListTp pakQue;
	depMapTp pakHash;
	depList.push_front(pak);
	pakHash.emplace(name);

	pDep = pak->depList.begin();
	pDepEnd = pak->depList.end();
	for (; pDep != pDepEnd; pDep++)
	{
		if (!is_installed(*pDep))
		{
			package *depPak = find_package(*pDep);
			if (depPak == NULL)
				return errInfo(std::string("E:Package not found:") + *pDep);
			depList.push_front(depPak);
			pakQue.push_back(*pDep);
			pakHash.emplace(*pDep);
		}
	}

	while (!pakQue.empty())
	{
		package *depPak = find_package(pakQue.front());
		pakQue.pop_front();
		if (depPak == NULL)
			return errInfo(std::string("E:Package not found:") + pakQue.front());
		pDep = depPak->depList.begin();
		pDepEnd = depPak->depList.end();
		for (; pDep != pDepEnd; pDep++)
		{
			if (is_installed(*pDep) == false && pakHash.find(*pDep) != pakHash.end())
			{
				depList.push_front(depPak);
				pakQue.push_back(*pDep);
				pakHash.emplace(*pDep);
			}
		}
	}

	std::list<package *>::iterator depItr, depEnd;
	infoStream << "I:Will install these packages:" << std::endl;
	depItr = depList.begin();
	depEnd = depList.end();
	for (; depItr != depEnd; depItr++)
		infoStream << "\t" << (*depItr)->name << std::endl;
	depItr = depList.begin();
	for (; depItr != depEnd; depItr++)
	{
		infoStream << "I:Installing package " << (*depItr)->name << std::endl;
		errInfo err = (*depItr)->inst();
		if (err.err)
			return err;
	}

	return errInfo();
}

errInfo uninstall(std::string name, bool upgrade)
{
	if (!is_installed(name))
		return errInfo(std::string("E:Not installed"));
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
			infoStream << "W:This package is depended by:" << std::endl;
			depIn.open((pakPath / FILENAME_BEDEP).string());
			while (!depIn.eof())
			{
				std::getline(depIn, line);
				infoStream << "\t" << line << std::endl;
			}
			depIn.close();
			return errInfo("E:This package is depended by other package");
		}
	}

	{
		if (!fs::exists(pakPath / FILENAME_DEP))
			return errInfo("E:Dependence info not found");
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
			infoStream << "I:Running purge script" << std::endl;
			fs::current_path(localPath);
			int ret = system(scriptPath.string().c_str());
			if (ret != 0)
				throw(std::string("E:Purge script exited with code") + num2str(ret));
			infoStream << "I:Done" << std::endl;
		}
	}
	scriptPath = pakPath / SCRIPT_REMOVE;
	if (exists(scriptPath))
	{
		infoStream << "I:Running removal script" << std::endl;
		fs::current_path(localPath);
		int ret = system(scriptPath.string().c_str());
		if (ret != 0)
			throw(std::string("E:Removal script exited with code") + num2str(ret));
		infoStream << "I:Done" << std::endl;
	}
	fs::current_path(currentPath);

	fs::path logPath = pakPath / FILENAME_INST;
	std::ifstream logIn(logPath.string());
	std::string tmpPath;

	try
	{
		infoStream << "I:Deleting files" << std::endl;
		while (!logIn.eof())
		{
			std::getline(logIn, tmpPath);
			if (!tmpPath.empty())
				fs::remove(tmpPath);
		}
		logIn.close();
		fs::remove_all(pakPath);
	}
	catch (fs::filesystem_error err)
	{
		return errInfo(std::string("E:filesystem:") + err.what());
	}
	catch (const char* err)
	{
		return errInfo(std::string(err));
	}
	catch (...)
	{
		throw;
	}
	infoStream << "I:Package removed" << std::endl;
	return errInfo();
}
