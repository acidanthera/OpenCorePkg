#!/bin/bash

abort() {
  echo "$1"
  exit 1
}

if [ ! -x "KextInject" ]; then
  abort "Please compile KextInject"
fi

if [ ! -d "$1" ]; then
  abort "Please pass a directory with caches!"
fi

ARCH=x86_64
DIR="$1"
CODE=0
shift

for dir in "${DIR}"/*/ ; do
  for file in kernelcache prelinkedkernel immutablekernel BootKernelExtensions.kc; do
    for resulting in ${file} ${file}_unpack ${file}_${ARCH} ${file}_${ARCH}_unpack; do
      if [ ! -f "${dir}${resulting}" ]; then
        continue
      fi

      printf "%s%s - " "${dir}" "${resulting}"

      ok=true
      echo "./KextInject ${dir}${resulting} ${*}" > "${dir}${resulting}.log"
      ./KextInject "${dir}${resulting}" "${@}" >> "${dir}${resulting}.log" || ok=false

      if $ok; then
        echo "OK"
      else
        echo "FAIL"
        CODE=-1
      fi
    done
  done
done

exit $CODE
