#pragma once

#ifndef _H_ZIP
#define _H_ZIP

#include "errInfo.h"
#include "global.h"

errInfo unzip(dataBuf::const_iterator dataBegin, dataBuf::const_iterator dataEnd, boost::filesystem::path path);
errInfo unzip(std::string filePath, boost::filesystem::path path);

#endif
