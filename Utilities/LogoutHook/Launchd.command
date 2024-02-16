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
# Non-installed script can be run directly as 'daemon', 'agent' or 'logout' hook by
# specifying one of those options on the command line. For 'logout' this saves NVRAM
# immediately, for 'daemon' or 'agent' this waits to be terminated e.g. CTRL+C before
# saving.
#
# Credits:
#  - Rodion Shingarev (with additional contributions by vit9696 and PMheart) for
#    original LogoutHook.command
#  - Rodion Shingarev for nvramdump command
#  - Mike Beaton for this script
#

usage() {
  echo "Usage: ${SELFNAME} [install|uninstall|status] [logout|daemon|agent|both]"
  echo "  - [install|uninstall] without type uses recommended type for macOS version"
  echo "  - 'status' shows status of agent, daemon and logout hook"
  echo "  - 'status daemon' or 'status agent' give additional detail"
  echo ""
}

# Non-sudo log only if agent will use it, so logfile has been chmod-ed at install.
# If only daemon or logout hook will use it, explicit sudo required at install
# time, but harmless when running.
doLog() {
  if [ ! "$AGENT" = "1" ] ; then
    # macOS recreates this for daemon at reboot, but preferable
    # to continue logging immediately after any log cleardown,
    # also never recreated except for this with logout hook
    sudo mkdir -p "${LOGDIR}"

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
  else
    # non-sudo copy, could simplify
    if [ ! "${LOG_PREFIX}" = "" ] ; then
      tee -a "${LOGFILE}" > /dev/null << EOF
$(date +"${DATEFORMAT}") (${LOG_PREFIX}) ${1}
EOF
    else
      tee -a "${LOGFILE}" > /dev/null << EOF
${1}
EOF
    fi
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
  if [ ! -d "${LOGDIR}" ] ; then
    # We intentionally don't use -p as something is probably
    # weird if parent dirs don't exist here
    sudo mkdir "${LOGDIR}" || abort "Failed to mkdir for logfile!"
  fi

  if [ ! -f "${LOGFILE}" ] ; then
    sudo touch "${LOGFILE}" || abort "Failed to touch logfile!"
  fi

  if [ "$AGENT" = "1" ] ; then
    sudo chmod 666 "${LOGFILE}" || abort "Failed to chmod logfile!"
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

  if [ "$DAEMON" = "1" ] ; then
    # Make customised Launchd.command.plist for daemon.
    if [ "$BOTH" = "1" ] ; then
      LAUNCHFILE_DAEMON="${LAUNCHFILE_DAEMON} (1 of 2)"
    fi
    if [ -f "$LAUNCHFILE_DAEMON" ] ; then
      sudo rm "$LAUNCHFILE_DAEMON" || abort "${FAIL}"
    fi
    sudo ln -s "$(which sh)" "$LAUNCHFILE_DAEMON" || abort "${FAIL}"
    sed "s/\$LABEL/${ORG}.daemon/g;s/\$HELPER/$(sed 's/\//\\\//g' <<< "$HELPER")/g;s/\$PARAM/${DAEMON_PARAMS}/g;s/\$LOGFILE/$(sed 's/\//\\\//g' <<< "$LOGFILE")/g;s/\$LAUNCHFILE/$(sed 's/\//\\\//g' <<< "$LAUNCHFILE_DAEMON")/g" "Launchd.command.plist" > "/tmp/Launchd.command.plist" || abort "${FAIL}"
    sudo cp "/tmp/Launchd.command.plist" "${DAEMON_PLIST}" || abort "${FAIL}"
    rm -f /tmp/Launchd.command.plist

    # Launch already installed daemon.
    sudo launchctl load "${DAEMON_PLIST}" || abort "${FAIL}"
  fi

  if [ "$AGENT" = "1" ] ; then
    # Make customised Launchd.command.plist for agent.
    if [ "$BOTH" = "1" ] ; then
      LAUNCHFILE_AGENT="${LAUNCHFILE_AGENT} (2 of 2)"
    fi
    if [ -f "$LAUNCHFILE_AGENT" ] ; then
      sudo rm "$LAUNCHFILE_AGENT" || abort "${FAIL}"
    fi
    sudo ln -s "$(which sh)" "$LAUNCHFILE_AGENT" || abort "${FAIL}"
    sed "s/\$LABEL/${ORG}.agent/g;s/\$HELPER/$(sed 's/\//\\\//g' <<< "$HELPER")/g;s/\$PARAM/${AGENT_PARAMS}/g;s/\$LOGFILE/$(sed 's/\//\\\//g' <<< "$LOGFILE")/g;s/\$LAUNCHFILE/$(sed 's/\//\\\//g' <<< "$LAUNCHFILE_AGENT")/g" "Launchd.command.plist" > "/tmp/Launchd.command.plist" || abort "${FAIL}"
    sudo cp "/tmp/Launchd.command.plist" "${AGENT_PLIST}" || abort "${FAIL}"
    rm -f /tmp/Launchd.command.plist

    # Launch already installed agent.
    sudo launchctl bootstrap gui/501 "${AGENT_PLIST}" || abort "${FAIL}"
  fi

  echo "Installed."
}

