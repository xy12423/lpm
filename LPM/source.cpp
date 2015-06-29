#include "stdafx.h"
#include "package.h"
#include "source.h"
#include "download.h"

srcListTp sourceList;

enum src_lineN{
	LINE_FNAME,
	LINE_NAME,
	LINE_VER,
	LINE_AUTHOR,
	LINE_INFO,
	LINE_DEP,
	LINE_CONF,
	LINE_END
};

void source::loadLocal(pakListTp &_pakList)
{
	pakMap.clear(); 
	std::for_each(pakList.begin(), pakList.end(), [this](package* arg){
		delete arg;
	});
	pakList.clear();
	std::for_each(_pakList.begin(), _pakList.end(), [this](package* arg){
		pakMap.emplace(arg->getName(), pakList.size());
		pakList.push_back(arg);
	});
}

errInfo source::loadRemote()
{
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
		return errInfo(msgData[MSGE_SRCINFO]);
	loadLocal(newPkgList);

	infoStream << msgData[MSGI_SRCINFO_REFED] << ':' << add << std::endl;
	return errInfo();
}

package* source::find_package(std::string name)
{
	std::unordered_map<std::string, int>::iterator p = pakMap.find(name);
	if (p == pakMap.end())
		return NULL;
	return pakList[p->second];
}

void source::checkUpgrade(pakListTp &ret)
{
	pakListTp::const_iterator p = pakList.cbegin(), pEnd = pakList.cend();
	for (; p != pEnd; p++)
		if (is_installed((*p)->getName()) && (*p)->needUpgrade())
			ret.push_back(*p);
}

errInfo source::upgradeAll()
{
	pakListTp::const_iterator p = pakList.cbegin(), pEnd = pakList.cend();
	for (; p != pEnd; p++)
	{
		if (is_installed((*p)->getName()) && (*p)->needUpgrade())
		{
			errInfo err = (*p)->upgrade();
			if (err.err)
				return err;
		}
	}
	return errInfo();
}
