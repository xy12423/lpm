#pragma once

#ifndef _H_VAR
#define _H_VAR

extern boost::filesystem::path localPath, dataPath;
extern std::ostream &infoStream;

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
const std::string SCRIPT_PURGE = "purge.bat";
const std::string SCRIPT_REMOVE = "remove.bat";
#endif
#ifdef __linux__
const std::string SCRIPT_INST = "inst.sh";
const std::string SCRIPT_INIT = "init.sh";
const std::string SCRIPT_PURGE = "purge.sh";
const std::string SCRIPT_REMOVE = "remove.sh";
#endif

#endif
