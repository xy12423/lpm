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
#pragma once

#ifndef _H_GLOBAL
#define _H_GLOBAL

typedef unsigned short int USHORT;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef unsigned long long ULONGLONG;
typedef unsigned char BYTE;

extern boost::filesystem::path localPath, dataPath, langPath;
extern std::ostream &infoStream;

#include "langn.h"
extern std::string msgDataDefault[msgCount];
extern std::string msgData[msgCount];

typedef void(*fProgressReportCallback)(double progress, size_t size_downloaded);
extern fProgressReportCallback prCallbackP;

const std::string DIRNAME_TEMP = "$temp";
const std::string DIRNAME_BACKUP = "$backup";
const std::string DIRNAME_NATIVE = "$native";
const std::string DIRNAME_UPGRADE = "$upgrade";
const std::string DIRNAME_INFO = "$info";
const std::string DIRNAME_PATH = "$path";
const std::string FILENAME_INST = "inst.inf";
const std::string FILENAME_INFO = "info.inf";
const std::string FILENAME_BEDEP = "bedep.inf";
const std::string FILENAME_DEP = "dep.inf";
const std::string FILENAME_CONF = "conf.inf";
const std::string FILENAME_LOCK = "lpm.lck";
#ifdef WIN32
const std::string SCRIPT_INST = "inst.bat";
const std::string SCRIPT_INIT = "init.bat";
const std::string SCRIPT_PURGE = "clear.bat";
const std::string SCRIPT_REMOVE = "remove.bat";
#endif
#ifdef __linux__
const std::string SCRIPT_INST = "inst.sh";
const std::string SCRIPT_INIT = "init.sh";
const std::string SCRIPT_PURGE = "clear.sh";
const std::string SCRIPT_REMOVE = "remove.sh";
#endif
const UINT DIR_ATTRIBUTE = 0x10;

typedef std::vector<BYTE> dataBuf;

struct errInfo
{
	errInfo(){ err = false; }
	errInfo(std::string _info) :info(_info){ err = true; }
	errInfo(const char *_info) :info(_info){ err = true; }
	bool err;
	std::string info;
};

void processEscChar(std::string &str);
int run_script(fs::path scriptPath, fs::path runPath);

#endif
