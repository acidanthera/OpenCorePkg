#!/bin/bash

#
# kpdescribe.sh (formerly kernel_diagreport2text.sh)
#
# Prints the stack trace from an OS X kernel panic diagnostic report, along
# with as much symbol translation as your mach_kernel version provides.
# By default, this is some, but with the Kernel Debug Kit, it should be a lot
# more. This is not an official Apple tool.
#
# USAGE:
# 	./kpdescribe.sh [-f kernel_file] [-k kext_dir1;kext_dir2] Kernel_report.panic [...]
#
# Note: The Kernel Debug Kit currently requires an Apple ID to download. It
# would be great if this was not necessary.
#
# This script calls atos(1) for symbol translation, and also some sed/awk
# to decorate remaining untranslated symbols with kernel extension names,
# if the ranges match.
#
# This uses your current kernel, /mach_kernel, to translate symbols. If you run
# this on kernel diag reports from a different kernel version, it will print
# a "kernel version mismatch" warning, as the translation may be incorrect. Find
# a matching mach_kernel file and use the -f option to point to it.
#
# Updated in 2018 by vit9696 to support recent macOS versions, KEXT symbol solving,
# register printing, and other stuff to work in bash.
#
# Copyright 2014 Brendan Gregg.  All rights reserved.
# Copyright 2018 vit9696.  All rights reserved.
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at docs/cddl1.txt or
# http://opensource.org/licenses/CDDL-1.0.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at docs/cddl1.txt.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END

kernel=/mach_kernel
kextdirs=$(echo "/System/Library/Extensions;/Library/Extensions" | tr ";" "\n")

if [ ! -f "$kernel" ]; then
	kernel=/System/Library/Kernels/kernel
fi

