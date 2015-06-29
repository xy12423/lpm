#pragma once

#ifndef _H_ERRINFO
#define _H_ERRINFO

struct errInfo
{
	errInfo(){ err = false; }
	errInfo(std::string _info) :info(_info){ err = true; }
	errInfo(const char *_info) :info(_info){ err = true; }
	bool err;
	std::string info;
};

char* str2cstr(std::string arg);
std::string num2str(long long n);

#endif
