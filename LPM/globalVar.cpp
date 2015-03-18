#include "stdafx.h"
#include "globalVar.h"

boost::filesystem::path localPath, dataPath;
#ifdef _LPM_GUI
std::stringstream infoStream;
#else
std::ostream &infoStream = std::cout;
#endif
