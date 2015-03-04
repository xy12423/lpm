#include "stdafx.h"
#include "package.h"
#include "source.h"

std::vector<source*> sourceList;

const int LINE_FNAME = 0;
const int LINE_NAME = 1;
const int LINE_VER = 2;
const int LINE_AUTHOR = 3;
const int LINE_INFO = 4;
const int LINE_DEP = 5;
const int LINE_CONF = 6;
const int LINE_END = 7;

size_t write_data_src(void *buffer, size_t size, size_t nmemb, void *userp)
{
	std::string *myBuf = static_cast<std::string*>(userp);
	char *dataBuf = static_cast<char*>(buffer);
	size_t sizeAll = size * nmemb;
	for (size_t i = 0; i < sizeAll; i++)
	{
		myBuf->push_back(*dataBuf);
		dataBuf++;
	}
	return sizeAll;
}

void source::loadLocal(std::vector<package*> &_pkgList)
{
	pkgMap.clear(); 
	std::for_each(pkgList.begin(), pkgList.end(), [this](package* arg){
		delete arg;
	});
	pkgList.clear(); 
	std::for_each(_pkgList.begin(), _pkgList.end(), [this](package* arg){ 
		pkgMap.emplace(arg->getName(), pkgList.size()); 
		pkgList.push_back(arg); 
	});
}

errInfo source::loadRemote()
{
	CURL *handle = curl_easy_init();
	char *addCStr = str2cstr(add + "/_index");
	std::string buf;
	char *errBuf = new char[2048];
	curl_easy_setopt(handle, CURLOPT_URL, addCStr);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_src);
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

	std::vector<package*> newPkgList;

	std::string::const_iterator p, pEnd = buf.cend();
	std::string::const_iterator p2, pEnd2;
	std::string data[LINE_END];
	int state = 0;
	for (p = buf.cbegin(); p != pEnd;)
	{
		switch (*p)
		{
			case '\r':
				p++;
			case '\n':
			{
				p++;
				
				state++;
				if (state == LINE_END)
				{
					version ver(data[LINE_VER]);
					pakExtInfo extInfo;
					depListTp depList, confList;

					std::string name;
					for (p2 = data[LINE_DEP].cbegin(), pEnd2 = data[LINE_DEP].cend(); p2 != pEnd2; p2++)
					{
						if (*p2 == ';')
						{
							depList.push_back(name);
							name.clear();
						}
						else
							name.push_back(*p2);
					}
					if (!name.empty())
						depList.push_back(name);
					name.clear();

					for (p2 = data[LINE_CONF].cbegin(), pEnd2 = data[LINE_CONF].cend(); p2 != pEnd2; p2++)
					{
						if (*p2 == ';')
						{
							confList.push_back(name);
							name.clear();
						}
						else
							name.push_back(*p2);
					}
					if (!name.empty())
						confList.push_back(name);
					name.clear();

					newPkgList.push_back(new package(add, data[LINE_NAME], ver, depList, confList, pakExtInfo(data[LINE_FNAME], data[LINE_AUTHOR], data[LINE_INFO])));

					for (int i = 0; i < 6; i++)
						data[i].clear();
					state = 0;
				}
				break;
			}
			default:
				data[state].push_back(*p);
				p++;
		}
	}

	if (state != 0)
		return errInfo(std::string("E:") + "Incorrect pack info from source");
	loadLocal(newPkgList);

	infoStream << "I:Package List of source " << add << " refreshed" << std::endl;
	return errInfo();
}

package* source::find_package(std::string name)
{
	std::unordered_map<std::string, int>::iterator p = pkgMap.find(name);
	if (p == pkgMap.end())
		return NULL;
	return pkgList[p->second];
}

errInfo source::upgradeAll()
{
	std::vector<package*>::const_iterator p = pkgList.cbegin(), pEnd = pkgList.cend();
	for (; p != pEnd; p++)
	{
		if (is_installed((*p)->getName()))
		{
			errInfo err = (*p)->upgrade();
			if (err.err && err.info.front() != 'W')
				return err;
		}
	}
	return errInfo();
}
