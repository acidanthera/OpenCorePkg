Apple Mac Serial Format
=======================

It is reasonably important to get more information about the goods you buy, especially if they are not new, and you do not have absolute confidence in the seller. Serial numbers are the first thing to look at. For Apple products [Apple Check Coverage](https://checkcoverage.apple.com) is your best friend.

However, it does not show all the details encoded in the serial, and in some case it may be important. For example, certain shady dealers may change one valid serial by the other, and it will not be obvious at a glance that the serial does not belong to the actual model. This FAQ attempts to explain the reverse-engineered structure of the serials used in Apple hardware.

You could always receive information about the current serial number of your Mac by running `./macserial`. For the other serial use `./macserial -i SERIALNUMBER`, where `SERIALNUMBER` is the serial you check.

## Apple base 34

Select fields in the numbers are encoded values in base 34. So, certain alpha-numeric characters represent a slightly uncommon base 34 code excluding `O` and `I`.

| Char | Value | Char | Value |
| ---- | ----- | ---- | ----- |
| `0`  | `0`   | `H`  | `17`  |
| `1`  | `1`   | `J`  | `18`  |
| `2`  | `2`   | `K`  | `19`  |
| `3`  | `3`   | `L`  | `20`  |
| `4`  | `4`   | `M`  | `21`  |
| `5`  | `5`   | `N`  | `22`  |
| `6`  | `6`   | `P`  | `23`  |
| `7`  | `7`   | `Q`  | `24`  |
| `8`  | `8`   | `R`  | `25`  |
| `9`  | `9`   | `S`  | `26`  |
| `A`  | `10`  | `T`  | `27`  |
| `B`  | `11`  | `U`  | `28`  |
| `C`  | `12`  | `V`  | `29`  |
| `D`  | `13`  | `W`  | `30`  |
| `E`  | `14`  | `X`  | `31`  |
| `F`  | `15`  | `Y`  | `32`  |
| `G`  | `16`  | `Z`  | `33`  |

## Serial number (SN)

There generally are 2 similar formats of serial encoding: the old 11 character format, and the new 12 character format. 

| Type      | Location  | Year | Week | Line  | Product  |
| --------- | --------- | ---- | ---- | ----- | -------- |
| Old (11)  | `LL`      | `Y`  | `WW` | `SSS` | `PPP`    |
| New (12)  | `LLL`     | `Y`  | `W`  | `SSS` | `PPPP`   |

Note: Models late 2021+ contain SN with 10 character format.  

### Location

This value encodes the manufacturing location, which is often more descriptive than `Made in China`, since it may reveal the responsible company and the city. For example, `F5K` means `USA (Flextronics)` and `QT` means `Taiwan (Quanta Computer)`. The list is not standardised or published anywhere, but you can see several known locations by running `./macserial -l`.

One of the important locations for old-style serials (11 characters) is `RM`. It means that the model was refurbished. For new-style serials you have to call [Apple support](https://support.apple.com) to know this.

### Year

Year encodes the actual manufacturing year of each model. For refurbished models it is unknown whether it is replaced by the remanufacturing year.

For old-style serials it always is a digit that encodes the last digit of the year. For example, `8` means 2008 and `1` means 2011. Only `0` to `9` digitis are used for year encoding. Old-style serials are out of use starting with 2013, so `3` means 2003 not 2013.

| Char | Year |
| ---- | ---- |
| `3`  | 2003 |
| `4`  | 2004 |
| `5`  | 2005 |
| `6`  | 2006 |
| `7`  | 2007 |
| `8`  | 2008 |
| `9`  | 2009 |
| `0`  | 2010 |
| `1`  | 2011 |
| `2`  | 2012 |


For new-style serials it is an alphanumeric value, which not only encodes the year, but its half as well. Not all the values are allowed. The table below outlines the pairs of characters which are assumed to encode each supported year. First character in the pair is believed to encode the first half of the year, and the second character — the second half.

| Pair     | Year |
| -------- | ---- |
| `C`, `D` | 2010 |
| `F`, `G` | 2011 |
| `H`, `J` | 2012 |
| `K`, `L` | 2013 |
| `M`, `N` | 2014 |
| `P`, `Q` | 2015 |
| `R`, `S` | 2016 |
| `T`, `V` | 2017 |
| `W`, `X` | 2018 |
| `Y`, `Z` | 2019 |
| `C`, `D` | 2020 |
| `F`, `G` | 2021 |

### Week

Week encodes the actual manufacturing week of each model. This week has nothing in common with [ISO 8601](https://en.wikipedia.org/wiki/ISO_week_date), and appears to be encoded literally as 7-day sequences starting from January, 1st. Since each year has either 365 or 366 days, 53rd week is extremely rare, and you are lucky to have such a serial.

For old-style serials week is encoded in plain numeric digits with leading zeroes. `01`, `02`, ... `53`. For new-style serials an alpha-numeric code is used. Encoded year half also counts and means adds 26 weeks for the second one.

| Char | 1st half | 2nd half |
| ---- | -------- | -------- |
| `1`  | `1`      | `27`     |
| `2`  | `2`      | `28`     |
| `3`  | `3`      | `29`     |
| `4`  | `4`      | `30`     |
| `5`  | `5`      | `31`     |
| `6`  | `6`      | `32`     |
| `7`  | `7`      | `33`     |
| `8`  | `8`      | `34`     |
| `9`  | `9`      | `35`     |
| `C`  | `10`     | `36`     |
| `D`  | `11`     | `37`     |
| `F`  | `12`     | `38`     |
| `G`  | `13`     | `39`     |
| `H`  | `14`     | `40`     |
| `J`  | `15`     | `41`     |
| `K`  | `16`     | `42`     |
| `L`  | `17`     | `43`     |
| `M`  | `18`     | `44`     |
| `N`  | `19`     | `45`     |
| `P`  | `20`     | `46`     |
| `Q`  | `21`     | `47`     |
| `R`  | `22`     | `48`     |
| `T`  | `23`     | `49`     |
| `V`  | `24`     | `50`     |
| `W`  | `25`     | `51`     |
| `X`  | `26`     | `52`     |
| `Y`  | `-`      | `53`     |

For old-style serials it is a pair of two digits, which encode the manufacturing week.

### Production line and copy

Production line is believed to be related to some identifier at assembly stage. It is encoded in base 34, but the actual derivation process is unknown and can only be guessed with relative success.

Current model, which apparently works well, represents it as a sum of three alpha-numeric characters with `1`, `34`, and `68` multipliers. The actual formula looks as follows:

```
base34[S1] * 68 + base34[S2] * 34 + base34[S3] = production line
```

This formula effectively defines a compression function, which allows to encode a total of `3400` production lines from `0` to `3399`. The compression produced by shortening `39304` space to `3400` allows multiple encodings of the same line. For example, for `939` line there can be `14` derivatives or "copies": `0TM`, `1RM`, `2PM`, `3MM`, `4KM`, ..., `D1M`.

While the formula does look strange, it was experimentally discovered that up to `N` first encoded derivatives are valid, and starting with the first invalid derivative there will be no valid ones. Thus for a complete serial list made up with all the derivatives from the above the following is assumed to be true: if `0TM` and `2PM` are valid and `3MM` is invalid, then `1RM` will also be valid, and `4KM` to `D1M` will be invalid. From this data it could be theorised that the encoded value is incremented for each model produced from the same line. So `0TM` is the first copy produced, and `D1M` is the last copy.

**Update**: At a later stage very few examples of valid derivatives after invalid were found. These exceptions disprove at least some parts of the model, but currently there exists no better theory.

### Product model

Last 3 (for legacy serials) or 4 (for new serials) symbols encode the actual product identifier of this exact piece of the hardware. This is probably the most useful part of the serial, since it allows you to get the detailed description of your hardware directly from the dedicated Apple Specs portal. To do so you need to modify the following URI to contain your real product code instead of `PPPP` and follow it in your browser:

```
http://support-sp.apple.com/sp/index?page=cpuspec&cc=PPPP
```

For example, for iMacPro1,1 it could be [HX87](http://support-sp.apple.com/sp/index?page=cpuspec&cc=HX87) and for MacBookPro14,3 it could be [HTD5](http://support-sp.apple.com/sp/index?page=cpuspec&cc=HTD5).

The list is not standardised or published anywhere, but you can see most products by running `./macserial -lp` and `./macserial -l` to match against mac models. The value seems to be a classic base 34 sequence: `P1 * 39304 + P2 * 1156 + P3 * 34 + P4`. The ranges seem to be allocated in chunks in non-decreasing manner. Normally each chunk is distanced from another chunk by up to 64 (90% matches). 

## Logic board serial number (MLB)

There generally are 2 formats of logic board serial encoding: the old 13 character format, and the new 17 character format. Unlike serial number, these formats are quite different and in addition very little is known about MLB in general.

| Type      | Location  | Year | Week | Item   | Infix  | Product  | Suffix |
| --------- | --------- | ---- | ---- | ------ | ------ | -------- | ------ |
| Old (13)  | `LL`      | `Y`  | `WW` | `IIII` |        | `EEE`    | `C`    |
| New (17)  | `LLL`     | `Y`  | `WW` | `III`  | `AA`   | `EEEE`   | `CC`   |

While it is unclear if this is intentional, for 17 character MLB it is possible to perform basic validation online through `osrecovery.apple.com`. The recovery server will return valid latest recovery image only when MLB is valid. Use `./macrecovery.py verify -m MLB -b BOARD-ID` to try verifying your MLB number.

It is not clear how strongly MLB is attached to serial number (SN). The following is known:

- Minimal supported macOS version is identified by `EEEE`
- Maximum supported macOS version is identified by `EEEE` and `board-id`
- Recovery server accepts a range of models with the same MLB (with only latest os different)

The following is suspected:
- `EEEE` is unique number for all MLBs
- `EEEE` are shared across different models and thus cannot identify the model

### Location

MLB location is equivalent to serial number location but does not necessarily match it, as logic boards can be manufactured at a different place.

### Year and week

MLB year and week in both 13-character and 17-character MLB are equivalent to legacy serial number year and week. The values are slightly lower as logic board is manufactured prior to the complete product.

### Item

MLB item is encoded differently for 13-character and 17-character MLB. It might serve as a production item per week and could be similar to 'Production line and copy' in the serial number.

- For old MLB, this is a variant of base 34 value. First item character is always `0`.
- For new MLB, this value always is a number.

### Infix

Base 34 value present in new MLBs only. No information is known about it. Could actually be part of Item.

### Product board

Similarly to 'Product model' this field encodes logic board model number. This code is often referred to as `EEE code` in part catalogues and is useful for purchasing a compatible logic board for replacement.

For new 17 character MLBs this field is also used for identification at `osrecovery.apple.com` to provide a compatible internet recovery image and diagnostic tools upon request.

### Suffix

Base 34 value with unclear designation. Might be used for checksum validation. Checksum validation algorithm is reverse engineered from diagnostics tools and is valid for all 17 character MLBs. It is not clear whether 13 character MLBs have any checksum. 17 character MLB checksum follows.

```C
static bool verify_mlb_checksum(const char *mlb, size_t len) {
 const char alphabet[] = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";
 size_t checksum = 0;
 for (size_t i = 0; i < len; ++i) {
   for (size_t j = 0; j <= sizeof (alphabet); ++j) {
     if (j == sizeof (alphabet))
       return false;
     if (mlb[i] == alphabet[j]) {
       checksum += (((i & 1) == (len & 1)) * 2 + 1) * j;
       break;
     }
   }
 }
 return checksum % (sizeof(alphabet) - 1) == 0;
}
```

## Appendix

This information was obtained experimentally and may not be accurate in certain details. Be warned that it is published at no warranty for educational and introductory purposes only.
