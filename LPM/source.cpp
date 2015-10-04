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

					std::string name = std::move(data[LINE_DEP]);
					if (!name.empty())
					{
						if (name.back() != ';')
							name.push_back(';');
						size_t pos1 = 0, pos2 = name.find(';');
						while (pos2 != std::string::npos)
						{
							depList.push_back('&' + name.substr(pos1, pos2 - pos1));
							pos1 = pos2 + 1;
							pos2 = name.find(';', pos1);
						}
					}

					name = std::move(data[LINE_CONF]);
					if (!name.empty())
					{
						if (name.back() != ';')
							name.push_back(';');
						size_t pos1 = 0, pos2 = name.find(';');
						while (pos2 != std::string::npos)
						{
							confList.push_back('!' + name.substr(pos1, pos2 - pos1));
							pos1 = pos2 + 1;
							pos2 = name.find(';', pos1);
						}
					}

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

void source::upgradeAll()
{
	pakListTp::const_iterator p = pakList.cbegin(), pEnd = pakList.cend();
	for (; p != pEnd; p++)
	{
		if (is_installed((*p)->getName()) && (*p)->needUpgrade())
		{
			errInfo err = (*p)->upgrade();
			if (err.err)
				infoStream << err.info << std::endl;
		}
	}
}
