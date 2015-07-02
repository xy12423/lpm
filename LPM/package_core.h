#pragma once

#ifndef _H_PKG_CORE
#define _H_PKG_CORE

typedef std::list<std::string> pakRListTp;
typedef std::unordered_set<std::string> pakRHashTp;
void uninst_list(const std::string &name, pakRListTp &removeList, pakRHashTp &removeHash);

#endif