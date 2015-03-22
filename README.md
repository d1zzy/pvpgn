Player versus Player Gaming Network
=====
![](http://i.imgur.com/LfI3hXo.png)

Next generation of PvPGN &mdash; Battle.net<sup> v1.0</sup> game server emulator.

*The project is still in development. Stay tuned and report any bugs!*

[Deleaker](http://www.deleaker.com/) helps us to find leaks

[![Build Status](https://travis-ci.org/HarpyWar/pvpgn.svg?branch=master)](https://travis-ci.org/HarpyWar/pvpgn) [![Build status](https://ci.appveyor.com/api/projects/status/dqoj9lkvhfwthmn6)](https://ci.appveyor.com/project/HarpyWar/pvpgn)


Source Code changes (since v1.99)
--
* Lua scripting support (WITH_LUA cmake directive)
* Add display custom icons depends on a user rating, custom output for command `/stats`; works with Warcraft and Starcraft (see [icons.conf](https://github.com/HarpyWar/pvpgn/blob/master/conf/icons.conf.in)) <sup>commits [1](https://github.com/HarpyWar/pvpgn/commit/c11af352603e18acc52102ba8574776425248331), [2](https://github.com/HarpyWar/pvpgn/commit/368c4b9296d18a515af746b65fe69054ab6f4236), [3](https://github.com/HarpyWar/pvpgn/commit/f1a96c392055a777b48dc4d77631c5e906161e28)</sup>
* fix command send flooding <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/74f9e4faafe24699597e4be5bfda83bf255ba72e)</sup>
* fix compile error when pointer size is larger than int <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/1ea116434ce009bad4903ff72bd69bbb8987ce06)</sup>
* fix Warcraft 3 ICON SWITCH hack <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/84811bcfe875d6c42cd8271bbdae757f0b5d445b)</sup>
* fix saving sql fields with custom characters in the name <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/18713ffe35cbe9a12193e5c1f1caf5031d4c4731)</sup>
* fix status.xml output errors and add a new game_id field
* welcome text for Warcraft 3 is moved from the code into a new file `bnmotd_w3.txt` ([example](http://img21.imageshack.us/img21/1808/j2py.png) with colored text is included)
 <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/ff8ca941cd7942bab201607fbc31382837a35617)</sup>
* Feature to use d2s character as a template for a new character in newbie.save; each character class can have it's own template <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/20)</sup>
* Full localization <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/13)</sup>
* Ignore flood protection for reserved users and bots <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/49)</sup>
* Several bug fixes which allow hackers to crash a server and a game client
* Logging for user commands <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/47)</sup>
* SID_READMEMORY implementation <sup>[issue](https://github.com/HarpyWar/pvpgn/pull/26)</sup>
* SID_EXTRAWORK implementation <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/72)</sup>
* Improve SQL storage performance <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/85)</sup>

New commands
--
* `/save` immediately save changes of accounts and clans from the cache to a storage (useful for testing) <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/be8d65d16f910b2090b0db9e7eb2c043b816dae7)</sup>
* `/icon <add|del|list>` icon stash implementation - each user has it's own stash with icons, admin/operator can add icon to user's stash, you can set aliases for icons in config; works with Warcraft and Starcraft <sup>commits [1](https://github.com/HarpyWar/pvpgn/commit/1ade081c6b10a3e710130b88613b71b880ba0cd7), [2](https://github.com/HarpyWar/pvpgn/commit/36deb1179bca931bd6585c2b6dbf7d8ade08bc8e)</sup>
* `/find <substr of username>` search account by part of the name - [patch #1526](http://developer.berlios.de/patch/?func=detailpatch&patch_id=1526&group_id=2291) from berlios <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/c229c6693b3dd55f02fe3a81403870044c0786b2)</sup>
* `/quiz` Trivia Quiz Game (implemented in Lua) <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/ee04fdd23dfef90f0b852a6e90df23c7f5edc08e)</sup>
* `/language <name>` change user language
* `/log` show/search in user log

Modified commands
--
* `/set` exclusion to get/set a password hash and user id; feature to empty key with "null" value; more info in output of the command and examples <sup>commits [1](https://github.com/HarpyWar/pvpgn/commit/d96e1029478d92f67000761983e83ccfde2abbdf), [2](https://github.com/HarpyWar/pvpgn/commit/1ade081c6b10a3e710130b88613b71b880ba0cd7#diff-ef576b6b7e90128c3718523eaaf1b894R4716)</sup>
* `/finger` more info in the command output - [patch #2859](http://developer.berlios.de/patch/?func=detailpatch&patch_id=2859&group_id=2291) from berlios <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/bdb450084704da1f33e28c9edd3d2d16b720a946)</sup>
* `/games lobby` show games in lobby only - [patch #3235](http://developer.berlios.de/patch/?func=detailpatch&patch_id=3235&group_id=2291) from berlios <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/5d27cece2c24b5fe779f1560162a31442bf02617)</sup>
* `/friends online` show online friends only - [patch #3236](http://developer.berlios.de/patch/?func=detailpatch&patch_id=3236&group_id=2291) from berlios <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/8762667276b535d3385d51941d41d780089a7049)</sup>
* `/topic` feature to set text on a new line <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/6)</sup>
* `/alert <message>` like /announce but message box shows instead of a text <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/15)</sup>
* `/rehash <mode>` feature to rehash configs separately <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/ee04fdd23dfef90f0b852a6e90df23c7f5edc08e)</sup>


Minor changes
--
* add original MOTD files with UTF-8 encoding 
* add support files v1.2, there are no need to download it separately after install
* add ODBC support for CMake
* add latest versioncheck.conf
* skip_versioncheck, allow_bad_version are enabled by default in bnetd.conf - for easy start
* source code is formatted for better reading
* unused files are removed, only pvpgn source here
* unknown udp packets are logged with enabled debug mode only (it always chokes a log file before) <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/c39f9f03159b2edc8d2457d8134d84486378f9b1)
* add a file location for all the text files like MOTD (often a server admin doesn't know where a file is located)
* add option ignore-version for programs **bnchat** and **bnstat** - [patch #3184](http://developer.berlios.de/patch/?func=detailpatch&patch_id=3184&group_id=2291) from berlios <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/a1fb914c30d9d69d062e8f698f7d0e9bacf41367)
* help for all commands is displayed from bnhelp.conf and format changed for better reading, updated more help messages <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/5)</sup>
* fix error in log when user sends a message with text length of 255 symbols <sup>[commit](https://github.com/HarpyWar/pvpgn/commit/af2baccdb8a2b624627caa94eac5595ac8f76e07)</sup>
* update default tracker servers in config, track is enabled by default <sup>issues [1](https://github.com/HarpyWar/pvpgn/issues/7), [2](https://github.com/HarpyWar/pvpgn/issues/18)</sup>
* fix Windows version identification <sup>[issue](https://github.com/HarpyWar/pvpgn/issues/60#issuecomment-49385463)</sup>
* reorganize unix install paths, add `make uninstall` feature <sup>[issue](https://github.com/HarpyWar/pvpgn/pull/80)</sup>


Build source code
--

#### Windows
Use [Magic Builder](https://github.com/HarpyWar/pvpgn-magic-builder). 

#### Linux
[Русский](http://harpywar.com/?a=articles&b=2&c=1&d=74) | [English](http://harpywar.com/?a=articles&b=2&c=1&d=74&lang=en)

[![Vimeo](http://habrastorage.org/storage3/48c/5a9/4b1/48c5a94b1173242e311f8376be80a585.png)](https://vimeo.com/83763862)
