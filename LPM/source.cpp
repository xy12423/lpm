#include "stdafx.h"
#include "package.h"
#include "source.h"

std::vector<source*> sourceList;

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
	std::string data[6];
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
				if (state == 6)
				{
					version ver;
					pakExtInfo extInfo;
					depListTp depList, confList;

					p2 = data[1].cbegin();
					pEnd2 = data[1].cend();
					for (; p2 != pEnd2; p2++)
					{
						if (*p2 == '.')
						{
							p2++;
							break;
						}
						if (!isdigit(*p2))
							return errInfo(std::string("E:") + "Invalid version on pack " + data[0]);
						ver.major = ver.major * 10 + static_cast<UINT>((*p2) - '0');
					}
					for (; p2 != pEnd2; p2++)
					{
						if (*p2 == '.')
						{
							p2++;
							break;
						}
						if (!isdigit(*p2))
							return errInfo(std::string("E:") + "Invalid version on pack " + data[0]);
						ver.minor = ver.minor * 10 + static_cast<UINT>((*p2) - '0');
					}
					for (; p2 != pEnd2; p2++)
					{
						if (!isdigit(*p2))
							return errInfo(std::string("E:") + "Invalid version on pack " + data[0]);
						ver.revision = ver.revision * 10 + static_cast<UINT>((*p2) - '0');
					}

					std::string name;
					for (p2 = data[4].cbegin(), pEnd2 = data[4].cend(); p2 != pEnd2; p2++)
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

					for (p2 = data[5].cbegin(), pEnd2 = data[5].cend(); p2 != pEnd2; p2++)
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

					newPkgList.push_back(new package(add, data[0], ver, depList, confList, pakExtInfo(data[2], data[3])));

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

	infoStream << "I:Package List of source" << add << "refreshed" << std::endl;
	return errInfo();
}

package* source::find_package(std::string name)
{
	std::unordered_map<std::string, int>::iterator p = pkgMap.find(name);
	if (p == pkgMap.end())
		return NULL;
	return pkgList[p->second];
}
