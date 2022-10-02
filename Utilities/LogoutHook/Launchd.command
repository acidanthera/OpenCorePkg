#!/bin/bash

#
# Copyright © 2022 Mike Beaton. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
# Examine log at /var/log/org.acidanthera.nvramhook.launchd/launchd.log.
#
# Script can run immediately or be installed as daemon or logout hook. Installs as
# launch daemon on Yosemite and above, which is the only supported approach that on
# shutdown reads NVRAM after macOS installer vars are set, in more recent macOS
# installers (e.g. Monterey).
#
# Non-installed script can be run directly as 'daemon' or 'logout' hook by
# specifying one of those options on the command line. For 'logout' this saves NVRAM
# immediately, for 'daemon' this waits to be terminated e.g. CTRL+C before saving.
#
# TO DO: rc.shutdown.d approach discussed in
# https://github.com/acidanthera/bugtracker/issues/2106
# might be a viable alternative (needs testing re timing of picking up NVRAM changes).
#
# Credits:
#  - Rodion Shingarev (with additional contributions by vit9696 and PMheart) for
#    original LogoutHook.command
#  - Rodion Shingarev for nvramdump command
#  - Mike Beaton for this script
#

usage() {
  echo "Usage: ${SELFNAME} [install|uninstall|status] [logout|daemon]"
  echo "  - [install|uninstall] without type uses recommended type for macOS version"
  echo "  - 'status' shows status of daemon and logout hook"
  echo "  - 'status daemon' gives additional detail on daemon"
  echo ""
}

doLog() {
  # 'sudo tee' to write to log file as root (could also 'sh -c') for installation steps;
  # will be root anyway when running installed as daemon or logout hook
  if [ ! "${LOG_PREFIX}" = "" ] ; then
    sudo tee -a "${LOGFILE}" > /dev/null << EOF
$(date +"${DATEFORMAT}") (${LOG_PREFIX}) ${1}
EOF
  else
    sudo tee -a "${LOGFILE}" > /dev/null << EOF
${1}
EOF
  fi

  if [ ! "$INSTALLED" = "1" ] ; then
    echo "${1}"
  fi
}

abort() {
  doLog "Fatal error: ${1}"
  exit 1
}

# abort without logging or requiring sudo
earlyAbort() {
  echo "Fatal error: ${1}" > /dev/stderr
  exit 1
}

getDarwinMajorVersion() {
  darwin_ver=$(uname -r)
  # Cannot add -r in earlier macOS
  # shellcheck disable=SC2162
  IFS="." read -a darwin_ver_array <<< "$darwin_ver"
  echo "${darwin_ver_array[0]}"
}

installLog() {
  FAIL="Failed to install log!"
  if [ ! -d "${LOGDIR}" ] ; then
    sudo mkdir "${LOGDIR}" || abort "${FAIL}"
  fi

  if [ ! -f "${LOGFILE}" ] ; then
    sudo touch "${LOGFILE}" || abort "${FAIL}"
  fi
}

install() {
  FAIL="Failed to install!"

  if [ ! -d "${PRIVILEGED_HELPER_TOOLS}" ] ; then
    sudo mkdir "${PRIVILEGED_HELPER_TOOLS}" || abort "${FAIL}"
  fi

  # Copy executable files into place.
  sudo cp "${SELFNAME}" "${HELPER}" || abort "${FAIL}"
  sudo cp nvramdump "${NVRAMDUMP}"  || abort "${FAIL}"

  # Install logout hook.
  if [ "$LOGOUT" = "1" ] ; then
    sudo defaults write com.apple.loginwindow LogoutHook "${HELPER}" || abort "${FAIL}"
  fi

  # Make customised Launchd.command.plist for daemon.
  if [ "$DAEMON" = "1" ] ; then
    sed "s/\$LABEL/${ORG}.daemon/g;s/\$HELPER/$(sed 's/\//\\\//g' <<< "$HELPER")/g;s/\$PARAM/daemon/g;s/\$LOGFILE/$(sed 's/\//\\\//g' <<< "$LOGFILE")/g" "Launchd.command.plist" > "/tmp/Launchd.command.plist" || abort "${FAIL}"
    sudo cp "/tmp/Launchd.command.plist" "${DAEMON_PLIST}" || abort "${FAIL}"
    rm -f /tmp/Launchd.command.plist
  fi

  # Launch already installed daemon.
  if [ "$DAEMON" = "1" ] ; then
    sudo launchctl load "${DAEMON_PLIST}" || abort "${FAIL}"
  fi

  echo "Installed."
}

