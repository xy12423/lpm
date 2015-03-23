#pragma once

#ifndef _H_ZIP
#define _H_ZIP

#include "errInfo.h"
#include "global.h"

typedef std::vector<BYTE> dataBuf;
typedef void(*fProgressReportCallback)(double progress);
extern fProgressReportCallback prCallbackP;
errInfo unzip(dataBuf::const_iterator dataBegin, dataBuf::const_iterator dataEnd, boost::filesystem::path path);

#endif
