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
#include "package_core.h"

typedef std::list<int> pakQListTp;
typedef std::unordered_set<int> pakIDHashTp;
struct depNode
{
	depNode(){ pak = NULL; }
	depNode(const std::string &_name) :name(_name){ pak = NULL; }
	depNode(package *_pak){ pak = _pak; name = pak->getName(); }
	bool processed = true;
	package *pak;
	std::string name;
	std::unordered_multimap<int, depInfo> con;
	std::unordered_map<int, int> ancestor;
	pakIDHashTp dep;
	pakIDHashTp bedep_dep, bedep_conf;
};
typedef std::unordered_map<int, depNode> depMapTp;
typedef std::unordered_map<std::string, int> depHashTp;
struct dep_throw{ dep_throw(){ id = INT_MIN; }; dep_throw(std::string _msg, int _id) :msg(_msg){ id = _id; }; std::string msg; int id; };
struct dep_trace{ dep_trace(){ id = INT_MIN; isDep = false; }; dep_trace(int _id, bool _isDep){ id = _id; isDep = _isDep; }; int id; bool isDep; };
typedef std::list<dep_trace> pakTListTp;

void clean_dep(depMapTp &pakMap, depHashTp &pakHash, int nodeID)
{
	pakQListTp que;
	depNode &node = pakMap.at(nodeID);
	std::for_each(node.dep.begin(), node.dep.end(), [&](int id){
		depNode &depNode = pakMap.at(id);
		depNode.con.erase(nodeID);
		depNode.bedep_dep.erase(nodeID);
		depNode.bedep_conf.erase(nodeID);
		if (id != nodeID)
			que.push_back(id);
	});
	node.dep.clear();
	while (!que.empty())
	{
		int id = que.front();
		depNode depN = pakMap.at(id);
		que.pop_front();
		std::for_each(node.ancestor.cbegin(), node.ancestor.cend(), [&depN](const std::pair<int, int>& p){
			depN.ancestor.at(p.first) -= p.second;
			if (depN.ancestor.at(p.first) <= 0)
				depN.ancestor.erase(p.first);
		});
		depN.ancestor.at(nodeID)--;
		if (depN.ancestor.at(nodeID) == 0)
			depN.ancestor.erase(nodeID);
		if (depN.ancestor.empty() && id != 0)
		{
			clean_dep(pakMap, pakHash, id);
			pakHash.erase(depN.pak->getName());
			pakMap.erase(id);
		}
		else
		{
			std::for_each(depN.dep.begin(), depN.dep.end(), [&](int nid){
				if (node.ancestor.find(nid) == node.ancestor.end() && nid != id)
					que.push_back(nid);
			});
		}
	}
}

void add_ancestor(depMapTp &pakMap, depHashTp &pakHash, int dst, int src)
{
	if (src == dst)
		return;
	depNode &srcN = pakMap.at(src), &dstN = pakMap.at(dst);
	std::for_each(srcN.ancestor.cbegin(), srcN.ancestor.cend(), [&dstN](const std::pair<int, int>& p){
		dstN.ancestor[p.first] += p.second;
	});
	dstN.ancestor[src]++;
	if (dstN.processed)
	{
		std::unordered_map<int, int>::iterator ancEnd = dstN.ancestor.end();
		std::for_each(dstN.dep.begin(), dstN.dep.end(), [&](int depID){
			if (dstN.ancestor.find(depID) == ancEnd)
				add_ancestor(pakMap, pakHash, depID, src);
		});
	}
}