uninstall() {
  UNINSTALLED=1

  if [ "$DAEMON" = "1" ] || [ "$AGENT" = "1" ] ; then
    if [ "$DAEMON_PID" = "" ] ; then
      UNINSTALLED=2
      if [ ! "$INSTALL" = "1" ] && [ "$DAEMON" = "1" ] ; then
        echo "Daemon was not installed!" > /dev/stderr
      fi
    else
      # Place special value in saved device node so that nvram.plist is not updated at uninstall
      sudo /usr/sbin/nvram "${BOOT_NODE}=null" || abort "Failed to save null boot device!"
      sudo launchctl unload "${DAEMON_PLIST}" || UNINSTALLED=0
      DAEMON_PID=
    fi
    sudo rm -f "${DAEMON_PLIST}"
    sudo rm -f "${LAUNCHFILE_DAEMON}"*
  fi

  if [ "$AGENT" = "1" ] || [ "$DAEMON" = "1" ] ; then
    if [ "$AGENT_PID" = "" ] ; then
      UNINSTALLED=2
      if [ ! "$INSTALL" = "1" ] && [ "$AGENT" = "1" ] ; then
        echo "Agent was not installed!" > /dev/stderr
      fi
    else
      # Place special value in saved device node so that nvram.plist is not updated at uninstall
      sudo /usr/sbin/nvram "${BOOT_NODE}=null" || abort "Failed to save null boot device!"
      sudo launchctl bootout "${AGENT_ID}" || UNINSTALLED=0
      AGENT_PID=
    fi
    sudo rm -f "${AGENT_PLIST}"
    sudo rm -f "${LAUNCHFILE_AGENT}"*
  fi

  if [ "$LOGOUT" = "1" ] ; then
    if [ "$LOGOUT_HOOK" = "" ] ; then
      UNINSTALLED=2
      echo "Logout hook was not installed!" > /dev/stderr
    else
      sudo defaults delete com.apple.loginwindow LogoutHook || UNINSTALLED=0
      LOGOUT_HOOK=
    fi
  fi

  if [ "$DAEMON_PID"  = "" ] &&
     [ "$AGENT_PID"   = "" ] &&
     [ "$LOGOUT_HOOK" = "" ] ; then
    sudo rm -f "${HELPER}"
    sudo rm -f "${NVRAMDUMP}"
  fi

  if [ ! "$INSTALL" = "1" ] ; then
    if [ "$UNINSTALLED" = "0" ] ; then
      echo "Could not uninstall!"
    elif [ "$UNINSTALLED" = "1" ] ; then
      echo "Uninstalled."
    fi
  fi
}

status() {
  if [ "$DAEMON" = "1" ] ; then
    # Detailed info on daemon.
    sudo launchctl print "$DAEMON_ID"
  elif [ "$AGENT" = "1" ] ; then
    # Detailed info on agent.
    sudo launchctl print "$AGENT_ID"
  else
    echo "Daemon pid = ${DAEMON_PID}"
    echo "Agent pid = ${AGENT_PID}"
    if [ ! "$BOTH" = "1" ] ; then
      echo "LogoutHook = ${LOGOUT_HOOK}"
    fi
  fi
}

