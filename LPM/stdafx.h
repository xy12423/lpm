#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#include <boost/filesystem.hpp>
#include <curl/curl.h>
#include <zlib.h>

#ifdef _DEBUG
#pragma comment (lib, "libcurld.lib")
#pragma comment (lib, "libeay32d.lib")
#pragma comment (lib, "ssleay32d.lib")
#pragma comment (lib, "zlibd.lib")
#else
#pragma comment (lib, "libcurl.lib")
#pragma comment (lib, "libeay32.lib")
#pragma comment (lib, "ssleay32.lib")
#pragma comment (lib, "zlib.lib")
#endif
#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "wldap32.lib")
