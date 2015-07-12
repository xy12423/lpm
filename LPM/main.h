#pragma once

#ifndef _H_MAIN
#define _H_MAIN

#include "global.h"
#include "package.h"
#include "source.h"

bool readConfig();
void writeConfig();
void checkPath();
bool readSource();
void writeSource();
bool readLocal();
void writeLocal();
void loadDefaultLang();
bool readLang();
errInfo update();
errInfo upgrade(std::string name, bool force = false);
errInfo upgrade();
void checkUpgrade(pakListTp &ret);
bool check(std::string name);
int getState(std::string name);
void init();
bool lock();
void unlock();

const int PAK_STATE_DEFAULT = 0x00;
const int PAK_STATE_IN_SOURCE = 0x01;
const int PAK_STATE_INSTALLED = 0x02;
const int PAK_STATE_NEED_UPGRADE = 0x04;

#endif
