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
	fs::path nativePath = dataPath / "$native";
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
	depListTp::const_iterator pDep, pDepEnd;

	pDep = confList.begin();
	pDepEnd = confList.end();
	for (; pDep != pDepEnd; pDep++)
		if (is_installed(*pDep))
			return errInfo(std::string("E:Confliction between package ") + name + " and " + *pDep);
	pDep = depList.begin();
	pDepEnd = depList.end();
	for (; pDep != pDepEnd; pDep++)
		if (!is_installed(*pDep))
		{
			infoStream << "I:Installing Dependent package" << *pDep << std::endl;
			errInfo err = install(*pDep);
			if (err.err)
				return err;
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

	fs::path tmpPath = dataPath / "$temp" / name, pakPath = dataPath / name;
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

		if (fs::exists(tmpPath / "$info"))
		{
			for (fs::directory_iterator p(tmpPath / "$info"), pEnd; p != pEnd; p++)
			{
				if (fs::is_regular_file(p->path()))
					fs::copy_file(p->path(), pakPath);
				else
					return errInfo(std::string("E:Illegal package:dir in $info"));
			}
			fs::remove_all(tmpPath / "$info");
		}

		std::ofstream iniOut((pakPath / "install.ini").string());
		infoStream << "I:Copying files" << *pDep << std::endl;
		install_copy(tmpPath, fs::path(), iniOut);
		infoStream << "I:File copied" << *pDep << std::endl;
		iniOut.close();

		fs::remove_all(tmpPath);

		fs::path scriptPath =
#ifdef WIN32
			pakPath / "install.bat"
#endif
#ifdef __linux__
			pakPath / "install.sh"
#endif
			;
		if (exists(scriptPath))
		{
			infoStream << "I:Running installation script" << std::endl;
			system(scriptPath.string().c_str());
			infoStream << "I:Done" << std::endl;
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

bool is_installed(std::string name)
{
	return fs::exists(dataPath / name);
}

errInfo install(std::string name)
{
	if (is_installed(name))
		return errInfo(std::string("E:Already installed"));

	std::vector<source*>::const_iterator p, pEnd = sourceList.cend();
	package *pak = NULL;
	for (p = sourceList.cbegin(); p != pEnd; p++)
	{
		pak = (*p)->find_package(name);
		if (pak != NULL)
			break;
	}
	if (pak == NULL)
		return errInfo(std::string("E:Package not found"));

	return pak->inst();
}

errInfo uninstall(std::string name)
{
	if (!is_installed(name))
		return errInfo(std::string("E:Not installed"));
	fs::path pakPath = dataPath / name;
	fs::path logPath = pakPath / "install.ini";
	fs::path scriptPath =
#ifdef WIN32
		pakPath / "uninstall.bat"
#endif
#ifdef __linux__
		pakPath / "uninstall.sh"
#endif
		;
	if (exists(scriptPath))
	{
		infoStream << "I:Running removal script" << std::endl;
		system(scriptPath.string().c_str());
	}
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
