Player vs Player Gaming Network - PRO
=====
![](http://i.imgur.com/LfI3hXo.png)

PvPGN is a free and open source cross-platform server software that supports Battle.net and and Westwood Online game clients. PvPGN-PRO is a fork of the official PvPGN project, whose development stopped in 2011, and aims to provide continued maintenance and additional features for PvPGN.

[![License (GPL version 2)](https://img.shields.io/badge/license-GNU%20GPL%20version%202-blue.svg?style=flat-square)](http://opensource.org/licenses/GPL-2.0)
![Language (C++)](https://img.shields.io/badge/powered_by-C++-brightgreen.svg?style=flat-square)
[![Language (Lua)](https://img.shields.io/badge/powered_by-Lua-red.svg?style=flat-square)](https://lua.org)
[![Github Releases (by Release)](https://img.shields.io/github/downloads/pvpgn/pvpgn-server/1.99.7.1-PRO/total.svg?maxAge=2592000)]()

[![Compiler (Microsoft Visual C++)](https://img.shields.io/badge/compiled_with-Microsoft%20Visual%20C++-yellow.svg?style=flat-square)](https://msdn.microsoft.com/en-us/vstudio/hh386302.aspx)
[![Compiler (LLVM/Clang)](https://img.shields.io/badge/compiled_with-LLVM/Clang-lightgrey.svg?style=flat-square)](http://clang.llvm.org/)
[![Compiler (GCC)](https://img.shields.io/badge/compiled_with-GCC-yellowgreen.svg?style=flat-square)](https://gcc.gnu.org/)

[![Build Status](https://travis-ci.org/pvpgn/pvpgn-server.svg?branch=master)](https://travis-ci.org/pvpgn/pvpgn-server)
[![Build status](https://ci.appveyor.com/api/projects/status/dqoj9lkvhfwthmn6)](https://ci.appveyor.com/project/HarpyWar/pvpgn)

[Deleaker](http://www.deleaker.com/) helps us find memory leaks.

## Tracking
By default, tracking is enabled and is only used for the purpose of sending informational data (e.g. server description, homepage, uptime, amount of users) to tracking servers. To disable tracking, set ````track = 0```` in ````conf/bnetd.conf````.

## Support
[Create an issue](https://github.com/pvpgn/pvpgn-server/issues) if you have any questions, suggestions, or anything else to say about PvPGN-PRO. Please note that D2GS is not part of the PvPGN project and is therefore unsupported here.

## Development
Submit pull requests to contribute to this project. Utilize C++11 features and adhere to the [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md) whenever possible.

## Building
See [docs/ports.md](https://github.com/pvpgn/pvpgn-server/blob/master/docs/ports.md) for operating systems and compilers that have been confirmed to work with PvPGN. Any operating system that supports WinAPI or POSIX, and any C++11 compliant compiler should be able to build PvPGN.

#### Windows
Use [Magic Builder](https://github.com/pvpgn/pvpgn-server-magic-builder). 

#### Linux

````
apt-get install git install cmake make build-essential zlib1g-dev
apt-get install liblua5.1-0-dev #Lua support
apt-get install mysql-server mysql-client libmysqlclient-dev #MySQL support
cd /home
git clone https://github.com/pvpgn/pvpgn-server.git
cmake -D CMAKE_INSTALL_PREFIX=/usr/local/pvpgn -D WITH_MYSQL=true -D WITH_LUA=true ../
make
make install
````

Full instructions: [Русский](http://harpywar.com/?a=articles&b=2&c=1&d=74) | [English](http://harpywar.com/?a=articles&b=2&c=1&d=74&lang=en)

## License

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
