#pragma once

#ifndef _H_GLOBAL
#define _H_GLOBAL

extern boost::filesystem::path localPath, dataPath;
#ifdef _LPM_GUI
extern std::stringstream infoStream;
#else
extern std::ostream &infoStream;
#endif
std::ostream& myEndl(std::ostream& os);

const std::string DIRNAME_TEMP = "$temp";
const std::string DIRNAME_NATIVE = "$native";
const std::string DIRNAME_UPGRADE = "$upgrade";
const std::string DIRNAME_INFO = "$info";
const std::string FILENAME_INST = "inst.inf";
const std::string FILENAME_INFO = "info.inf";
const std::string FILENAME_BEDEP = "bedep.inf";
const std::string FILENAME_DEP = "dep.inf";
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

typedef unsigned short int USHORT;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef unsigned long long ULONGLONG;
typedef unsigned char BYTE;

#endif