# On older macOS, previous version of this script would install and uninstall
# harmless (i.e. takes no actions) agent script if requested, but fail to report
# it as installed, and fail to start it immediately, so clean up.
#
# Working agent (probably never much use) requires:
#  - chmod for logfile access
#  - non-sudo log writer (agent is not sudo and cannot gain it)
#  - customised agent.plist
#  - load with launchctl load (earlier macOS) or sudo launchctl bootstrap
#  - unload with launchctl unload (earlier macOS) or sudo launchctl bootout
#
uninstallAgent() {
  if [ -f "${AGENT_PLIST}" ] ; then
    sudo rm -f "${AGENT_PLIST}"
    echo "Uninstalled agent."
  fi
}

uninstall() {
  UNINSTALLED=1

  if [ "$DAEMON" = "1" ] ; then
    if [ "$DAEMON_PID" = "" ] ; then
      UNINSTALLED=2
      echo "Daemon was not installed!" > /dev/stderr
    else
      # Place special value in saved device node so that nvram.plist is not updated at uninstall
      sudo /usr/sbin/nvram "${BOOT_NODE}=null" || abort "Failed to save null boot device!"
      sudo launchctl unload "${DAEMON_PLIST}" || UNINSTALLED=0
      DAEMON_PID=
    fi
    sudo rm -f "${DAEMON_PLIST}"
  elif [ "$LOGOUT" = "1" ] ; then
    if [ "$LOGOUT_HOOK" = "" ] ; then
      UNINSTALLED=2
      echo "Logout hook was not installed!" > /dev/stderr
    else
      sudo defaults delete com.apple.loginwindow LogoutHook || UNINSTALLED=0
      LOGOUT_HOOK=
    fi
  fi

  if [ "$DAEMON_PID"  = "" ] &&
     [ "$LOGOUT_HOOK" = "" ] ; then
    sudo rm -f "${HELPER}"
    sudo rm -f "${NVRAMDUMP}"
  fi

  if [ "$UNINSTALLED" = "0" ] ; then
    echo "Could not uninstall!"
  elif [ "$UNINSTALLED" = "1" ] ; then
    echo "Uninstalled."
  fi
}

status() {
  if [ ! "$DAEMON" = "1" ] ; then
    echo "Daemon pid = ${DAEMON_PID}"
    echo "LogoutHook = ${LOGOUT_HOOK}"
  else
    # Detailed info on daemon.
    if [ "$DAEMON" = "1" ] ; then
      sudo launchctl print "system/${ORG}.daemon"
    fi
  fi
}

# Save some diskutil info in emulated NVRAM for use at daemon shutdown:
#  - While we can access diskutil normally at agent startup and at logout hook;
#  - We cannot use diskutil at daemon shutdown, because:
#     "Unable to run because unable to use the DiskManagement framework.
#     Common reasons include, but are not limited to, the DiskArbitration
#     framework being unavailable due to being booted in single-user mode."
#  - At daemon startup, diskutil works but the device may not be ready
#    immediately, but macOS restarts us quickly (~5s) and then we can run.
# Note that saving any info for use at process shutdown if not running as
# daemon (sudo) would have to go into e.g. a file not nvram.
saveMount() {
  UUID="$(/usr/sbin/nvram "${BOOT_PATH}" | sed 's/.*GPT,\([^,]*\),.*/\1/')"
  if [ "$(printf '%s' "${UUID}" | /usr/bin/wc -c)" -eq 36 ] && [ -z "$(echo "${UUID}" | sed 's/[-0-9A-F]//g')" ] ; then
    node="$(/usr/sbin/diskutil info "${UUID}" | sed -n 's/.*Device Node: *//p')"

    # This may randomly fail initially, if so the script gets restarted by
    # launchd and eventually succeeds.
    if [ "${node}" = "" ] ; then
      abort "Cannot access device node!"
    fi

    doLog "Found boot device at ${node}"

    if [ "${1}" = "1" ] ; then
      # On earlier macOS (at least Mojave in VMWare) there is an intermittent
      # problem where msdos/FAT kext is occasionally not available when we try
      # to mount the drive on daemon exit; mounting and unmounting on daemon
      # start here fixes this.
      # Doing this introduces a different problem, since this early mount sometimes
      # fails initially, however (as with 'diskutil info' above) in that case the
      # script gets restarted by launchd after ~5s, and eventually either succeeds or
      # finds the drive already mounted (when applicable, i.e. not marked as ESP).
      mount_path=$(mount | sed -n "s:${node} on \(.*\) (.*$:\1:p")
      if [ ! "${mount_path}" = "" ] ; then
        doLog "Early mount not needed, already mounted at ${mount_path}"
      else
        sudo /usr/sbin/diskutil mount "${node}" 1>/dev/null || abort "Early mount failed!"
        sudo /usr/sbin/diskutil unmount "${node}" 1>/dev/null || abort "Early unmount failed!"
        doLog "Early mount/unmount succeeded"
      fi

      # Use hopefully emulated NVRAM as temporary storage for the boot
      # device node discovered with diskutil.
      # If we are in emulated NVRAM, should not appear at next boot as
      # nvramdump does not write values from OC GUID back to nvram.plist.
      sudo /usr/sbin/nvram "${BOOT_NODE}=${node}" || abort "Failed to store boot device!"
    fi
  else
    abort "Missing or invalid ${BOOT_PATH} value!"
  fi
}