# On earlier macOS (at least Mojave in VMWare) there is an intermittent
# problem where msdos/FAT kext is occasionally not available when we try
# to mount the drive on daemon exit; mounting and unmounting on daemon
# start here fixes this.
# Doing this introduces a different problem, since this early mount sometimes
# fails initially, however (as with 'diskutil info' above) in that case the
# script gets restarted by launchd after ~5s, and eventually either succeeds or
# finds the drive already mounted (when applicable, i.e. not marked as ESP).
#
# On later macOS (Sonoma) this problem recurs, and the strategy of co-ordinated
# daemon and user agent was introduced to handle this (see saveNvram()):
#
# At startup:
#  - daemon waits to see agent
#  - daemon mounts ESP
#  - daemon kills agent, then waits 1s
#  - agent:
#   o traps kill
#   o touches nvram.plist (which forces kext to be loaded)
#   o exits
#  - after 1s wait daemon unmounts ESP
#
# At shutdown:
#  - daemon mounts ESP and writes nvram.plist
#   o this only works if the above palaver at startup has happened recently
#
earlyMount() {
  mount_path=$(mount | sed -n "s:${node} on \(.*\) (.*$:\1:p")
  if [ ! "${mount_path}" = "" ] ; then
    doLog "Early mount not needed, already mounted at ${mount_path}"
  else
    sudo /usr/sbin/diskutil mount "${node}" 1>/dev/null || abort "Early mount failed!"
    sleep 1 & wait
    sudo /usr/sbin/diskutil unmount "${node}" 1>/dev/null || abort "Early unmount failed!"
    doLog "Early mount/unmount succeeded"
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
#
# $1 = 0 - Set node var, but do not save in emulated NVRAM
# $1 = 1 - Set node var and save in emulated NVRAM
#
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
      if [ "$BOTH" = "1" ] &&
         [ "$DAEMON" = "1" ]; then
        doLog "Using both agent and daemon, delaying early mount"
      else
        earlyMount
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

# $1 = 0 - Use existing node var, do not fetch from emulated NVRAM
# $1 = 1 - Use node var saved in emulated NVRAM
#
# $2 = 0 - Just touch nvram.plist
# $2 = 1 - Really save NVRAM state to nvram.plist
# $2 = 2 - Just pre-mount
#
saveNvram() {
  if [ "${1}" = "1" ] ; then
    # . matches tab, note that \t for tab cannot be used in earlier macOS (e.g Mojave)
    node=$(/usr/sbin/nvram "$BOOT_NODE" | sed -n "s/${BOOT_NODE}.//p")
    if [ "$2" = "1" ]; then
      if [ ! "$INSTALLED" = "1" ] ; then
        # don't trash saved value if daemon or agent is live
        launchctl print "$DAEMON_ID" 2>/dev/null 1>/dev/null || \
        launchctl print "$AGENT_ID" 2>/dev/null 1>/dev/null || \
        sudo /usr/sbin/nvram -d "$BOOT_NODE"
      else
        sudo /usr/sbin/nvram -d "$BOOT_NODE"
      fi
    fi
  fi

  if [ "${node}" = "" ] ; then
    abort "Cannot access saved device node!"
  elif [ "${node}" = "null" ] ; then
    if [ ! "$AGENT" = "1" ] ; then
      sudo /usr/sbin/nvram "${BOOT_NODE}=" || abort "Failed to remove boot node variable!"
    fi
    doLog "Uninstalling…"
    return
  fi

  if [ "$INSTALLED" = "1" ] &&
     [ "$AGENT"     = "1" ] ; then
    if /usr/sbin/nvram "${DAEMON_WAITING}" 1>/dev/null 2>/dev/null ; then
      echo -n
    else
      doLog "Daemon is not waiting, stopping..."
      return
    fi
  fi

  mount_path=$(mount | sed -n "s:${node} on \(.*\) (.*$:\1:p")
  if [ ! "${mount_path}" = "" ] ; then
    doLog "Found mounted at ${mount_path}"
    if [ "$DAEMON" = "1" ] &&
       [ "$BOTH" = "1" ] &&
       [ ! "$2"  = "2" ] &&
       [ "$(getDarwinMajorVersion)" -ge ${LAUNCHD_BOTH} ] ; then
       doLog "WARNING: User-mounted ESP may not save NVRAM successfully"
    fi
  else
    if [ "$INSTALLED" = "1" ] &&
       [ "$AGENT"     = "1" ] ; then
      # This should have been pre-mounted for us by daemon
      abort "Agent cannot mount ESP!"
    fi
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
      mkdir "${nvram_dir}" || abort "Failed to make directory ${nvram_dir}"
    fi
  fi

  if [ "${2}" = "2" ] ; then
    doLog "Killing agent…"
    sudo /usr/sbin/nvram "${DAEMON_WAITING}=1" || abort "Failed to set daemon-waiting variable!"
    kill "$3" || abort "Cannot kill agent!"
    if [ "$BOTH" = "1" ] ; then
      sleep 1 & wait
      sudo /usr/sbin/nvram "${DAEMON_WAITING}=" || abort "Failed to remove daemon-waiting variable!"
    fi
  elif [ ! "${2}" = "1" ] ; then
    # Just touch nvram.plist
    touch "${nvram_dir}/nvram.plist" || abort "Failed to touch nvram.plist!"
    doLog "Touched nvram.plist"
  else
    # Really save NVRAM
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
  fi

  # We would like to always unmount here if we made the mount point, but at exit of
  # installed daemon umount fails with "Resource busy" and diskutil is not available.
  # This should not cause any problem except that the boot drive will be left mounted
  # at the unique path if the daemon process gets killed (the process would then be
  # restarted by macOS and NVRAM should still be saved at exit).
  while [ "$MOUNTED" = "1" ] ;
  do
    # For logout hook or if running in immediate mode, we can clean up.
    if [   "$LOGOUT"    = "1" ] ||
       [ ! "$INSTALLED" = "1" ] ||
       [ "$2" = "2" ] ; then
      # Sometimes needed after directory access...
      # # if [ ! "$2" = "2" ] ; then
      # #   sleep 1 & wait
      # # fi

      # Depending on how installed and macOS version, either unmount may be needed.
      # (On Lion failure of 'diskutil unmount' may be to say volume is already unmounted when it is not.)
      MOUNTED=0
      sudo diskutil unmount "${node}" 1>/dev/null 2>/dev/null || MOUNTED=1

      if [ "$MOUNTED" = "0" ] ; then
        doLog "Unmounted with diskutil"
      else
        sudo umount "${node}" && MOUNTED=0
        if [ "$MOUNTED" = "0" ] ; then
          sudo rmdir "${mount_path}" || abort "Failed to remove directory!"
          doLog "Unmounted with umount"
        else
          if [ "$INSTALLED" = "1" ] ; then
            abort "Failed to unmount!"
          else
            doLog "Retrying…"
          fi
        fi
      fi
    fi
  done
}

onComplete() {
  doLog "Trap ${1}"

  if [ "$DAEMON" = "1" ] ; then
    saveNvram 1 1
  elif [ "$AGENT" = "1" ] ; then
    saveNvram 1 0
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

# Darwin version for which we need both agent and daemon, corresponds to Sonoma and upwards.
LAUNCHD_BOTH=23

# OC generated NVRAM var
BOOT_PATH="${OCNVRAMGUID}:boot-path"

# temp storage vars for this script
BOOT_NODE="${OCNVRAMGUID}:boot-node"
DAEMON_WAITING="${OCNVRAMGUID}:daemon-waiting"

# re-use as unique directory name for mount point when needed
UNIQUE_DIR="${BOOT_NODE}"

PRIVILEGED_HELPER_TOOLS="/Library/PrivilegedHelperTools"

ORG="org.acidanthera.nvramhook"
NVRAMDUMP="${PRIVILEGED_HELPER_TOOLS}/${ORG}.nvramdump.helper"

DAEMON_ID="system/${ORG}.daemon"
DAEMON_PLIST="/Library/LaunchDaemons/${ORG}.daemon.plist"
DAEMON_PARAMS="daemon"

AGENT_ID="gui/501/${ORG}.agent"
AGENT_PLIST="/Library/LaunchAgents/${ORG}.agent.plist"
AGENT_PARAMS="agent"

HELPER="${PRIVILEGED_HELPER_TOOLS}/${ORG}.helper"
LAUNCHFILE_DAEMON="${PRIVILEGED_HELPER_TOOLS}/${ORG} System Daemon"
LAUNCHFILE_AGENT="${PRIVILEGED_HELPER_TOOLS}/${ORG} User Agent"

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
    "daemon both" )
      DAEMON=1
      BOTH=1
      ;;
    agent )
      AGENT=1
      ;;
    "agent both" )
      AGENT=1
      BOTH=1
      ;;
    both )
      BOTH=1
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

