Player versus Player Gaming Network
=====
![](http://harpywar.com/images/items/pvpgn.gif)

Original PvPGN 1.99 source with tweaks.

Source Code changes
--
* fix command send flooding <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/74f9e4faafe24699597e4be5bfda83bf255ba72e)</sup>
* fix compile error when pointer size is larger than int <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/1ea116434ce009bad4903ff72bd69bbb8987ce06)</sup>
* fix Warcraft 3 ICON SWITCH hack <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/84811bcfe875d6c42cd8271bbdae757f0b5d445b)</sup>
* fix saving sql fields with custom characters in the name <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/18713ffe35cbe9a12193e5c1f1caf5031d4c4731)</sup>
* add game id field in status.xml output <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/b989d26e1182a3ee8cf62f3ee79dfb231fd66e23)</sup>
* add game id field and user game version in status.dat output (the same as status.xml) <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/39d0b2be71c7ddd808a20f97fe6ac17078ce013f)</sup>

New commands
--
* `/save` immediately save changes of accounts and clans from the cache to a storage (useful for testing) <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/be8d65d16f910b2090b0db9e7eb2c043b816dae7)</sup>


Minor changes
--
* add original MOTD files with UTF-8 encoding 
* add support files v1.2, there are no need to download it separately after install
* add ODBC support for CMake
* add latest versioncheck.conf
* skip_versioncheck, allow_bad_version are enabled by default in bnetd.conf - for easy start
* source code is formatted for better reading
* unused files are removed, only pvpgn source here

Build source code
--
[Русский](http://harpywar.com/?a=articles&b=2&c=1&d=74) | [English](http://harpywar.com/?a=articles&b=2&c=1&d=74&lang=en)

[![Vimeo](http://habrastorage.org/storage3/48c/5a9/4b1/48c5a94b1173242e311f8376be80a585.png)](https://vimeo.com/83763862)
