#!/bin/bash

#
# Copyright © 2019-2022 Rodion Shingarev, PMheart, vit9696, mikebeaton.
#
# Includes logic to install and run this script as one or more of
# agent, daemon or logout hook. Can also be run directly for testing,
# use CTRL+C for script termination.
#
# Currently installs just as launch daemon, which is the only one that
# on shutdown can access NVRAM after macOS installer vars are set.
#
# Examine log at /var/log/org.acidanthera.nvramhook.launchd/launchd.log.
#

if [ ! -x /usr/bin/dirname   ] ||
   [ ! -x /usr/bin/basename  ] ||
   [ ! -x /usr/bin/wc        ] ||
   [ ! -x /usr/sbin/diskutil ] ||
   [ ! -x /usr/sbin/nvram    ] ; then
  abort "Unix environment is broken!"
fi

# Non-installed script can be run directly as 'agent' or 'daemon', i.e. script
# is started and runs as if launched that way by launchd, for debugging purposes.
#
# Agent is currently a null install, mainly since it seemed a shame to remove
# that code - i.e. it installs, and stops and starts correctly, but unlike
# daemon it does not actually do anything at stop and start. (Daemon is sudo
# so not everything that daemon does can be done by agent, also not everything
# that daemon does needs to be done, by agent.)
#
# Note that agent shutdown is to early to pick up NVRAM changes made by macOS
# installer before first restart, whereas daemon is not.
#
usage() {
  echo "Usage: ${SELFNAME} [install|uninstall|status] [logout|agent|daemon]"
  echo "  - [install|uninstall|status] with no type uses recommended type (daemon)."
  echo ""
}

doLog() {
  if [ ! "${PREFIX}" = "" ] ; then
    echo "$(date) (${PREFIX}) ${1}" >> "${LOG}"
  else
    echo "${1}" >> "${LOG}"
  fi
}

abort() {
  doLog "Fatal error: ${1}"
  exit 1
}

NVRAM_DIR="NVRAM"

WRITE_HOOK_LOG=0

LOG=/dev/stdout

OCNVRAMGUID="4D1FDA02-38C7-4A6A-9CC6-4BCCA8B30102"

# OC generated NVRAM var
BOOT_PATH="${OCNVRAMGUID}:boot-path"

# temp storage name for this hook
BOOT_NODE="${OCNVRAMGUID}:boot-node"

# re-use as unique directory name for mount point when needed
UNIQUE_DIR="${BOOT_NODE}"

PRIVILEGED_HELPER_TOOLS="/Library/PrivilegedHelperTools"

ORG="org.acidanthera.nvramhook"
NVRAMDUMP="/Library/PrivilegedHelperTools/${ORG}.nvramdump.helper"

DAEMON_PLIST="/Library/LaunchDaemons/${ORG}.daemon.plist"
AGENT_PLIST="/Library/LaunchAgents/${ORG}.agent.plist"

HELPER="/Library/PrivilegedHelperTools/${ORG}.helper"
LOGDIR="/var/log/${ORG}.launchd"
LOGFILE="${LOGDIR}/launchd.log"

SELFDIR="$(/usr/bin/dirname "${0}")"
SELFNAME="$(/usr/bin/basename "${0}")"

USERID=$(id -u)

for arg;
do
  case $arg in
    install )
      INSTALL=1
      ;;
    uninstall )
      UNINSTALL=1
      ;;
    agent )
      AGENT=1
      PREFIX="Agent"
      ;;
    daemon )
      DAEMON=1
      PREFIX="Daemon"
      ;;
    logout )
      LOGOUT=1
      ;;
    status )
      STATUS=1
      ;;
    * )
      usage
      exit 0
      ;;
  esac
done

if [ ! "$SELFNAME" = "Launchd.command" ] ; then
  USE_NVRAMDUMP="${NVRAMDUMP}"
  INSTALLED=1
else
  cd "${SELFDIR}" || abort "Failed to enter working directory!"

  if [ ! -x ./nvramdump ] ; then
    abort "nvramdump is not found!"
  fi

  USE_NVRAMDUMP="./nvramdump"
  INSTALLED=0
fi

# When installed runs as logout hook - i.e. dump immediately - unless specified otherwise.
if [ ! "$AGENT"  = "1" ] &&
   [ ! "$DAEMON" = "1" ] &&
   [ ! "$LOGOUT" = "1" ] ; then
  if [ "$INSTALL"   = "1" ] ||
     [ "$UNINSTALL" = "1" ] ; then
    DAEMON=1
  else
    if [ "$INSTALLED" = "0" ] &&
       [ ! "$STATUS" = "1" ] ; then
        usage
        exit 0
    fi
    LOGOUT=1
  fi
