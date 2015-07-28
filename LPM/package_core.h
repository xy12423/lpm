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

#ifndef _H_PKG_CORE
#define _H_PKG_CORE

typedef std::list<std::string> pakRListTp;
typedef std::unordered_set<std::string> pakRHashTp;
void uninst_list(const std::string &name, pakRListTp &removeList, pakRHashTp &removeHash);

#endif
