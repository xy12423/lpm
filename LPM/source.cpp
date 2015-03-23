#include "stdafx.h"
#include "package.h"
#include "source.h"
#include "download.h"

srcListTp sourceList;

const int LINE_FNAME = 0;
const int LINE_NAME = 1;
const int LINE_VER = 2;
const int LINE_AUTHOR = 3;
const int LINE_INFO = 4;
const int LINE_DEP = 5;
const int LINE_CONF = 6;
const int LINE_END = 7;

void source::loadLocal(pakListTp &_pkgList)
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
	dataBuf buf;
	errInfo err = download(add + "/_index", &buf);
	if (err.err)
		return err;

	pakListTp newPkgList;

	dataBuf::const_iterator p, pEnd = buf.cend();
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

					for (int i = 0; i < LINE_END; i++)
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
		return errInfo("E:Incorrect pack info from source");
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
	pakListTp::const_iterator p = pkgList.cbegin(), pEnd = pkgList.cend();
	for (; p != pEnd; p++)
	{
		if (is_installed((*p)->getName()) && (*p)->needUpgrade())
		{
			errInfo err = (*p)->upgrade(true);
			if (err.err)
				return err;
		}
	}
	return errInfo();
}
