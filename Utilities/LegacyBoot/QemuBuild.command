#!/bin/bash

# Build QEMU image, example:
# qemu-system-x86_64 -drive file=$QEMU_IMAGE/OpenCore.RO.raw -serial stdio \
#   -usb -device usb-kbd -device usb-mouse -s -m 8192

cd "$(dirname "$0")" || exit 1

usage () {
  echo "Usage: $(basename "$0") [X64|IA32|EFI]"
  exit 1
}

missing_files () {
  echo "Boot files are missing from this package!"
  echo "You probably forgot to build DuetPkg first."
  exit 1
}

if [ $# -gt 1 ]; then
  usage
elif [ $# -eq 1 ]; then
  if [ "$1" != "X64" ] && [ "$1" != "IA32" ] && [ "$1" != "EFI" ]; then
    usage
  fi
  ARCHS=$1
elif [ "${ARCHS}" = "" ]; then
  usage
fi

if [ "$ARCHS" != "EFI" ]; then
  if [ ! -f "boot${ARCHS}" ] || [ ! -f boot0 ] || [ ! -f boot1f32 ]; then
    missing_files
  fi
fi

if [ "$(which qemu-img)" = "" ]; then
  echo "QEMU installation missing"
  exit 1
fi

if [ ! -d ROOT ]; then
  echo "No ROOT directory with ESP partition contents"
  exit 1
fi

if [ "$(uname)" = "Linux" ]; then
  if [ "$EUID" -ne 0 ]
    then echo "Please run this script as root"
    exit
  fi
  if [ "$(which qemu-nbd)" = "" ]; then
    echo "Your QEMU installation doesn't contain qemu-nbd tool!"
    exit 1
  fi
  if [ "$(which mkfs.vfat)" = "" ]; then
    echo "mkfs.vfat not found, dosfstools package is missing?"
    exit 1
  fi
  if [ "$(which fdisk)" = "" ]; then
    echo "fdisk tool is missing!"
    exit 1
  fi
  IMAGE=OpenCore.raw
  DIR="$IMAGE.d"
  NBD=/dev/nbd0

  rm -rf "$DIR"
  rm -f "$IMAGE" newbs origbs
  mkdir -p "$DIR"

  # Create 200M MS-DOS bootable disk image with 1 FAT32 partition
  modprobe nbd
  qemu-img create -f raw "$IMAGE" 200M
  fdisk "$IMAGE" << END
o
n
p
1
2048
409599
t
b
a
w
END
  qemu-nbd -f raw --connect="$NBD" "$IMAGE"
  # Wait a it after mounting
  sleep 2
  mkfs.vfat -F32 ${NBD}p1

  if [ "$ARCHS" != "EFI" ]; then
    # Copy boot1f32 into FAT32 Boot Record
    dd if=${NBD}p1 of=origbs count=1
    cp -v boot1f32 newbs
    dd if=origbs of=newbs skip=3 seek=3 bs=1 count=87 conv=notrunc
    dd if=/dev/random of=newbs skip=496 seek=496 bs=1 count=14 conv=notrunc
    dd if=newbs of=${NBD}p1 conv=notrunc
  fi

  mount -t vfat ${NBD}p1 "$DIR" -o rw,noatime,uid="$(id -u)",gid="$(id -g)"
  sleep 2

  if [ "$ARCHS" != "EFI" ]; then
    # Copy boot file into FAT32 file system
    cp -v "boot${ARCHS}" "$DIR/boot"
  fi

  # Copy ESP contents into FAT32 file system
  cp -rv ROOT/* "$DIR"

  # Remove temporary files
  sleep 2
  umount -R "$DIR"
  qemu-nbd -d "$NBD"
  rm -r "$DIR"
  rm newbs origbs

  if [ "$ARCHS" != "EFI" ]; then
    # Copy boot0 into MBR
    dd if=boot0 of="$IMAGE" bs=1 count=446 conv=notrunc
  fi

  chown "$(whoami)" "$IMAGE"
elif [ "$(uname)" = "Darwin" ]; then
  rm -f OpenCore.dmg.sparseimage OpenCore.RO.raw OpenCore.RO.dmg
  hdiutil create -size 200m  -layout "UNIVERSAL HD"  -type SPARSE  -o OpenCore.dmg
  newDevice=$(hdiutil attach -nomount OpenCore.dmg.sparseimage |head -n 1 |  awk  '{print $1}')
  echo newdevice "$newDevice"

  diskutil partitionDisk "${newDevice}" 1 MBR fat32 TEST R

  # boot install script
  diskutil list
  N=$(echo "$newDevice" | tr -dc '0-9')
  echo "Will be installed to Disk ${N}"


  if [[ ! $(diskutil info disk"${N}" |  sed -n 's/.*Device Node: *//p') ]]
  then
    echo Disk "$N" not found
    exit 1
  fi

  FS=$(diskutil info disk"${N}"s1 | sed -n 's/.*File System Personality: *//p')
  echo "$FS"

  if [ "$FS" != "MS-DOS FAT32" ]
  then
    echo "No FAT32 partition to install"
    exit 1
  fi

  if [ "$ARCHS" != "EFI" ]; then
    # Write MBR
    sudo fdisk -f boot0 -u /dev/rdisk"${N}"

    # Write boot1f32
    diskutil umount disk"${N}"s1
    sudo dd if=/dev/rdisk"${N}"s1 count=1  of=origbs
    cp -v boot1f32 newbs
    sudo dd if=origbs of=newbs skip=3 seek=3 bs=1 count=87 conv=notrunc
    dd if=/dev/random of=newbs skip=496 seek=496 bs=1 count=14 conv=notrunc
    sudo dd if=newbs of=/dev/rdisk"${N}"s1
    diskutil mount disk"${N}"s1

    # Copy boot
    cp -v "boot${ARCHS}" "$(diskutil info  disk"${N}"s1 |  sed -n 's/.*Mount Point: *//p')/boot"
  fi

  cp -rv ROOT/* "$(diskutil info  disk"${N}"s1 |  sed -n 's/.*Mount Point: *//p')"

  if [ "$ARCHS" != "EFI" ] && [ "$(diskutil info  disk"${N}" |  sed -n 's/.*Content (IOContent): *//p')" == "FDisk_partition_scheme" ]; then
  sudo fdisk -e /dev/rdisk"$N" <<-MAKEACTIVE
p
f 1
w
y
q
MAKEACTIVE
  fi

  hdiutil detach "$newDevice"
  hdiutil convert -format UDRO OpenCore.dmg.sparseimage -o OpenCore.RO.dmg
  qemu-img convert -f dmg -O raw OpenCore.RO.dmg OpenCore.RO.raw
fi