fi

# Install one or more of agent, daemon and logout hook.
install() {
  FAIL="Failed to install!"

  if [ ! -d "${PRIVILEGED_HELPER_TOOLS}" ] ; then
    sudo mkdir "${PRIVILEGED_HELPER_TOOLS}" || abort "${FAIL}"
  fi

  if [ "$LOGOUT" = "1" ] ; then
    # logout hook from more permanent location if available
    if [ "$AGENT" = "1" ] ||
       [ "$DAEMON" = "1" ] ; then
      HOOKPATH="${HELPER}"
    else
      HOOKPATH="$(pwd)/${SELFNAME}"
    fi
    sudo defaults write com.apple.loginwindow LogoutHook "${HOOKPATH}" || abort "${FAIL}"
  fi

  sudo cp "${SELFNAME}" "${HELPER}" || abort "${FAIL}"
  sudo cp nvramdump "${NVRAMDUMP}"  || abort "${FAIL}"

  # customise Launchd.command.plist for agent
  if [ "$AGENT" = "1" ] ; then
    sed "s/\$LABEL/${ORG}.agent/g;s/\$HELPER/$(sed 's/\//\\\//g' <<< $HELPER)/g;s/\$PARAM/agent/g;s/\$LOGFILE/$(sed 's/\//\\\//g' <<< $LOGFILE)/g" "Launchd.command.plist" > "/tmp/Launchd.command.plist" || abort "${FAIL}"
    sudo cp "/tmp/Launchd.command.plist" "${AGENT_PLIST}" || abort "${FAIL}"
    rm -f /tmp/Launchd.command.plist
  fi

  # customise Launchd.command.plist for daemon
  if [ "$DAEMON" = "1" ] ; then
    sed "s/\$LABEL/${ORG}.daemon/g;s/\$HELPER/$(sed 's/\//\\\//g' <<< $HELPER)/g;s/\$PARAM/daemon/g;s/\$LOGFILE/$(sed 's/\//\\\//g' <<< $LOGFILE)/g" "Launchd.command.plist" > "/tmp/Launchd.command.plist" || abort "${FAIL}"
    sudo cp "/tmp/Launchd.command.plist" "${DAEMON_PLIST}" || abort "${FAIL}"
    rm -f /tmp/Launchd.command.plist
  fi

  if [ ! -d "${LOGDIR}" ] ; then
    sudo mkdir "${LOGDIR}" || abort "${FAIL}"
  fi

  if [ ! -f "${LOGFILE}" ] ; then
    sudo touch "${LOGFILE}" || abort "${FAIL}"
  fi

  if [ "$AGENT" = "1" ] ; then
    # Allow agent to access log
    sudo chmod 666 "${LOGFILE}" || abort "${FAIL}"
  fi

  if [ "$AGENT" = "1" ] ; then
    # sudo for agent commands to get better logging of errors
    sudo launchctl bootstrap "gui/${USERID}" "${AGENT_PLIST}" || abort "${FAIL}"
  fi

  if [ "$DAEMON" = "1" ] ; then
    sudo launchctl load "${DAEMON_PLIST}" || abort "${FAIL}"
  fi

  echo "Installed."
}

uninstall() {
  UNINSTALLED=1

  if [ "$LOGOUT" = "1" ] ; then
    sudo defaults delete com.apple.loginwindow LogoutHook || UNINSTALLED=0
  fi

  if [ "$AGENT" = "1" ] ; then
    sudo launchctl bootout "gui/${USERID}" "${AGENT_PLIST}" || UNINSTALLED=0
    sudo rm -f "${AGENT_PLIST}" || UNINSTALLED=0
  fi

  if [ "$DAEMON" = "1" ] ; then
    # Special value in saved device node so that nvram.plist is not upadated at uninstall
    sudo /usr/sbin/nvram "${BOOT_NODE}=null" || abort "Failed to save null boot device!"
    sudo launchctl unload "${DAEMON_PLIST}" || UNINSTALLED=0
    sudo rm -f "${DAEMON_PLIST}" || UNINSTALLED=0
  fi

  sudo rm -f "${HELPER}" || UNINSTALLED=0
  sudo rm -f "${NVRAMDUMP}" || UNINSTALLED=0

  if [ "$UNINSTALLED" = "1" ] ; then
    echo "Uninstalled."
  else
    echo "Could not uninstall!"
  fi
}