# Get root permissions early if immediate mode.
if [ "$INSTALL"   = "1" ] ||
   [ "$UNINSTALL" = "1" ] ||
   [ "$STATUS"    = "1" ] ; then
  if [ "$(whoami)" = "root" ] ; then
    earlyAbort "Do not run this script as root!"
  fi

  sudo echo -n || earlyAbort "Could not obtain sudo!"

  if [ "$INSTALL" = "1" ] ; then
    UNINSTALL=1
  fi

  if [ "$UNINSTALL" = "1" ] ||
     [ "$STATUS"    = "1" ] ; then
    # For agent/daemon pid prefer 'launchctl list' as it works on older macOS where 'launchtl print' does not.
    DAEMON_PID="$(sudo launchctl list | sed -n "s/\([0-9\-]*\).*${ORG}.daemon/\1/p" | sed 's/-/Failed to start!/')"
    AGENT_PID="$(launchctl list | sed -n "s/\([0-9\-]*\).*${ORG}.agent/\1/p" | sed 's/-/Failed to start!/')"
    LOGOUT_HOOK="$(sudo defaults read com.apple.loginwindow LogoutHook 2>/dev/null)"
  fi
fi

# If not told what to do, work it out.
# When running as daemon, 'daemon' is specified as a param.
if [ ! "$DAEMON" = "1" ] &&
   [ ! "$AGENT"  = "1" ] &&
   [ ! "$BOTH"   = "1" ] &&
   [ ! "$LOGOUT" = "1" ] ; then
  if [ "$INSTALL"   = "1" ] ||
     [ "$UNINSTALL" = "1" ] ; then
    # When not specified, choose to (un)install daemon or logout hook depending on macOS version.
    if [ "$(getDarwinMajorVersion)" -ge ${LAUNCHD_BOTH} ] ; then
      echo "Darwin $(uname -r) >= ${LAUNCHD_BOTH}, using both agent and daemon"
      BOTH=1
      AGENT=1
      DAEMON=1
    elif [ "$(getDarwinMajorVersion)" -ge ${LAUNCHD_DARWIN} ] ; then
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
    elif [ ! "$STATUS" = "1" ] ; then
      # Usage for no params.
      usage
      exit 0
    fi
  fi
