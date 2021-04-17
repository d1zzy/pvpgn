# VersionCheck

### Configuration File
The configuration file is in standard JSON format and consists of arrays for each Battle.net game client whose names are a four-letter string.

Within the game client arrays are arrays for platform whose names are also a four-letter string (i.e. `IX86`, `PMAC`, `XMAC`).

Within the platform arrays are arrays for the game client's version ID, traditionally known as a "version byte". The version ID is traditionally written in hex format, as indicated with a preceding "0x" or "0X", but can also be in decimal format. The version ID array consists of two pairs, `checkRevisionFile` and `equation`,  and an array, `entries`.

The `entries` array consists of five pairs: `title`, `version`, `hash`, `fileMetadata`, `versionTag`.
- `title`: This is to assist the reader of the configuration file, it does not affect the entry in any way.
- `version`: The version number returned by CheckRevision which is obtained from the [VERSIONINFO](https://msdn.microsoft.com/en-us/library/aa381058) resource of the game file. See sample implementation: https://github.com/pvpgn/CheckRevision
- `hash`: The hash returned by CheckRevision which uses up to three files from the game to produce the hash. See sample implementation: https://github.com/pvpgn/CheckRevision
- `fileMetadata`: The string returned by CheckRevision which consists of the game's filename, last modified date, last modified time, and filesize, all separated by one space (e.g. `war3.exe 08/16/09 19:21:59 471040`). See sample implementation: https://github.com/pvpgn/CheckRevision
	- Note: This pair is currently unused by PvPGN, but may be used in the future.
- `versionTag`: An arbitrary string that must be unique from all other version tags. It is traditionally in the form of the four-letter game string, followed by an underscore and the version (e.g. `WAR3_1282` is used for WarCraft 3: Reign of Chaos 1.28.2).