saveNvram() {
  if [ "${1}" = "1" ] ; then
    # . matches tab, note that \t for tab cannot be used in earlier macOS (e.g Mojave)
    node=$(/usr/sbin/nvram "$BOOT_NODE" | sed -n "s/${BOOT_NODE}.//p")
    if [ ! "$INSTALLED" = "1" ] ; then
      # don't trash saved value if daemon is live
      launchctl print "system/${ORG}.daemon" 2>/dev/null 1>/dev/null || sudo /usr/sbin/nvram -d "$BOOT_NODE"
    else
      sudo /usr/sbin/nvram -d "$BOOT_NODE"
    fi
  fi

  if [ "${node}" = "" ] ; then
    abort "Cannot access saved device node!"
  elif [ "${node}" = "null" ] ; then
    sudo /usr/sbin/nvram "${BOOT_NODE}=" || abort "Failed to remove boot node variable!"
    doLog "Uninstalling…"
    return
  fi

  mount_path=$(mount | sed -n "s:${node} on \(.*\) (.*$:\1:p")
  if [ ! "${mount_path}" = "" ] ; then
    doLog "Already mounted at ${mount_path}"
  else
    # use reasonably assumed unique path
    mount_path="/Volumes/${UNIQUE_DIR}"
    sudo mkdir -p "${mount_path}" || abort "Failed to make directory!"
    sudo mount -t msdos "${node}" "${mount_path}" || abort "Failed to mount!"
    doLog "Mounted at ${mount_path}"
    MOUNTED=1
  fi

  if [ "${NVRAM_DIR}" = "" ] ; then
    nvram_dir="${mount_path}"
  else
    nvram_dir="${mount_path}/${NVRAM_DIR}"
    if [ ! -d "${nvram_dir}" ] ; then
      sudo mkdir "${nvram_dir}" || abort "Failed to make directory ${nvram_dir}"
    fi
  fi

  rm -f /tmp/nvram.plist
  ${USE_NVRAMDUMP} || abort "failed to save nvram.plist!"

  if [ -f "${nvram_dir}/nvram.plist" ] ; then
    cp "${nvram_dir}/nvram.plist" "${nvram_dir}/nvram.fallback" || abort "Failed to create nvram.fallback!"
    doLog "Copied nvram.fallback"
  fi

  cp /tmp/nvram.plist "${nvram_dir}/nvram.plist" || abort "Failed to copy nvram.plist!"
  doLog "Saved nvram.plist"

  rm -f /tmp/nvram.plist

  if [ -f "${nvram_dir}/nvram.used" ] ; then
    rm "${nvram_dir}/nvram.used" || abort "Failed to delete nvram.used!"
    doLog "Deleted nvram.used"
  fi

  # We would like to always unmount here if we made the mount point, but at exit of
  # installed daemon umount fails with "Resource busy" and diskutil is not available.
  # This should not cause any problem except that the boot drive will be left mounted
  # at the unique path if the daemon process gets killed (the process would then be
  # restarted by macOS and NVRAM should still be saved at exit).
  if [ "$MOUNTED" = "1" ] ; then
    # For logout hook or if running in immediate mode, we can clean up.
    if [   "$LOGOUT"    = "1" ] ||
       [ ! "$INSTALLED" = "1" ] ; then
      # Depending on how installed and macOS version, either unmount may be needed.
      # (On Lion failure of 'diskutil unmount' may be to say volume is already unmounted when it is not.)
      MOUNTED=0
      sudo diskutil unmount "${node}" 1>/dev/null 2>/dev/null || MOUNTED=1

      if [ "$MOUNTED" = "0" ] ; then
        doLog "Unmounted with diskutil"
      else
        sudo umount "${node}" || abort "Failed to unmount!"
        sudo rmdir "${mount_path}" || abort "Failed to remove directory!"
        doLog "Unmounted with umount"
      fi
    fi
  fi
}

