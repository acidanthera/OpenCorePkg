#!/bin/bash

# Install booter on physical disk.

cd "$(dirname "$0")" || exit 1

if [ ! -f "boot${ARCHS}" ] || [ ! -f boot0 ] || [ ! -f boot1f32 ]; then
  echo "Boot files are missing from this package!"
  echo "You probably forgot to build DuetPkg first."
  exit 1
fi

diskutil list
echo "Disable SIP in the case of any problems with installation!!!"
echo "Enter disk number to install to:"
read -r N

if ! diskutil info disk"${N}" |  grep -q "/dev/disk"
then
  echo Disk "$N" not found
  exit 1
fi

if ! diskutil info disk"${N}"s1 | grep -q -e FAT_32 -e EFI
then
echo "No FAT32 partition to install"
exit 1
fi

# Write MBR
sudo fdisk -uy -f boot0 /dev/rdisk"${N}" || exit 1

diskutil umount disk"${N}"s1
sudo dd if=/dev/rdisk"${N}"s1 count=1  of=origbs
cp -v boot1f32 newbs
sudo dd if=origbs of=newbs skip=3 seek=3 bs=1 count=87 conv=notrunc
dd if=/dev/random of=newbs skip=496 seek=496 bs=1 count=14 conv=notrunc
sudo dd if=newbs of=/dev/rdisk"${N}"s1
#if [[ "$(sudo diskutil mount disk"${N}"s1)" == *"mounted" ]]
if sudo diskutil mount disk"${N}"s1 | grep -q mounted
then
cp -v "boot${ARCHS}" "$(diskutil info  disk"${N}"s1 |  sed -n 's/.*Mount Point: *//p')/boot"
else
p=/tmp/$(uuidgen)/EFI
mkdir -p "${p}" || exit 1
sudo mount_msdos /dev/disk"${N}"s1 "${p}" || exit 1
cp -v "boot${ARCHS}" "${p}/boot" || exit 1
open "${p}"
fi

if diskutil info  disk"${N}" |  grep -q FDisk_partition_scheme
then
sudo fdisk -e /dev/rdisk"$N" <<-MAKEACTIVE
p
f 1
w
y
q
MAKEACTIVE
fi
