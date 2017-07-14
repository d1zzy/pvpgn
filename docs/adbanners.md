# Ad Banners
---

### Configuration File
The configuration file consists of a single array named ````ads```` which can contain an unlimited number of elements. Each element contains 4 pairs in the following order: ````filename````, ````url````, ````client````, and ````lang````.
- ````filename````: A string containing the filename of the ad banner, should not include a path.
- ````url````: A string containing the URL that users should be directed to when clicking on the ad banner.
- ````client````: A string containing the 4 character client tag that the ad banner should be shown to, a string of "NULL" will cause the ad banner to be shown to any client.
- ````lang````: A string containing a 4 character language code that will be displayed to users who have enabled that particular language, a string of "NULL" will cause the ad banner to be shown to any user.
    - Valid ````lang```` tags: ````enUS````, ````deDE````, ````csCZ````, ````esES````, ````frFR````, ````itIT````, ````jaJA````, ````koKR````, ````plPL````, ````ruRU````, ````zhCN````, ````zhTW````, ````NULL````.

### File Formats
| Client     | Banner Format |
|------------|---------------|
| StarCraft, Warcraft 2, Diablo  | PCX, SMK      |
| Diablo 2   | SMK             |
| WarCraft 3 | MNG, PNG |

### Banner Dimensions
- The dimensions for ad banners are **468 x 60** pixels

### How To Create SMK Files
1. Download the [Old Smacker Tools](http://files.campaigncreations.org/resources/sc/programs/RADTools.zip)
2. Run *smackerw.exe* and then click on the *Smack (compress) a graphics file.*
3. On the left side, navigate to the folder where your ad banner is stored and select it.
4. Under *Options*, click on the *Palette* tab. Check *Total palette colors to use:*, set it to *64* and check *Starting palette index to use:*, set it to *32*.
5. Set *8-bit input palettes* to *Create new.*
5. Click on the *Frame* tab and check *Create ring?*
6. *Smack!*

### Notes
- It is not known how to create PCX files that will display properly in PvPGN