onComplete() {
  doLog "Trap ${1}"

  if [ "$DAEMON" = "1" ] ; then
    saveNvram 1 "daemon"
  fi

  doLog "Ended."

  # Required if running directly (launchd kills any orphaned child processes by default).
  # ShellCheck ref: https://github.com/koalaman/shellcheck/issues/648#issuecomment-208306771
  # shellcheck disable=SC2046
  kill $(jobs -p)

  exit 0
}

if [ ! -x /usr/bin/dirname   ] ||
   [ ! -x /usr/bin/basename  ] ||
   [ ! -x /usr/bin/wc        ] ||
   [ ! -x /usr/sbin/diskutil ] ||
   [ ! -x /usr/sbin/nvram    ] ; then
  earlyAbort "Unix environment is broken!"
fi

NVRAM_DIR="NVRAM"

OCNVRAMGUID="4D1FDA02-38C7-4A6A-9CC6-4BCCA8B30102"

# Darwin version from which we can wake a sleeping daemon, corresponds to Yosemite and upwards.
LAUNCHD_DARWIN=14

# OC generated NVRAM var
BOOT_PATH="${OCNVRAMGUID}:boot-path"

# temp storage var for this script
BOOT_NODE="${OCNVRAMGUID}:boot-node"

# re-use as unique directory name for mount point when needed
UNIQUE_DIR="${BOOT_NODE}"

PRIVILEGED_HELPER_TOOLS="/Library/PrivilegedHelperTools"

ORG="org.acidanthera.nvramhook"
NVRAMDUMP="${PRIVILEGED_HELPER_TOOLS}/${ORG}.nvramdump.helper"

DAEMON_PLIST="/Library/LaunchDaemons/${ORG}.daemon.plist"
AGENT_PLIST="/Library/LaunchAgents/${ORG}.agent.plist"

HELPER="${PRIVILEGED_HELPER_TOOLS}/${ORG}.helper"

LOGDIR="/var/log/${ORG}.launchd"
# launchd .plist configuration is set in install() to redirect stdout and stderr (useful) to LOGFILE anyway.
LOGFILE="${LOGDIR}/launchd.log"

SELFDIR="$(/usr/bin/dirname "${0}")"
SELFNAME="$(/usr/bin/basename "${0}")"

# -R sets this, on newer macOS only; date format is required because root from user sudo and root
# when running daemons or logout hooks are picking up different date formats.
DATEFORMAT="%a, %d %b %Y %T %z"

# Detect whether we're running renamed, i.e. installed copy, or else presumably from distribution directory.
if [ ! "$SELFNAME" = "Launchd.command" ] ; then
  USE_NVRAMDUMP="${NVRAMDUMP}"
  INSTALLED=1
else
  cd "${SELFDIR}" || earlyAbort "Failed to enter working directory!"

  if [ ! -x ./nvramdump ] ; then
    earlyAbort "Compiled nvramdump is not found!"
  fi

  USE_NVRAMDUMP="./nvramdump"
fi

for arg;
do
  case $arg in
    install )
      INSTALL=1
      ;;
    uninstall )
      UNINSTALL=1
      ;;
    daemon )
      DAEMON=1
      ;;
    logout )
      LOGOUT=1
      ;;
    status )
      STATUS=1
      ;;
    # Usage for invalid param.
    # Note script called as logout hook gets passed name of user logging out as a param.
    * )
      if [ ! "$INSTALLED" = "1" ] ; then
        usage
        exit 0
      fi
      ;;
  esac
