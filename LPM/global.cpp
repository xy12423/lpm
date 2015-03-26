#include "stdafx.h"
#include "global.h"

boost::filesystem::path localPath, dataPath, langPath;
std::ostream &infoStream = std::cout;

std::string msgData[msgCount] = {

};

fProgressReportCallback prCallbackP = NULL;
