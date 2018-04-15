/*
----
This file is part of SECONDO.
Realizing a simple distributed filesystem for master thesis of stephan scheide

Copyright (C) 2015,
Faculty of Mathematics and Computer Science,
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----


//[$][\$]

*/

#ifndef DFS_DEBUG_H
#define DFS_DEBUG_H

#include "../shared/str.h"
#include "../shared/log.h"

class Debug {
public:
  static dfs::log::Logger* logger;

  static void debug(const dfs::Str& str) {
    if (logger != 0 && logger->canDebug) logger->debug(str);
  }

  static void debug(const dfs::Str& caption, int value) {
    debug(caption.append(": ").append(value));
  }

};

#endif //DFS_DEBUG_H