fi

if [ ! "$INSTALLED" = "1" ] &&
   [ "$BOTH" = "1" ] ; then
  LOG_PREFIX="Both"
  AGENT=1
  DAEMON=1
  AGENT_PARAMS="${AGENT_PARAMS} both"
  DAEMON_PARAMS="${DAEMON_PARAMS} both"
elif [ "$DAEMON" = "1" ] ; then
  LOG_PREFIX="Daemon"
elif [ "$AGENT" = "1" ] ; then
  LOG_PREFIX="Agent"
elif [ "$LOGOUT" = "1" ] ; then
  LOG_PREFIX="Logout"
fi

if [ ! "$INSTALLED" = "1" ] ; then
  LOG_PREFIX="${LOG_PREFIX}-Immediate"
fi

if [   "$INSTALL" = "1" ] &&
   [   "$AGENT"   = "1" ] &&
   [ ! "$DAEMON"  = "1" ] ; then
  earlyAbort "Standalone agent install is not supported!"
fi

if [ "$UNINSTALL" = "1" ] ; then
  msg="$(uninstall)"
  if [ ! "${msg}" = "" ] ; then
    doLog "${msg}"
  fi
  if [ ! "$INSTALL" = "1" ] ; then
    exit 0
  fi
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
      # daemon
      saveNvram 0 1
    else
      # logout hook
      saveNvram 0 1
    fi
  fi

  install

  exit 0
fi

if [ "$STATUS" = "1" ] ; then
  status
  exit 0
fi

if [ "${LOGOUT}" = "1" ] ; then
  saveMount 0
  saveNvram 0 1
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

if [ "$DAEMON" = "1" ] &&
   [ "$BOTH"   = "1" ] ; then
  agent_pid=""
  doLog "Waiting for agent…"
  while [ "$agent_pid" = "" ] ;
  do
    agent_pid="$(pgrep -f "$(echo "${HELPER} agent" | sed "s/\./\\\./g")")"
    if [ "$agent_pid" = "" ] ; then
     # https://apple.stackexchange.com/a/126066/113758
     # Only works from Yosemite upwards.
     sleep 5 & wait
    fi
  done
  doLog "Found agent…"
  saveNvram 1 2 "$agent_pid"
fi

while true
do
  doLog "Running…"

  # https://apple.stackexchange.com/a/126066/113758
  # Only works from Yosemite upwards.
  sleep $RANDOM & wait
done
