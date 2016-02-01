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
void set_default_path();
void init();
bool lock();
void unlock();

const int PAK_STATE_DEFAULT = 0x00;
const int PAK_STATE_IN_SOURCE = 0x01;
const int PAK_STATE_INSTALLED = 0x02;
const int PAK_STATE_NEED_UPGRADE = 0x04;

#endif