void force_remove(depMapTp &pakMap, depHashTp &pakHash, pakRListTp &removeList, pakRHashTp &removeHash, std::string &confName, int id, int &nextID)
{
	depNode &thisNode = pakMap.at(id);
	depHashTp::iterator itrHash, itrHashEnd = pakHash.end();

	itrHash = pakHash.find(confName);
	int confID = itrHash->second;

	removeList.push_front(confName);
	pakRListTp::iterator itrEnd = removeList.begin();
	uninst_list(confName, removeList, removeHash);
	removeHash.emplace(confName);

	std::unordered_map<int, int>::iterator ancEnd = thisNode.ancestor.end();
	for (pakRListTp::iterator itr = removeList.begin(); itr != itrEnd; itr++)	//Add all of packages in the removeList to map
	{
		itrHash = pakHash.find(*itr);
		int rmID;
		if (itrHash != itrHashEnd)	//The package is in the map
		{
			rmID = itrHash->second;
			depNode &rmN = pakMap.at(rmID);
			bool alreadyConf = (rmN.bedep_conf.find(id) != rmN.bedep_conf.end());
			if (alreadyConf)
				rmN.bedep_conf.emplace(id);
			rmN.con.emplace(id, depInfo(*itr, version(), depInfo::NONE, false));
			if (!rmN.bedep_dep.empty() || thisNode.ancestor.find(rmID) != ancEnd)
			{
				if (!alreadyConf)
					rmN.bedep_conf.erase(id);
				rmN.bedep_dep.emplace(confID);
				throw(dep_throw(msgData[MSGE_DEP_CONF] + ':' + rmN.name, rmID));
			}
			else
				rmN.pak = NULL;
			if (thisNode.ancestor.find(rmID) == ancEnd)
				add_ancestor(pakMap, pakHash, rmID, id);
			thisNode.dep.emplace(rmID);
		}
		else	//Add the package to the map
		{
			int newID = nextID;
			nextID++;
			rmID = newID;
			thisNode.dep.emplace(newID);
			depNode &rmN = pakMap.emplace(newID, depNode(*itr)).first->second;
			pakHash.emplace(*itr, newID);
			itrHash = pakHash.find(*itr);
			rmN.bedep_conf.emplace(id);
			rmN.con.emplace(id, depInfo(*itr, version(), depInfo::NONE, false));
			std::for_each(thisNode.ancestor.cbegin(), thisNode.ancestor.cend(), [&rmN](const std::pair<int, int>& p){
				rmN.ancestor[p.first] += p.second;
			});
			rmN.ancestor[id]++;
		}
	}

	if (removeHash.find(thisNode.name) != removeHash.end())
		throw(msgData[MSGE_DEP_CONF] + ':' + thisNode.name);
}

void trace_back(depMapTp &pakMap, pakTListTp &que, pakIDHashTp &queHash, std::list<pakTListTp> &ret, int id, bool isDep)
{
	const int targetID = 0;
	if (id == targetID)
	{
		que.push_front(dep_trace(id, true));
		ret.push_back(que);
		que.pop_front();
		return;
	}
	if (queHash.find(id) == queHash.end())
	{
		queHash.emplace(id);
		que.push_front(dep_trace(id, isDep));
		depNode &node = pakMap.at(id);
		std::for_each(node.bedep_dep.begin(), node.bedep_dep.end(), [&](int nextID){
			trace_back(pakMap, que, queHash, ret, nextID, true);
		});
		std::for_each(node.bedep_conf.begin(), node.bedep_conf.end(), [&](int nextID){
			trace_back(pakMap, que, queHash, ret, nextID, false);
		});
		que.pop_front();
		queHash.erase(id);
	}
}

