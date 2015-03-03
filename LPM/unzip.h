#pragma once

#ifndef _H_ZIP
#define _H_ZIP

#include "errInfo.h"

typedef std::vector<BYTE> dataBuf;
errInfo unzip(dataBuf::const_iterator dataBegin, dataBuf::const_iterator dataEnd, boost::filesystem::path path);

#endif
