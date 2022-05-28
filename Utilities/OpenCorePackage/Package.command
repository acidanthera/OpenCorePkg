#!/bin/bash
# script OpenCorePackage
# OpenCorePackage by chris1111
#

# Use some vars
PARENTDIR=$(dirname "$0")
cd "$PARENTDIR"
root_build="./OC-EFI"
distrib_dir="./Distribution"
resources_dir="./Resource"
sript_uefi="./Script/UEFI"
script_duet="./Script/Legacy"
temp_dir="/Private/tmp/Package-DIR"
root_dir="./OpenCore-Package"
package_name="./OpenCorePackage.pkg"
duet_name="./OC-EFI/usr/standalone/i386/boot0"
duetfile_name="Duet"

# Delete build if exist
find . -name '.DS_Store' -type f -delete
rm -rf $temp_dir
rm -rf $root_dir
rm -rf $package_name
Sleep 1
mkdir -p $temp_dir
mkdir -p $root_dir/BUILD-PACKAGE
Sleep 1
echo "
= = = = = = = = = = = = = = = = = = = = = = = = =
Create the Packages with pkgbuild "
# Create the Packages with pkgbuild
pkgbuild --root $root_build --scripts $sript_uefi --identifier com.opencoreUefi.pkg --version 1.0 --install-location /Private/tmp $root_dir/BUILD-PACKAGE/Uefi.pkg
pkgbuild --root $root_build --scripts $script_duet --identifier com.opencoreLegacy.pkg --version 1.0 --install-location /Private/tmp $root_dir/BUILD-PACKAGE/Legacy.pkg
Sleep 1
# Expend the Packages with pkgutil
pkgutil --expand $root_dir/BUILD-PACKAGE/Uefi.pkg $temp_dir/Uefi.pkg
pkgutil --expand $root_dir/BUILD-PACKAGE/Legacy.pkg $temp_dir/Legacy.pkg
Sleep 1
# Copy resources and distribution
FILE="$duet_name"
if [ -f "$FILE" ]; then
    echo "$duetfile_name is build, use Distribution UEFI/Duet."
    cp -rp $distrib_dir/Distribution $temp_dir/Distribution
    cp -rp $resources_dir/UEFI-DUET/Resources $temp_dir
else 
    echo "$duetfile_name does not build, use Distribution UEFI."
    cp -rp $distrib_dir/DistributionUefiOnly $temp_dir/Distribution
    cp -rp $resources_dir/UEFI/Resources $temp_dir
fi
echo "
= = = = = = = = = = = = = = = = = = = = = = = = =
Create the final Packages with pkgutil "
Sleep 1
# Flatten the Packages with pkgutil
pkgutil --flatten $temp_dir ./OpenCorePackage.pkg
rm -rf $root_dir
rm -rf $temp_dir
echo "Done "

