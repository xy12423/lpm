/*
Live Package Manager, Package Manager for LBLive
Copyright (C) <2015>  <xy12423>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <cstring>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <ctime>

#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <list>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <algorithm>
#include <memory>
#include <exception>
#include <utility>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <curl/curl.h>
#include <zlib.h>

#include <wx/platform.h>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#	include <wx/wx.h>
#endif
#ifdef _MSC_VER
#	ifdef _DEBUG
#		pragma comment (lib, "wxbase31ud.lib")
#		pragma comment (lib, "wxbase31ud_xml.lib")
#		pragma comment (lib, "wxmsw31ud_adv.lib")
#		pragma comment (lib, "wxmsw31ud_aui.lib")
#		pragma comment (lib, "wxmsw31ud_core.lib")
#		pragma comment (lib, "wxmsw31ud_gl.lib")
#		pragma comment (lib, "wxmsw31ud_html.lib")
#		pragma comment (lib, "wxmsw31ud_media.lib")
#		pragma comment (lib, "wxmsw31ud_propgrid.lib")
#		pragma comment (lib, "wxmsw31ud_qa.lib")
#		pragma comment (lib, "wxmsw31ud_ribbon.lib")
#		pragma comment (lib, "wxmsw31ud_richtext.lib")
#		pragma comment (lib, "wxmsw31ud_stc.lib")
#		pragma comment (lib, "wxmsw31ud_xrc.lib")
#		pragma comment (lib, "wxscintillad.lib")
#		pragma comment (lib, "wxbase31ud.lib")
#		pragma comment (lib, "wxtiffd.lib")
#		pragma comment (lib, "wxjpegd.lib")
#		pragma comment (lib, "wxpngd.lib")
#	else
#		pragma comment (lib, "wxbase31u.lib")
#		pragma comment (lib, "wxbase31u_xml.lib")
#		pragma comment (lib, "wxmsw31u_adv.lib")
#		pragma comment (lib, "wxmsw31u_aui.lib")
#		pragma comment (lib, "wxmsw31u_core.lib")
#		pragma comment (lib, "wxmsw31u_gl.lib")
#		pragma comment (lib, "wxmsw31u_html.lib")
#		pragma comment (lib, "wxmsw31u_media.lib")
#		pragma comment (lib, "wxmsw31u_propgrid.lib")
#		pragma comment (lib, "wxmsw31u_qa.lib")
#		pragma comment (lib, "wxmsw31u_ribbon.lib")
#		pragma comment (lib, "wxmsw31u_richtext.lib")
#		pragma comment (lib, "wxmsw31u_stc.lib")
#		pragma comment (lib, "wxmsw31u_xrc.lib")
#		pragma comment (lib, "wxscintilla.lib")
#		pragma comment (lib, "wxbase31u.lib")
#		pragma comment (lib, "wxtiff.lib")
#		pragma comment (lib, "wxjpeg.lib")
#		pragma comment (lib, "wxpng.lib")
#	endif
#endif

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment (lib, "libcurld.lib")
#pragma comment (lib, "libeay32d.lib")
#pragma comment (lib, "ssleay32d.lib")
#pragma comment (lib, "libssh2d.lib")
#pragma comment (lib, "zlibd.lib")
#else
#pragma comment (lib, "libcurl.lib")
#pragma comment (lib, "libeay32.lib")
#pragma comment (lib, "ssleay32.lib")
#pragma comment (lib, "libssh2.lib")
#pragma comment (lib, "zlib.lib")
#endif
#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "wldap32.lib")
#pragma comment (lib, "comctl32.lib")
#pragma comment (lib, "rpcrt4.lib")
#pragma comment (lib, "wsock32.lib")
#pragma comment (lib, "odbc32.lib")
#endif

#if (!defined(WIN32)) && (defined(_WIN32) || defined(_WIN32_WINNT))
#define WIN32
#endif

#if (!defined(__linux__)) && (defined(__linux))
#define __linux__
#endif
