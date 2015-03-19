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
errInfo update();
errInfo upgrade(std::string name);
errInfo upgrade();
bool check(std::string name);
int getState(std::string name);
void init();

const int PAK_STATE_DEFAULT = 0x00;
const int PAK_STATE_IN_SOURCE = 0x01;
const int PAK_STATE_INSTALLED = 0x02;
const int PAK_STATE_NOT_INSTALLED = 0x04;
const int PAK_STATE_NEED_UPGRADE = 0x08;

#endif
