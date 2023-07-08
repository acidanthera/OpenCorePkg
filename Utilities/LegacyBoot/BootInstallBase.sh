#!/bin/bash

# Install booter on physical disk.

cd "$(dirname "$0")" || exit 1

if [ ! -f "boot${ARCHS}${DUET_SUFFIX}" ] || [ ! -f boot0 ] || [ ! -f boot1f32 ]; then
  echo "Boot files are missing from this package!"
  echo "You probably forgot to build DuetPkg first."
  exit 1
fi

if [ "$(uname)" = "Linux" ]; then
  if [ "$EUID" -ne 0 ]
    then echo "Please run this script as root"
    exit
  fi
  if [ "$(which lsblk)" = "" ]; then
    echo "lsblk tool is missing! Try installing util-linux package"
    exit 1
  fi
  if [ "$(which fdisk)" = "" ]; then
    echo "fdisk tool is missing!"
    exit 1
  fi

  rm -f newbs origbs

  echo "Select the disk where you want to install boot files (${ARCHS}${DUET_SUFFIX}):"
  lsblk -d | tail -n+2 | cut -d" " -f1
  echo "Example: sda"
  read -r DRIVE

  DRIVE="/dev/${DRIVE}"

  if ! lsblk "$DRIVE"; then
    echo Disk "${DRIVE}" not found
    exit 1
  fi

  echo "Choose EFI partition on selected disk:"
  lsblk -f "${DRIVE}"
  echo "Example: sda1"
  read -r EFI_PART

  EFI_PART="/dev/${EFI_PART}"

  if ! lsblk -f "$EFI_PART" | grep -q -e FAT32 -e vfat; then
    echo "No FAT32 partition to install"
    exit 1
  fi

  # Write MBR
  dd if=boot0 of="$DRIVE" bs=1 count=446 conv=notrunc || exit 1

  umount "${EFI_PART}"

  dd if="${EFI_PART}" count=1 of=origbs
  cp -v boot1f32 newbs
  dd if=origbs of=newbs skip=3 seek=3 bs=1 count=87 conv=notrunc
  dd if=/dev/random of=newbs skip=496 seek=496 bs=1 count=14 conv=notrunc
  dd if=newbs of="${EFI_PART}"

  p=/tmp/$(uuidgen)/EFI
  mkdir -p "${p}" || exit 1
  mount -t vfat "${EFI_PART}" "${p}" -o rw,noatime,uid="$(id -u)",gid="$(id -g)" || exit 1

  cp -v "boot${ARCHS}${DUET_SUFFIX}" "${p}/boot" || exit 1

  echo Check "${p}" boot drive EFI folder to install OpenCorePkg

  DISK_SCHEME=$(fdisk -l "${DRIVE}" | sed -n 's/.*Disklabel type: *//p')
  if [ "$DISK_SCHEME" != "gpt" ]; then
    BOOT_FLAG=$(dd if="$DRIVE" bs=1 count=1 status=none skip=$((0x1BE)) | od -t x1 -A n | tr -d ' ')
    if [ "$BOOT_FLAG" != "80" ]; then
      fdisk "$DRIVE" <<END
p
a
1
w
END
    fi
  fi
else
  rm -f newbs origbs

  diskutil list
  echo "Disable SIP in the case of any problems with installation!!!"
  echo "Enter disk number to install OpenDuet (${ARCHS}${DUET_SUFFIX}) to:"
  read -r N

  if ! diskutil info disk"${N}" |  grep -q "/dev/disk"; then
    echo Disk "$N" not found
    exit 1
  fi

  if ! diskutil info disk"${N}"s1 | grep -q -e FAT_32 -e EFI; then
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
  if sudo diskutil mount disk"${N}"s1 | grep -q mounted; then
    cp -v "boot${ARCHS}${DUET_SUFFIX}" "$(diskutil info  disk"${N}"s1 |  sed -n 's/.*Mount Point: *//p')/boot"
  else
    p=/tmp/$(uuidgen)/EFI
    mkdir -p "${p}" || exit 1
    sudo mount_msdos /dev/disk"${N}"s1 "${p}" || exit 1
    cp -v "boot${ARCHS}${DUET_SUFFIX}" "${p}/boot" || exit 1
    open "${p}"
  fi

  if diskutil info  disk"${N}" |  grep -q FDisk_partition_scheme; then
    sudo fdisk -e /dev/rdisk"$N" <<-MAKEACTIVE
p
f 1
w
y
q
MAKEACTIVE
  fi
fi
