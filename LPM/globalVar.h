#pragma once

#ifndef _H_VAR
#define _H_VAR

extern boost::filesystem::path localPath, dataPath;
extern std::ostream &infoStream;

const std::string DIRNAME_TEMP = "$temp";
const std::string DIRNAME_NATIVE = "$native";
const std::string DIRNAME_INFO = "$info";
const std::string FILENAME_INST = "inst.inf";
const std::string FILENAME_INFO = "info.inf";
const std::string FILENAME_BEDEP = "bedep.inf";
const std::string FILENAME_DEP = "dep.inf";

#endif