done

# If not told what to do, work it out.
# When running as daemon, 'daemon' is specified as a param.
if [ ! "$DAEMON" = "1" ] &&
   [ ! "$LOGOUT" = "1" ] ; then
  if [ "$INSTALL"   = "1" ] ||
     [ "$UNINSTALL" = "1" ] ; then
    # When not specified, choose to (un)install daemon or logout hook depending on macOS version.
    if [ "$(getDarwinMajorVersion)" -ge ${LAUNCHD_DARWIN} ] ; then
      echo "Darwin $(uname -r) >= ${LAUNCHD_DARWIN}, using daemon"
      DAEMON=1
    else
      echo "Darwin $(uname -r) < ${LAUNCHD_DARWIN}, using logout hook."
      LOGOUT=1
    fi
  else
    if [ "$INSTALLED" = "1" ] ; then
      # If installed with no type as a param, we are installed as logout hook.
      LOGOUT=1
    elif [ ! "$STATUS"    = "1" ] ; then
      # Usage for no params.
      usage
      exit 0
    fi
  fi
fi

if [ "$DAEMON" = "1" ] ; then
  LOG_PREFIX="Daemon"
elif [ "$LOGOUT" = "1" ] ; then
  LOG_PREFIX="Logout"
fi

if [ ! "$INSTALLED" = "1" ] ; then
  LOG_PREFIX="${LOG_PREFIX}-Immediate"
fi

if [ "$INSTALL"   = "1" ] ||
   [ "$UNINSTALL" = "1" ] ||
   [ "$STATUS"    = "1" ] ; then
  # Get root immediately
  sudo echo -n || earlyAbort "Could not obtain sudo!"

  if [ "$UNINSTALL" = "1" ] ||
     [ "$STATUS"    = "1" ] ; then
    # For agent/daemon pid prefer 'launchctl list' as it works on older macOS where 'launchtl print' does not.
    DAEMON_PID="$(sudo launchctl list | sed -n "s/\([0-9\-]*\).*${ORG}.daemon/\1/p" | sed 's/-/Failed to start!/')"
    LOGOUT_HOOK="$(sudo defaults read com.apple.loginwindow LogoutHook 2>/dev/null)"
  fi
fi

if [ "$INSTALL"   = "1" ] ||
   [ "$UNINSTALL" = "1" ] ; then
  uninstallAgent
fi

if [ "$INSTALL" = "1" ] ; then
  installLog

  # Save NVRAM immediately, this will become the fallback NVRAM after the first shutdown.
  # Do not install if this fails, since this indicates that required boot path from OC is
  # not available, or other fatal error.
  if [ "$DAEMON" = "1" ] ||
     [ "$LOGOUT" = "1" ] ; then
    doLog "Installing…"
    doLog "Saving initial nvram.plist…"
    saveMount 0
    if [ "$DAEMON" = "1" ] ; then
      saveNvram 0 "daemon"
    else
      saveNvram 0 "logout"
    fi
  fi

  install

  exit 0
fi

if [ "$UNINSTALL" = "1" ] ; then
  msg="$(uninstall)"
  if [ ! "${msg}" = "" ] ; then
    doLog "${msg}"
  fi
  exit 0
fi

if [ "$STATUS" = "1" ] ; then
  status
  exit 0
fi

if [ "${LOGOUT}" = "1" ] ; then
  saveMount 0
  saveNvram 0 "logout"
  exit 0
fi

# Useful for trapping all signals to see what we get.
#for s in {1..31} ;do trap "onComplete $s" $s ;done

# Trap CTRL+C for testing for immediate mode, and trap normal or forced daemon
# or agent termination. Separate trap commands so we can log which was caught.
trap "onComplete SIGINT" SIGINT
trap "onComplete SIGTERM" SIGTERM

doLog "Starting…"

if [ "$DAEMON" = "1" ] ; then
  saveMount 1
fi

while true
do
    doLog "Running…"

    # https://apple.stackexchange.com/a/126066/113758
    # Only works from Yosemite upwards.
    sleep $RANDOM & wait
done