void package::checkDep(pakIListTp &instList, const depListTp &extraDep, bool force)
{
	//Check requirement:init
	depMapTp pakMap;
	depHashTp pakHash;
	depHashTp::iterator itrHash, itrHashEnd = pakHash.end();
	int nextID = 1;

	depHashTp tmpHash;
	int tmpNextID = -1;
	pakMap.emplace(0, this);
	std::for_each(extraDep.begin(), extraDep.end(), [&](const depInfo& inf){
		int newID = tmpNextID;
		tmpNextID--;
		pakMap.emplace(newID, depNode(inf.name));
		tmpHash.emplace(inf.name, newID);
		pakMap[0].con.emplace(newID, inf);
		pakMap[0].bedep_dep.emplace(newID);
	});
	pakHash.emplace(name, 0);
	pakQListTp pakQue;
	pakQue.push_back(0);
	pakRListTp removeList;
	pakRHashTp removeHash;
	std::list<int> tmpRemoveList;

	//Check requirement:add local info
	fs::directory_iterator p(dataPath), pEnd;
	std::string buf;
	for (; p != pEnd; p++)
	{
		std::string name = p->path().filename().string();
		if (name.front() != '$')
		{
			int newTmpID;
			itrHash = tmpHash.find(name);
			if (itrHash == tmpHash.end())
			{
				newTmpID = tmpNextID;
				tmpNextID--;
				pakMap.emplace(newTmpID, depNode(name));
				tmpHash.emplace(name, newTmpID);
			}
			else
				newTmpID = itrHash->second;

			std::ifstream fin((dataPath / name / FILENAME_CONF).string());
			while (!fin.eof())
			{
				std::getline(fin, buf);
				if (!buf.empty())
				{
					depInfo conf(buf);
					itrHash = pakHash.find(conf.name);
					if (itrHash != itrHashEnd)
					{
						depNode &confN = pakMap.at(itrHash->second);
						confN.con.emplace(newTmpID, conf);
						confN.bedep_conf.emplace(newTmpID);
					}
					else
					{
						int newID = nextID;
						nextID++;
						depNode &confN = pakMap.emplace(newID, depNode(conf.name)).first->second;
						pakHash.emplace(conf.name, newID);
						itrHash = pakHash.find(conf.name);
						confN.con.emplace(newTmpID, conf);
						confN.bedep_conf.emplace(newTmpID);
					}
				}
			}
			fin.close();
		}
	}

	{
		depNode &instN = pakMap.at(0);
		if (!instN.con.empty())
		{
			std::for_each(instN.con.begin(), instN.con.end(), [&, this](std::pair<int, depInfo> p){
				if (!p.second.check(ver))
				{
					if (force)
						force_remove(pakMap, pakHash, removeList, removeHash, pakMap.at(p.first).name, 0, nextID);
					else
						throw(msgData[MSGE_CONF] + ':' + p.second.fullStr());
				}
			});
		}
	}

	depListTp::iterator pDep, pDepEnd;
	//Check requirement:BFS
	try
	{
		while (!pakQue.empty())
		{
			int id = pakQue.front();
			depNode &cur_node = pakMap.at(id);

			pDep = cur_node.pak->confList.begin();
			pDepEnd = cur_node.pak->confList.end();
			for (; pDep != pDepEnd; pDep++)
			{
				int confID;
				itrHash = pakHash.find(pDep->name);
				if (itrHash != itrHashEnd)	//The package is in the map
				{
					confID = itrHash->second;
					depNode &confN = pakMap.at(confID);
					confN.con.emplace(id, ~(*pDep));
					confN.bedep_conf.emplace(id);
					if (id != confID)
					{
						if (cur_node.ancestor.find(confID) == cur_node.ancestor.end())
							add_ancestor(pakMap, pakHash, confID, id);
						cur_node.dep.emplace(confID);
					}
				}
				else	//Add the package to the map
				{
					int newID = nextID;
					nextID++;
					confID = newID;
					cur_node.dep.emplace(newID);
					depNode &confN = pakMap.emplace(newID, depNode(pDep->name)).first->second;
					pakHash.emplace(pDep->name, newID);
					confN.con.emplace(id, ~(*pDep));
					confN.bedep_conf.emplace(id);
					std::for_each(cur_node.ancestor.cbegin(), cur_node.ancestor.cend(), [&confN](const std::pair<int, int>& p){
						confN.ancestor[p.first] += p.second;
					});
					confN.ancestor[id]++;
				}
				if (pDep->check() && removeHash.find(pDep->name) == removeHash.end())	//Conflict with installed package
				{
					depNode &confN = pakMap.at(confID);
					clean_dep(pakMap, pakHash, confID);
					package *pak = find_package(confN.name, confN.con, false, tmpRemoveList);	//Try to find a correct package
					if (pak == NULL)
					{
						if (!confN.bedep_dep.empty() || cur_node.ancestor.find(confID) != cur_node.ancestor.end())
							throw(dep_throw(msgData[MSGE_DEP_CONF] + ':' + confN.name, confID));
						if (force)
						{
							force_remove(pakMap, pakHash, removeList, removeHash, pDep->name, id, nextID);
							if (cur_node.pak == NULL)
								break;
						}
						else
						{
							if (cur_node.pak->name == pDep->name)
								throw(dep_throw(msgData[MSGE_DEP_CONF] + ':' + pDep->name, id));
							else
								throw(dep_throw(msgData[MSGE_CONF] + ':' + cur_node.pak->name + ":" + pDep->fullStr(), id));
						}
					}
					confN.pak = pak;
					if (pak != NULL && confN.processed && confID != id)
					{
						confN.processed = false;
						pakQue.push_back(confID);
					}
				}
				else
				{
					depNode &confN = pakMap.at(confID);
					if (confN.pak != NULL && pDep->check(confN.pak->ver))	//Conflict with installing package
					{
						if (cur_node.ancestor.find(confID) != cur_node.ancestor.end())	//Loop in map
							throw(dep_throw(msgData[MSGE_DEP] + ':' + cur_node.pak->name + ":" + pDep->fullStr(), id));
						clean_dep(pakMap, pakHash, confID);
						package *pak = find_package(confN.name, confN.con, false, tmpRemoveList);
						if (pak == NULL)
						{
							if (!confN.bedep_dep.empty() || cur_node.ancestor.find(confID) != cur_node.ancestor.end())
								throw(dep_throw(msgData[MSGE_DEP_CONF] + ':' + confN.name, confID));
							if (force)
							{
								force_remove(pakMap, pakHash, removeList, removeHash, pDep->name, id, nextID);
								if (cur_node.pak == NULL)
									break;
							}
							else
							{
								std::string errMessage(msgData[MSGI_PAK_NOT_FOUND_SPEC_1]);
								errMessage.push_back('\n');
								std::for_each(confN.con.begin(), confN.con.end(), [&errMessage, &pakMap](std::pair<int, depInfo> p){
									errMessage.push_back('\t');
									errMessage.append(pakMap.at(p.first).name);
									errMessage.append(p.second.conStr());
									errMessage.push_back('\n');
								});
								errMessage.append(msgData[MSGI_PAK_NOT_FOUND_SPEC_2]);
								throw(dep_throw(msgData[MSGE_PAK_NOT_FOUND_SPEC] + ":\n\t" + confN.name + '\n' + errMessage, confID));
							}
						}
						confN.pak = pak;
						if (confN.processed && confID != id)
						{
							confN.processed = false;
							pakQue.push_back(confID);
						}
					}
				}
			}
			if (cur_node.pak == NULL)
			{
				cur_node.processed = true;
				pakQue.pop_front();
				continue;
			}

			pDep = cur_node.pak->depList.begin();
			pDepEnd = cur_node.pak->depList.end();
			for (; pDep != pDepEnd; pDep++)
			{
				int depID;
				itrHash = pakHash.find(pDep->name);
				if (itrHash != itrHashEnd)	//The package is in the map
				{
					depID = itrHash->second;
					depNode &depN = pakMap.at(depID);
					depN.bedep_dep.emplace(id);
					depN.con.emplace(id, *pDep);
					if (cur_node.ancestor.find(depID) == cur_node.ancestor.end())
						add_ancestor(pakMap, pakHash, depID, id);
					cur_node.dep.emplace(depID);
					if (removeHash.find(pDep->name) != removeHash.end())
						throw(dep_throw(msgData[MSGE_DEP_CONF] + ':' + pDep->name, depID));
					if ((depN.pak == NULL && !pDep->check()) || (depN.pak != NULL && !pDep->check(depN.pak->ver)))	//Checking failed
					{
						if (cur_node.ancestor.find(itrHash->second) != cur_node.ancestor.end())	//Loop in map
							throw(dep_throw(msgData[MSGE_DEP] + ':' + cur_node.pak->name + ":" + pDep->fullStr(), id));
						clean_dep(pakMap, pakHash, depID);
						package *pak = find_package(pDep->name, depN.con, force, tmpRemoveList);
						if (pak == NULL)
						{
							std::string errMessage(msgData[MSGI_PAK_NOT_FOUND_SPEC_1]);
							errMessage.push_back('\n');
							std::for_each(depN.con.begin(), depN.con.end(), [&errMessage, &pakMap](std::pair<int, depInfo> p){
								errMessage.push_back('\t');
								errMessage.append(pakMap.at(p.first).name);
								errMessage.append(p.second.conStr());
								errMessage.push_back('\n');
							});
							errMessage.append(msgData[MSGI_PAK_NOT_FOUND_SPEC_2]);
							throw(dep_throw(msgData[MSGE_PAK_NOT_FOUND_SPEC] + ":\n\t" + pDep->name + '\n' + errMessage, depID));
						}
						else if (!tmpRemoveList.empty())
						{
							std::for_each(tmpRemoveList.begin(), tmpRemoveList.end(), [&](int pakID){
								depNode &confN = pakMap.at(pakID);
								if (!confN.bedep_dep.empty() || cur_node.ancestor.find(pakID) != cur_node.ancestor.end())
									throw(dep_throw(msgData[MSGE_DEP_CONF] + ':' + confN.name, pakID));
								force_remove(pakMap, pakHash, removeList, removeHash, confN.name, id, nextID);
								if (cur_node.pak == NULL)
									throw(dep_throw(msgData[MSGE_DEP_CONF] + ':' + cur_node.name, id));
							});
						}
						depN.pak = pak;
						if (depN.processed && depID != id)
						{
							depN.processed = false;
							pakQue.push_back(itrHash->second);
						}
					}
				}
				else	//Add the package to the map
				{
					int newID = nextID;
					nextID++;
					depID = newID;
					package *pak;
					if (pDep->check())	//Installed correct package
						pak = NULL;
					else
					{
						pak = find_package(pDep->name, *pDep);
						if (pak == NULL)
							throw(dep_throw(msgData[MSGE_PAK_NOT_FOUND_SPEC] + ":\n\t" + cur_node.name + '\n' +
							msgData[MSGI_PAK_NOT_FOUND_SPEC_1] + "\n\t" + pDep->fullStr() + '\n' + msgData[MSGI_PAK_NOT_FOUND_SPEC_2], id));
					}
					cur_node.dep.emplace(newID);
					depNode &depN = pakMap.emplace(newID, depNode(pDep->name)).first->second;
					depN.pak = pak;
					depN.bedep_dep.emplace(id);
					pakHash.emplace(pDep->name, newID);
					depN.con.emplace(id, *pDep);
					std::for_each(cur_node.ancestor.cbegin(), cur_node.ancestor.cend(), [&depN](const std::pair<int, int> p){
						depN.ancestor[p.first] += p.second;
					});
					depN.ancestor[id]++;
					if (pak != NULL && depN.processed)
					{
						depN.processed = false;
						pakQue.push_back(newID);
					}
				}
			}

			pakQue.pop_front();
			cur_node.processed = true;
		}
	}
	catch (dep_throw err)
	{
		std::list<pakTListTp> traceQue;
		pakTListTp traceStack;
		pakIDHashTp traceHash;
		trace_back(pakMap, traceStack, traceHash, traceQue, err.id, true);
		std::string traceMessage(msgData[MSGI_TRACE_INFO]);
		std::for_each(traceQue.begin(), traceQue.end(), [&traceMessage, &pakMap](const pakTListTp &list){
			traceMessage.push_back('\n');
			std::for_each(list.begin(), list.end(), [&traceMessage, &pakMap](dep_trace item){
				traceMessage.push_back('\t');
				traceMessage.push_back(item.isDep ? '^' : '#');
				traceMessage.append(pakMap.at(item.id).name);
				traceMessage.push_back('\n');
			});
		});
		while (traceMessage.back() == '\n')
			traceMessage.pop_back();
		throw(err.msg + '\n' + traceMessage);
	}
	catch (...)
	{
		throw;
	}

	//Check which package need to be installed or upgraded
	pakIDHashTp pakDiff;
	pakIDHashTp::iterator itrDiffEnd = pakDiff.end();
	pakQue.push_back(0);
	pakDiff.emplace(0);
	while (!pakQue.empty())
	{
		int id = pakQue.front();
		depNode &node = pakMap.at(id);
		pakQue.pop_front();
		package *pak = node.pak;
		if (pak != NULL)
		{
			if (is_installed(pak->name))
			{
				if (cur_version(pak->name) != pak->ver)
					instList.push_front(instItem(pak, instItem::UPG));
			}
			else
				instList.push_front(instItem(pak, instItem::INST));
			pDep = node.pak->depList.begin();
			pDepEnd = node.pak->depList.end();
			for (; pDep != pDepEnd; pDep++)
			{
				int depID = pakHash.at(pDep->name);
				if (pakDiff.find(depID) == itrDiffEnd)
				{
					pakQue.push_back(depID);
					pakDiff.emplace(depID);
				}
			}
		}
	}
	while (!removeList.empty())
	{
		instList.push_front(instItem(removeList.back()));
		removeList.pop_back();
	}
}
