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
#include <memory>
#include <exception>

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
#		pragma comment (lib, "wxbase30ud.lib")
#		pragma comment (lib, "wxbase30ud_xml.lib")
#		pragma comment (lib, "wxmsw30ud_adv.lib")
#		pragma comment (lib, "wxmsw30ud_aui.lib")
#		pragma comment (lib, "wxmsw30ud_core.lib")
#		pragma comment (lib, "wxmsw30ud_gl.lib")
#		pragma comment (lib, "wxmsw30ud_html.lib")
#		pragma comment (lib, "wxmsw30ud_media.lib")
#		pragma comment (lib, "wxmsw30ud_propgrid.lib")
#		pragma comment (lib, "wxmsw30ud_qa.lib")
#		pragma comment (lib, "wxmsw30ud_ribbon.lib")
#		pragma comment (lib, "wxmsw30ud_richtext.lib")
#		pragma comment (lib, "wxmsw30ud_stc.lib")
#		pragma comment (lib, "wxmsw30ud_xrc.lib")
#		pragma comment (lib, "wxscintillad.lib")
#		pragma comment (lib, "wxbase30ud.lib")
#		pragma comment (lib, "wxtiffd.lib")
#		pragma comment (lib, "wxjpegd.lib")
#		pragma comment (lib, "wxpngd.lib")
#	else
#		pragma comment (lib, "wxbase30u.lib")
#		pragma comment (lib, "wxbase30u_xml.lib")
#		pragma comment (lib, "wxmsw30u_adv.lib")
#		pragma comment (lib, "wxmsw30u_aui.lib")
#		pragma comment (lib, "wxmsw30u_core.lib")
#		pragma comment (lib, "wxmsw30u_gl.lib")
#		pragma comment (lib, "wxmsw30u_html.lib")
#		pragma comment (lib, "wxmsw30u_media.lib")
#		pragma comment (lib, "wxmsw30u_propgrid.lib")
#		pragma comment (lib, "wxmsw30u_qa.lib")
#		pragma comment (lib, "wxmsw30u_ribbon.lib")
#		pragma comment (lib, "wxmsw30u_richtext.lib")
#		pragma comment (lib, "wxmsw30u_stc.lib")
#		pragma comment (lib, "wxmsw30u_xrc.lib")
#		pragma comment (lib, "wxscintilla.lib")
#		pragma comment (lib, "wxbase30u.lib")
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
