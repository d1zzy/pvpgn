Player versus Player Gaming Network
=====

Original PvPGN 1.99 source with tweaks.

Source Code changes
--
* fix command send flooding <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/74f9e4faafe24699597e4be5bfda83bf255ba72e)</sup>
* fix compile error when pointer size is larger than int <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/1ea116434ce009bad4903ff72bd69bbb8987ce06)</sup>
* fix Warcraft 3 ICON SWITCH hack <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/84811bcfe875d6c42cd8271bbdae757f0b5d445b)</sup>
* add game id field in status.xml output <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/b989d26e1182a3ee8cf62f3ee79dfb231fd66e23)</sup>
* add game id field and user game version in status.dat output (the same as status.xml) <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/39d0b2be71c7ddd808a20f97fe6ac17078ce013f)</sup>


Minor changes
--
* add original MOTD files with UTF-8 encoding 
* add support files v1.2, there are no need to download it separately after install
* add ODBC support for CMake
* add latest versioncheck.conf
* skip_versioncheck, allow_bad_version are enabled by default in bnetd.conf - for easy start