status() {
  if [ ! "$AGENT" = "1" ] &&
     [ ! "$DAEMON" = "1" ] ; then
    # summary info
    echo "Daemon pid = $(sudo launchctl print "system/${ORG}.daemon" 2>/dev/null | sed -n 's/.*pid = *//p')"
    echo "Agent pid = $(launchctl print "gui/${USERID}/${ORG}.agent" 2>/dev/null | sed -n 's/.*pid = *//p')"
    echo "LogoutHook = $(sudo defaults read com.apple.loginwindow LogoutHook 2>/dev/null)"
  else
    # detailed info on whatever is selected
    if [ "$AGENT" = "1" ] ; then
      launchctl print "gui/${USERID}/${ORG}.agent"
    fi

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
      # fails initially, however (as with `diskutil info` above) in that case the
      # script gets restarted by launchd after ~5s, and eventually either succeeds or
      # finds the drive already mounted (when applicable, i.e. not the ESP).
      mount_path=$(mount | sed -n "s:${node} on \(.*\) (.*$:\1:p")
      if [ ! "${mount_path}" = "" ] ; then
        doLog "Early mount not needed, already mounted at ${mount_path}"
      else
        /usr/sbin/diskutil mount "${node}" 1>/dev/null || abort "Early mount failed!"
        /usr/sbin/diskutil unmount "${node}" 1>/dev/null || abort "Early unmount failed!"
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
    node=$(nvram "$BOOT_NODE" | sed -n "s/${BOOT_NODE}.//p")
    if [ "$INSTALLED" = "0" ] ; then
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
    sudo mkdir "${mount_path}" || abort "Failed to make directory!"
    sudo mount -t msdos "${node}" "${mount_path}" || abort "Failed to mount!"
    doLog "Successfully mounted at ${mount_path}"
  fi

  if [ "${NVRAM_DIR}" = "" ] ; then
    nvram_dir=$mount_path
  else
    nvram_dir="${mount_path}/${NVRAM_DIR}"
    if [ ! -d "${nvram_dir}" ] ; then
      sudo mkdir $nvram_dir || abort "Failed to make directory ${nvram_dir}"
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

  if [ "${WRITE_HOOK_LOG}" = "1" ] ; then
    date >> "${nvram_dir}/${2}.hook.log" || abort "Failed to write to ${2}.hook.log!"
  fi

  # We would like to unmount here, but umount fails with "Resource busy"
  # and diskutil is not available. This should not cause any problem except
  # that the boot drive will be left mounted at the unique path if the
  # daemon process gets killed (the process would then be restarted by macOS
  # and NVRAM should still be saved at exit).
}

onComplete() {
  doLog "Trap ${1}"

  if [ "$DAEMON" = "1" ] ; then
    saveNvram 1 "daemon"
  fi

  doLog "Ended."

  # Needed if running directly (launchd kills any orphaned child processes by default).
  # ShellCheck ref: https://github.com/koalaman/shellcheck/issues/648#issuecomment-208306771
  # shellcheck disable=SC2046
  kill $(jobs -p)

  exit 0
}

if [ "$INSTALL" = "1" ] ; then
  # Get root immediately (not required, looks nicer than doing a bunch of stuff and then asking)
  sudo echo -n

  # Save nvram immediately, will become the fallback after the first daemon shutdown.
  # Do not install if this fails, since this indicates that required boot path from
  # OC is not available, or other fatal error.
  if [ "$DAEMON" = "1" ] ; then
    doLog "Saving initial nvram.plist…"
    saveMount 0
    saveNvram 0 "daemon"
  fi

  install
  exit 0
fi

if [ "$UNINSTALL" = "1" ] ; then
  uninstall
  exit 0
fi

if [ "$STATUS" = "1" ] ; then
  status
  exit 0
fi

if [ "${LOGOUT}" = "1" ] ; then
  #LOG="${SELFDIR}/error.log"
  saveMount 0
  saveNvram 0 "logout"
  exit 0
fi

# Useful for trapping all signals to see what we get.
#for s in {1..31} ;do trap "onComplete $s" $s ;done

# Trap CTRL+C for testing when running in immediate mode, and trap agent/daemon termination.
# Separate trap commands so we can log which was caught.
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
