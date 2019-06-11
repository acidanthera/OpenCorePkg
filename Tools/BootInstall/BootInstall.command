#!/bin/bash

cd "$(dirname "$0")"
diskutil list
echo "Enter disk number to install to:"
read N

if [[ ! $(diskutil info disk${N} |  sed -n 's/.*Device Node: *//p') ]]
then
  echo Disk $N not found
  exit
fi

FS=$(diskutil info disk${N}s1 | sed -n 's/.*File System Personality: *//p')
echo $FS

if [ "$FS" != "MS-DOS FAT32" ]
then
  echo "No FAT32 partition to install"
  exit
fi

# Write MBR
sudo fdisk -f boot0af -u /dev/rdisk${N}

diskutil umount disk${N}s1
sudo dd if=/dev/rdisk${N}s1 count=1  of=origbs
cp -v boot1f32 newbs
sudo dd if=origbs of=newbs skip=3 seek=3 bs=1 count=87 conv=notrunc
sudo dd if=newbs of=/dev/rdisk${N}s1
diskutil mount disk${N}s1

cp -v boot "$(diskutil info  disk${N}s1 |  sed -n 's/.*Mount Point: *//p')"

if [ $(diskutil info  disk${N} |  sed -n 's/.*Content (IOContent): *//p') == "FDisk_partition_scheme" ]
then
sudo fdisk -e /dev/rdisk$N <<-MAKEACTIVE
p
f 1
w
y
q
MAKEACTIVE
fi
