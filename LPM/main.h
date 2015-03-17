#pragma once

#ifndef _H_MAIN
#define _H_MAIN

#include "globalVar.h"
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
void init();

#endif
