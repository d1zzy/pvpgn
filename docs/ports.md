# PvPGN Porting Status
Arch    | Operating System          | Status    |   Compiler                          |   CMake
-----   | ------------------------- | --------- | ----------------------------------- | ---------
x86_64  |   Arch Linux 2015.09.29   |   OK      |   G++ 5.2.0                         |   3.3.2
x86_64  |   Arch Linux 2015.09.29   |   OK      |   Clang 3.7.0                       |   3.3.2
x86_64  |   Arch Linux 2015.09.29   |   NO      |   MinGW 5.2.0                       |   3.3.2
x86_64  |   Ubuntu Server 15.04     |   OK      |   G++ 5.1.1                         |   3.3.2
x86_64  |   Ubuntu Server 15.04     |   OK      |   G++ 5.1.0                         |   3.4.0-rc1
x86_64  |   Ubuntu Server 14.04     |   OK      |   G++ 5.1.0                         |   3.4.0-rc1
x86_64  |   CentOS 7.3.1611         |   OK      |   G++ 5.3.1                         |   3.6.1
x86_64  |   Fedora 25               |   OK      |   G++ 6.2.1                         |   3.6.2
x86_64  |   Windows 10.0.10240      |   OK      |   VC++ 14                           |   3.3.1
x86_64  |   Windows 10.0.10240      |   OK      |   Clang r249555                     |   3.3.1
x86_64  |   OS X 10.11.6            |   OK      |   Apple LLVM 7.3.0 (clang-703.0.31) |   3.6.1

# Notes
- Any POSIX/WIN32 OS with a C++11 compliant compiler should work
- The minimum required compiler version for G++ and Visual Studio is hardcoded
