#pragma once

#ifndef _H_DOWN
#define _H_DOWN

#include "errInfo.h"
#include "global.h"

errInfo download(const std::string &add, dataBuf *buf);
errInfo download(const std::string &add, std::string path);

#endif
