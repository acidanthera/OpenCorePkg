## OpenCore + OpenLinuxBoot + Secure Boot

If you want to use OpenCore + OpenLinuxBoot + Secure Boot it is possible to sign everything
manually yourself, including any new Linux kernels after updates. This is possible since most
standard distros leave at least the previous kernel bootable (and OpenLinuxBoot exposes
this, via the Auxiliary menu), so you can boot into the old kernel, then sign the new
kernel yourself.

More convenient may be to trust the signing keys of the specific distros which you
want to boot, which are bundled into the `shimx64.efi` file installed with each distro.
You can extract these with `shim-to-cert.tool` distributed with OpenCore, then install
them in your system Secure Boot `db` variable. Best practice would be to install the deny
list (`vendor.dbx`) from `shimx64.efi`, if any, into your system `dbx` variable, as well.
(Otherwise you are ignoring any revocations which the vendor has made.)

Recently, Shim has added SBAT support, as a more efficient way to revoke unsafe
binaries. Unfortunately, the SBAT enforcement code is part of Shim, and is not
something you can extract and add to your system Secure Boot database.

To work round this, the new recommended way to boot OpenCore + OpenLinuxBoot +
Secure Boot is to make a user build of Shim. The vendor certificates
and revocation lists extracted from the distro `shimx64.efi` files are combined
and signed by you, into your own build of Shim; in this approach, these vendor
certificates should NOT also be included in the system Secure Boot database,
and should be removed if you added them previously. Including them in both places
will still boot under Secure Boot, but will effectively disable SBAT revocation.

> If you are signing everything yourself, including Linux kernels after updates, that
will still work as before and the below is not needed. Equally, if you are not
using Secure Boot the below is not needed.

The advantages of using a user build of Shim are:
 - No need to sign every kernel after updates (same as previous method)
 - Linux SBAT integration (new)
 - Linux MOK integration (new)
 - No need to include the Windows intermediate CA - you are trusting whichever distro
   keys you choose to include in your own Shim, directly (new)

Disadvantages are:
 - Need to update when distro keys or distro revocation lists within Shim are updated
   (same as previous method)
 - Need to udpate when Shim SBAT level is updated (new)

### Method
`Utilities/ShimUtils` includes a script `shim-make.tool` which will download the
current Shim source and build it for you, on macOS (using Ubuntu multipass) or on
Linux (Ubuntu and Fedora supported, others may work).

 - Extract `vendor.db` and `vendor.dbx` files from the `shimx64.efi` file of each distro
   which you want to load (using `shim-to-cert.tool`)
   - For non-GRUB distros, the required public keys for this process cannot be extracted
     from `shimx64.efi` and so must be found by additional user research
 - Concatentate these (e.g. `cat fedora/vendor.db ubuntu/vendor.db > combined/vendor.db`
   and `cat fedora/vendor.dbx ubuntu/vendor.dbx > combined/vendor.dbx`)
   - Do not concatenate `.der` files directly, it will not work
   - If you have a single distro with a single `.der` file, you can use `VENDOR_CERT_FILE`
   instead of `VENDOR_DB_FILE` in the `make` options below; otherwise, you will need to use
   `cert-to-efi-sig-list` from `efitools` to convert the `.der` file to a sig list - this
   is done automatically by `shim-to-cert.tool` when `efitools` are available (in
   Linux; or from within Ubuntu multipass on macOS, e.g. `multipass shell oc-shim`)
 - Build a version of Shim which includes these concatenated signature lists (and
   launches OpenCore.efi directly):
   - `./shim-make.tool setup`
   - `./shim-make.tool clean` (only needed if remaking after the initial make)
   - `./shim-make.tool make VENDOR_DB_FILE={full-path-to}/vendor.db VENDOR_DBX_FILE={full-path-to}/vendor.dbx`
     - On macOS, the paths to these files must either be within the multipass VM, or
       within a subdirectory visible to macOS and the VM on the same path, such as
       `/Users/{username}/shim_root` when using `shim-make.tool` default settings
 - Copy the relevant files (`shimx64.efi` and `mmx64.efi` as well as `BOOTX64.CSV`) to your mounted ESP volume, e.g.:
   - `./shim-make.tool install /Volumes/EFI` (macOS)
   - `sudo ./shim-make.tool install /boot/efi` (Linux)
 - Sign the newly built `shimx64.efi` and `mmx64.efi` with your own ISK (see e.g.
   https://habr.com/en/articles/273497/ - Google translate is your friend)
   - If you do not copy and sign `mmx64.efi` as well as `shimx64.efi`, your system will hang if any MOK operations are attempted
   - `BOOTX64.CSV` is not required and is for information only

As before you need to sign `OpenCore.efi` and any drivers it loads with your ISK.
You now also need to add an empty SBAT section to `OpenCore.efi` before signing it.

> An empty SBAT section means: 'I'm not part of the system which allocates SBAT names
and signs them into boot files, and I don't want this boot file to be revoked by any
future SBAT revocations'. Of course, you can still revoke boot files you signed yourself
by rotating your own signing keys.

As noted [here](https://github.com/fwupd/fwupd/issues/2910) and
[here](https://github.com/rhboot/shim/issues/376),
the [documented](https://github.com/rhboot/shim/blob/main/SBAT.md) method for adding an
SBAT section to an already-linked `.efi` file does not work correctly (GNU `objcopy`
corrupts the executable). This
[third party python script](https://github.com/rhboot/shim/issues/376#issuecomment-1628004034)
does work. A suitable command is:

`pe-add-sections.py -s .sbat <(echo -n) -z .sbat -i OpenCore.efi -o OpenCore_empty_sbat.efi`

This file then needs to be signed and copied back into place, e.g.:

`sbsign --key {path-to}/ISK.key --cert {path-to}/ISK.pem OpenCore_empty_sbat.efi --output OpenCore.efi`

Finally, in order for OpenCore integration with Shim to work correctly
`UEFI/Quirks/ShimRetainProtocol` must be enabled in `config.plist`, and
`LauncherPath` should be set to `\EFI\OC\shimx64.efi`.

> Using Ubuntu multipass, it is now possible to operate entirely within macOS for signing,
key generation, etc. Note that the `~/shim_root` directory is already shared between
macOS and the `oc-shim` multipass VM (under its macOS path, e.g. `/Users/username/shim_root`),
and other macOS folders and volumes can be mounted if you wish, e.g.
`multipass mount /Volumes/EFI oc-shim:/Volumes/EFI`.
