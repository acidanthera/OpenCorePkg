#!/bin/sh

#
# Copyright © 2020 Rodion Shingarev. All rights reserved.
# Slight optimizations by PMheart and vit9696.
#

if [ "$1" = "install" ]; then
  SELFNAME=$(basename "$0")
  SELFDIR=$(dirname "$0")
  cd "$SELFDIR" || exit 1
  sudo defaults write com.apple.loginwindow LogoutHook "$(pwd)/${SELFNAME}"
  exit 0
fi

if [ ! -x /usr/bin/dirname ] || [ ! -x /usr/sbin/nvram ] || [ ! -x /usr/bin/grep ] || [ ! -x /bin/chmod ] || [ ! -x /usr/bin/sed ] || [ ! -x /usr/bin/base64 ] || [ ! -x /bin/rm ] || [ ! -x /bin/mkdir ]  || [ ! -x /usr/bin/stat ] || [ ! -x /usr/libexec/PlistBuddy ] || [ ! -x /usr/sbin/ioreg ] || [ ! -x /usr/bin/xxd ] || [ ! -x /usr/sbin/diskutil ] || [ ! -x /bin/cp ] || [ ! -x /usr/bin/wc ] || [ ! -x /usr/bin/uuidgen ] || [ ! -x /usr/bin/hexdump ]; then
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
if  ! "${nvram}" -x '4D1FDA02-38C7-4A6A-9CC6-4BCCA8B30102:boot-path' | grep -q 'xml' ; then
  nvram="$(pwd)/nvram.mojave"
  if [ ! -f "${nvram}" ]; then
    abort "${nvram} does NOT exist!"
  elif [ ! -x "${nvram}" ]; then
    abort "${nvram} is not executable!"
  fi
fi

getKey() {
  k="$1"
  "${nvram}" -x "${k}" | grep -v '[<>]' | /usr/bin/base64 --decode
}

/bin/rm -rf "${uuidDump}"
/bin/mkdir "${uuidDump}" || abort "Failed to create dump directory!"
cd "${uuidDump}"         || abort "Failed to enter dump directory!"

"${nvram}" -xp > ./nvram1.plist || abort "Failed to dump nvram!"


for key in BootOrder BootNext Boot0080 Boot0081 Boot0082 Boot0083; do
  getKey "8BE4DF61-93CA-11D2-AA0D-00E098032B8C:${key}" > "${key}"
  if [ -n "$(/usr/bin/hexdump "${key}" )" ]; then
    /usr/libexec/PlistBuddy -c "Import Add:8BE4DF61-93CA-11D2-AA0D-00E098032B8C:${key} ${key}" ./nvram.plist || abort "Failed to import ${key} from 8BE4DF61-93CA-11D2-AA0D-00E098032B8C!"
  fi
done
# not an error
# shellcheck disable=SC2043
for key in DefaultBackgroundColor; do
  getKey "4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14:${key}" > "${key}"
    if [ -n "$(/usr/bin/hexdump "${key}" )" ]; then
      /usr/libexec/PlistBuddy -c "Import Add:4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14:${key} ${key}" ./nvram.plist || abort "Failed to import ${key} from 4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14!"
    fi
done

# Optional for security reasons: Wi-Fi settings for Install OS X and Recovery
# for key in current-network preferred-count; do
#   getKey "36C28AB5-6566-4C50-9EBD-CBB920F83843:${key}" > "${key}"
#   if [ -n "$(/usr/bin/hexdump "${key}" )" ]; then
#     /usr/libexec/PlistBuddy -c "Import Add:36C28AB5-6566-4C50-9EBD-CBB920F83843:${key} ${key}" ./nvram.plist || abort "Failed to import ${key} from 36C28AB5-6566-4C50-9EBD-CBB920F83843!"
#   fi
# done

/usr/libexec/PlistBuddy -c "Add Version integer 1"                                       ./nvram.plist || abort "Failed to add Version!"
/usr/libexec/PlistBuddy -c "Add Add:7C436110-AB2A-4BBB-A880-FE41995C9F82 dict"           ./nvram.plist || abort "Failed to add dict 7C436110-AB2A-4BBB-A880-FE41995C9F82"
/usr/libexec/PlistBuddy -c "Merge nvram1.plist Add:7C436110-AB2A-4BBB-A880-FE41995C9F82" ./nvram.plist || abort "Failed to merge with nvram1.plist!"

UUID="$("${nvram}" 4D1FDA02-38C7-4A6A-9CC6-4BCCA8B30102:boot-path | /usr/bin/sed 's/.*GPT,\([^,]*\),.*/\1/')"
if [ "$(printf '%s' "${UUID}" | /usr/bin/wc -c)" -eq 36 ] && [ -z "$(echo "${UUID}" | /usr/bin/sed 's/[-0-9A-F]//g')" ]; then
  /usr/sbin/diskutil mount "${UUID}" || abort "Failed to mount ${UUID}!"
  /bin/cp ./nvram.plist "$(/usr/sbin/diskutil info "${UUID}" | /usr/bin/sed -n 's/.*Mount Point: *//p')" || abort "Failed to copy nvram.plist!"
  /usr/sbin/diskutil unmount "${UUID}" || abort "Failed to unmount ${UUID}!"
  /bin/rm -rf "${uuidDump}"
  exit 0
else
  abort "Illegal UUID or unknown loader!"
fi

/bin/rm -rf "${uuidDump}"
