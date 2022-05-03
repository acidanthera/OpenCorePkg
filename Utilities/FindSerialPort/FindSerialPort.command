#!/bin/sh

## @file
# Copyright (c) 2022, joevt. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
##

serial_dev=$(ioreg -p IODeviceTree -lw0 | perl -e '
  $ioregpath=""; $pcipath=""; while (<>) {
    if ( /^([ |]*)\+\-o (.+)  <class (\w+)/ ) {
      $indent = (length $1) / 2; $name = $2; $class = $3;
      $ioregpath =~ s|^((/[^/]*){$indent}).*|$1/$name|;
      $pcipath =~ s|^((/[^/]*){$indent}).*|$1/|;
      $name = ""; $uid = 0; $classcode = "";
    }
    
    if ( $class eq "IOACPIPlatformDevice" ) { if ( /^[ |]*"name" = <"([^\"]*)">/ ) { $name = $1; }; if ( /^[ |]*"_UID" = "(\d+)"/ ) { $uid = $1; } }
    elsif ( $class eq "IOPCIDevice" ) { if ( /^[ |]*"pcidebug" = "\d+:(\d+):(\d+).*"/ ) { $name = sprintf("Pci(0x%x,0x%x)", $1, $2);  } if ( /^[ |]*"class-code" = <(\w+)>/ ) { $classcode = $1; } }
    if ( /^[ |]*}/ && $name ) {
      if ( $class eq "IOACPIPlatformDevice" ) {
           if ($name eq "PNP0A03") { $name = sprintf ("PciRoot(0x%x)", $uid); }
        elsif ($name eq "PNP0A08") { $name = sprintf ("PciRoot(0x%x)", $uid); }
        elsif ($name =~ /PNP..../) { $name = sprintf ("Acpi(%s,0x%x)", $name, $uid); }
        # not translating all ACPI types since we only care about PCI devices
      }
      $pcipath .= $name;
      if ( $classcode eq "02000700" ) {
        $serialdevicepath = $pcipath =~ s/.*PciRoot\(0x0\)//r =~ s|/Pci\(||gr =~ s|\)| 00 00 |gr =~ s|0x||gr =~ s|,| |gr =~ s|\b(\w)\b|0\1|gr =~ s|$|FF|r;
        print $ioregpath =~ s|/Root/||r . "\n";
        print $pcipath =~ s|/*||r  . "\n"; 
        print "xxd -p -r <<< \"" . $serialdevicepath . "\" | base64\n";
        print "\n";
      }
    }
  }
')

if [ "$serial_dev" != "" ]; then
  echo "${serial_dev}"
else
  echo "No serial device found!"
fi

exit 0
