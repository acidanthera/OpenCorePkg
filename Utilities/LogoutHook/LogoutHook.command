#!/bin/sh

#
# Copyright © 2019 Rodion Shingarev. All rights reserved.
# Slight optimizations by PMheart and vit9696.
#

if [ ! -x /usr/bin/dirname ] || [ ! -x /usr/sbin/nvram ] || [ ! -x /usr/bin/grep ] || [ ! -x /bin/chmod ] || [ ! -x /usr/bin/sed ] || [ ! -x /usr/bin/base64 ] || [ ! -x /bin/rm ] || [ ! -x /bin/mkdir ] || [ ! -x /bin/cat ] || [ ! -x /bin/dd ] || [ ! -x /usr/bin/stat ] || [ ! -x /usr/libexec/PlistBuddy ] || [ ! -x /usr/sbin/ioreg ] || [ ! -x /usr/bin/xxd ] || [ ! -x /usr/sbin/diskutil ] || [ ! -x /bin/cp ] || [ ! -x /usr/bin/wc ] || [ ! -x /usr/bin/uuidgen ]; then
  abort "Unix environment is broken!"
fi

thisDir="$(/usr/bin/dirname "${0}")"
uuidDump="${thisDir}/$(/usr/bin/uuidgen)"
if [ "${thisDir}/" = "${uuidDump}" ]; then
  echo "uuidgen returns null!"
  exit 1
fi
cd "${thisDir}" || abort "Failed to enter working directory!"

abort() {
  echo "Fatal error: ${1}"
  /bin/rm -rf "${uuidDump}"
  exit 1
}

nvram=/usr/sbin/nvram
# FIXME: find an nvram key that is mandatory
if [ -z "$("${nvram}" -x '4D1FDA02-38C7-4A6A-9CC6-4BCCA8B30102:boot-path' | /usr/bin/grep 'xml')" ]; then
  nvram="$(pwd)/nvram.mojave"
  if [ ! -f "${nvram}" ]; then
    abort "${nvram} does NOT exist!"
  elif [ ! -x "${nvram}" ]; then
    abort "${nvram} is not executable!"
  fi
fi

getKey() {
  local key="$1"
  "${nvram}" -x "${key}" | /usr/bin/sed '/\<data\>/,/\<\/data\>/!d;//d' | /usr/bin/base64 --decode
}

/bin/rm -rf "${uuidDump}"
/bin/mkdir "${uuidDump}" || abort "Failed to create dump directory!"
cd "${uuidDump}"         || abort "Failed to enter dump directory!"

"${nvram}" -xp > ./nvram1.plist || abort "Failed to dump nvram!"

getKey '8BE4DF61-93CA-11D2-AA0D-00E098032B8C:Boot0080' > ./Boot0080
if [ ! -z "$(/bin/cat ./Boot0080)" ]; then
  getKey 'efi-boot-device-data' > efi-boot-device-data || abort "Failed to retrieve efi-boot-device-data!"
  /bin/dd seek=24 if=efi-boot-device-data of=Boot0080 bs=1 count=$(/usr/bin/stat -f%z efi-boot-device-data)    || abort "Failed to fill Boot0080 with efi-boot-device-data!"
  /usr/libexec/PlistBuddy -c "Import Add:8BE4DF61-93CA-11D2-AA0D-00E098032B8C:Boot0080 Boot0080" ./nvram.plist || abort "Failed to import Boot0080!"
fi

for key in BootOrder BootCurrent BootNext Boot008{1..3}; do
  getKey "8BE4DF61-93CA-11D2-AA0D-00E098032B8C:${key}" > "${key}"
  if [ ! -z "$(/bin/cat "${key}")" ]; then
    /usr/libexec/PlistBuddy -c "Import Add:8BE4DF61-93CA-11D2-AA0D-00E098032B8C:${key} ${key}" ./nvram.plist || abort "Failed to import ${key} from 8BE4DF61-93CA-11D2-AA0D-00E098032B8C!"
  fi
done

/usr/libexec/PlistBuddy -c "Add Version integer 1"                                       ./nvram.plist || abort "Failed to add Version!"
/usr/libexec/PlistBuddy -c "Add Add:7C436110-AB2A-4BBB-A880-FE41995C9F82 dict"           ./nvram.plist || abort "Failed to add dict 7C436110-AB2A-4BBB-A880-FE41995C9F82"
/usr/libexec/PlistBuddy -c "Merge nvram1.plist Add:7C436110-AB2A-4BBB-A880-FE41995C9F82" ./nvram.plist || abort "Failed to merge with nvram1.plist!"

UUID="$("${nvram}" 4D1FDA02-38C7-4A6A-9CC6-4BCCA8B30102:boot-path | /usr/bin/sed 's/.*GPT,\([^,]*\),.*/\1/')"
if [ "$(printf "${UUID}" | /usr/bin/wc -c)" -eq 36 ] && [ -z "$(echo "${UUID}" | /usr/bin/sed 's/[-0-9A-F]//g')" ]; then
  /usr/sbin/diskutil mount "${UUID}" || abort "Failed to mount ${UUID}!"
  /bin/cp ./nvram.plist "$(/usr/sbin/diskutil info "${UUID}" | /usr/bin/sed -n 's/.*Mount Point: *//p')" || abort "Failed to copy nvram.plist!"
  /usr/sbin/diskutil unmount "${UUID}" || abort "Failed to unmount ${UUID}!"
  /bin/rm -rf "${uuidDump}"
  exit 0
else
  abort "Illegal UUID or unknown loader!"
fi

/bin/rm -rf "${uuidDump}"