function usage {
	echo "USAGE: $0 [-f kernel_file] [-k kext_dir1;kext_dir2] Kernel_diag_report.panic [...]"
	echo "   eg, $0 /Library/Logs/DiagnosticReports/Kernel_2014-05-26-124827_bgregg.panic"
	exit
}
(( $# == 0 )) && usage
[[ $1 == "-h" || $1 == "--help" ]] && usage

while true; do
	if [[ $1 == "-f" ]]; then
		kernel=$2
		if [[ ! -e $kernel ]]; then
			echo "ERROR: Kernel $kernel not found. Quitting."
			exit 2
		fi
		shift 2
	elif [[ $1 == "-k" ]]; then
		kextdirs=$(echo "$2" | tr ";" "\n")
		shift 2
	else
		break
	fi
done

if [[ ! -x /usr/bin/atos ]]; then
	echo "ERROR: Couldn't find, and need, /usr/bin/atos. Is this part of Xcode? Quitting..."
	exit 2
fi

kexts=()
# Expansion is intentional here
# shellcheck disable=SC2068
for kextdir in ${kextdirs[@]}; do
	if [ -d "$kextdir" ]; then
		while IFS='' read -r kext; do kexts+=("$kext"); done < <(find "$kextdir" -name Info.plist)
	fi
done

while (( $# != 0 )); do
	if [[ "$file" != "" ]]; then print; fi
	file=$1
	shift
	echo "File ${file/$HOME/~}"

	if [[ ! -e "$file" ]]; then
		print "ERROR: File ""$file"" not found. Skipping."
		continue
	fi

	# Find slide address
	slide=$(awk '/^Kernel slide:.*0x/ { print $3 }' "$file")
	if [[ "$slide" == "" ]]; then
		echo -n "ERROR: Missing \"Kernel slide:\" line, so can't process ""$file"". "
		echo "This is needed for atos -s. Is this really a Kernel diag panic file?"
		continue
	fi

	# Print panic line
  (grep -E -A 50 '^panic' | grep -E -B 50 '^Backtrace') < "$file" | grep -vE '^Backtrace'

	# Check kernel version match (uname -v string)
	kernel_ver=$(strings -a "$kernel" | grep 'Darwin Kernel Version' | grep -v '@(#)')
	panic_ver=$(grep 'Darwin Kernel Version' "$file")
	warn=""
	if [[ "$kernel_ver" != "$panic_ver" ]]; then
		echo "WARNING: kernel version mismatch (use -f):"
		printf "%14s: %s\n" "$kernel" "$kernel_ver"
		printf "%14s: %s\n" "panic file" "$panic_ver"
		warn=" (may be incorrect due to mismatch)"
	fi

	# Find kernel extension ranges
	ranges=$(awk 'ext == 1 && /0x.*->.*0x/ {
		    gsub(/\(/, " "); gsub(/\)/, ""); gsub(/\[.*\]/, ""); gsub(/@/, " "); gsub(/->/, " ")
		    print $0
		}
		/Kernel Extensions in backtrace/ { ext = 1 }
		/^$/ { ext = 0 }
	' < "$file" | while read -r n v s e; do
		# the awk gsub's convert this line:
		#   com.apple.driver.AppleUSBHub(666.4)[CD9B71FF-2FDD-3BC4-9C39-5E066F66D158]@0xffffff7f84ed2000->0xffffff7f84ee9fff
		# into this:
		#   com.apple.driver.AppleUSBHub 666.4 0xffffff7f84ed2000 0xffffff7f84ee9fff
		# which can then be read as three fields
		echo "$n" "$v" "$s" "$e"
	done)

	i=0
	unset name version start end kfile
	while (( i < ${#ranges[@]} )); do
		read -r n v s e <<< "${ranges[$i]}"
		name[i]=$n
		start[i]=$s
		end[i]=$e

		for kext in "${kexts[@]}"; do
			if [ ! -f "$kext" ]; then
				continue
			fi

			kname=$(/usr/libexec/PlistBuddy -c 'Print CFBundleIdentifier' "$kext" 2>&1)
			if [ "$kname" != "$n" ]; then
				continue
			fi
			kver=$(/usr/libexec/PlistBuddy -c 'Print CFBundleVersion' "$kext" 2>&1)
			if [[ "$kver" =~ $v ]] || [ "$(echo "$v" | grep "$kver")" != "" ]; then
				path="$(dirname "$kext")/MacOS/$(/usr/libexec/PlistBuddy -c 'Print CFBundleExecutable' "$kext" 2>&1)"
				if [ -f "$path" ]; then
					kfile[i]="$path"
				fi
			else
				echo "Version mismatch for $kname ($kver vs $v)"
			fi
		done
		(( i++ ))
	done

	# Print and translate stack
	echo "Slide: $slide"
	echo "Backtrace [addr unslid symbol]$warn:"
	awk 'backtrace == 1 && /^[^ ]/ { print $3 }
		/Backtrace.*Return Address/ { backtrace = 1 }
		/^$/ { backtrace = 0 }
	' < "$file" | while read -r addr; do
		line=""
		# Check extensions
		if [[ $addr =~ 0x* ]]; then
			i=0
			while (( i <= ${#name[@]} )); do
				[[ "${start[i]}" == "" ]] && break
				# Assuming fixed width addresses, use string comparison:
				if [[ $addr > ${start[$i]} && $addr < ${end[$i]} ]]; then
					unslid=$((addr-${start[$i]}))
					if [ "${kfile[$i]}" != "" ]; then
						line=$(atos -o "${kfile[$i]}" -l "${start[$i]}" "$addr")
					else
						line="(in ${name[$i]} at ${start[$i]})"
					fi
					break
				fi
				(( i++ ))
			done
		fi
		# Fallback to kernel
		if [ "$line" = "" ] ; then
			line=$(atos -o "$kernel" -s "$slide" "$addr")
			unslid=$((addr-slide))
		fi
		printf "0x%016llx  0x%016llx  %s\n" "$addr" "$unslid" "$line"
	done

	# Print other key details
	awk '/^BSD process name/ { gsub(/ corresponding to current thread/, ""); print $0 }
		ver == 1 { print "Mac OS version:", $0; ver = 0 }
		/^Mac OS version/ { ver = 1 }
		/^Boot args:/ { print $0 }
	' < "$file"
done

echo ""
echo "ALWAYS include the original panic log as well!"
echo ""
