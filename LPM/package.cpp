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
	depListTp::const_iterator pDep, pDepEnd;
	pDep = confList.begin();
	pDepEnd = confList.end();
	std::list<std::string> confNList;
	infoStream << "I:Checking requirement..." << std::endl;
	for (; pDep != pDepEnd; pDep++)
		if (is_installed(*pDep))
			confNList.push_back(*pDep);
	if (!confNList.empty())
	{
		infoStream << "W:Conflict Package" << std::endl;
		while (!confNList.empty())
		{
			infoStream << "\t" << confNList.front() << std::endl;
			confNList.pop_front();
		}
		return errInfo("E:Confliction found");
	}
	pDep = depList.begin();
	pDepEnd = depList.end();
	for (; pDep != pDepEnd; pDep++)
	{
		if (!is_installed(*pDep))
		{
			infoStream << "I:Installing Dependent package" << *pDep << std::endl;
			errInfo err = install(*pDep);
			if (err.err)
				return err;
		}
	}

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
	bool flag1 = false, flag2 = false;
	try
	{
		fs::create_directory(tmpPath);
		flag1 = true;
		fs::create_directory(pakPath);
		flag2 = true;
		infoStream << "I:Decompressing package" << *pDep << std::endl;
		errInfo err = unzip(buf.begin(), buf.end(), tmpPath);
		if (err.err)
			return err;
		infoStream << "I:Decompressed" << *pDep << std::endl;

		if (fs::exists(tmpPath / DIRNAME_INFO))
		{
			for (fs::directory_iterator p(tmpPath / DIRNAME_INFO), pEnd; p != pEnd; p++)
			{
				if (fs::is_regular_file(p->path()))
					fs::copy_file(p->path(), pakPath);
				else
					throw("E:Illegal package:dir in $info");
			}
			fs::remove_all(tmpPath / DIRNAME_INFO);
		}

		std::ofstream infOut((pakPath / FILENAME_INST).string());
		infoStream << "I:Copying files" << *pDep << std::endl;
		install_copy(tmpPath, fs::path(), infOut);
		infoStream << "I:File copied" << *pDep << std::endl;
		infOut.close();

		fs::remove_all(tmpPath);

		infOut.open((pakPath / FILENAME_INFO).string());
		infOut << extInfo.fname << std::endl;
		infOut << name << std::endl;
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

		pDep = depList.begin();
		pDepEnd = depList.end();
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
		if (upgrade)
			fs::copy_file(dataPath / DIRNAME_UPGRADE / (name + ".inf"), pakPath / FILENAME_BEDEP);

		fs::path scriptPath = pakPath / SCRIPT_INST;
		if (exists(scriptPath))
		{
			infoStream << "I:Running installation script" << std::endl;
			system(scriptPath.string().c_str());
			infoStream << "I:Done" << std::endl;
		}
		if (!upgrade)
		{
			scriptPath = pakPath / SCRIPT_INIT;
			if (exists(scriptPath))
			{
				infoStream << "I:Running initialization script" << std::endl;
				system(scriptPath.string().c_str());
				infoStream << "I:Done" << std::endl;
			}
		}
	}
	catch (fs::filesystem_error err)
	{
		if (flag1)
			fs::remove_all(tmpPath);
		if (flag2)
			fs::remove_all(pakPath);
		return errInfo(std::string("E:filesystem:") + err.what());
	}
	catch (const char* err)
	{
		if (flag1)
			fs::remove_all(tmpPath);
		if (flag2)
			fs::remove_all(pakPath);
		return errInfo(std::string(err));
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
		return errInfo("E:Not installed");

	std::string line;
	std::ifstream infoIn((dataPath / name / FILENAME_INFO).string());
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	std::getline(infoIn, line);
	infoIn.close();
	version oldver(line);
	if (oldver <= getVer())
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
	pDep = depList.begin();
	pDepEnd = depList.end();
	for (; pDep != pDepEnd; pDep++)
		if (!is_installed(*pDep))
			return false;
	return true;
}

bool is_installed(std::string name)
{
	return fs::exists(dataPath / name);
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
		return errInfo(std::string("E:Already installed"));

	infoStream << "I:Searching package..." << std::endl;
	package *pak = find_package(name);
	if (pak == NULL)
		return errInfo(std::string("E:Package not found"));
	infoStream << "I:Package found:" << std::endl;
	
	return pak->inst();
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
		depIn.close();
	}

	fs::path scriptPath;
	if (!upgrade)
	{
		scriptPath = pakPath / SCRIPT_PURGE;
		if (exists(scriptPath))
		{
			infoStream << "I:Running purge script" << std::endl;
			system(scriptPath.string().c_str());
		}
	}
	scriptPath = pakPath / SCRIPT_REMOVE;
	if (exists(scriptPath))
	{
		infoStream << "I:Running removal script" << std::endl;
		system(scriptPath.string().c_str());
	}

	fs::path logPath = pakPath / "install.ini";
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
