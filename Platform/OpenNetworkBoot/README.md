# OpenNetworkBoot

`OpenNetworkBoot` is an OpenCore boot entry protocol driver which provides
PXE and HTTP boot entries if the underlying firmware supports it, or if
the required network boot drivers have been loaded using OpenCore. Using the
additional network boot drivers provided with OpenCore, when needed, HTTP
boot should be available on most firmware even if not natively supported.

See [OpenCore documentation](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf)
for information on the optional configuration arguments which are available for this driver.

> **Note**: In this file 'HTTP boot' refers to booting using either
`http://` or `https://` URIs. The additional steps to configure a certificate for
`https://` (and to lock `OpenNetworkBoot` to `https://` only, if required)
are covered below.

## PXE Boot

On almost all firmware, the drivers for PXE boot will already be present;
adding `OpenNetworkBoot.efi` to the OpenCore drivers should produce PXE
boot entries.

> **Note**: On some firmware (e.g. HP) the native network boot drivers are not loaded
if the system boots directly to OpenCore and it is necessary to start
OpenCore from the firmware boot menu in order to see PXE and HTTP boot entries.
(Alternatively, it should be possible to load the network boot stack provided with
OpenCore, see end of this document.)

## HTTP Boot

On most recent firmware either no or only a few additional drivers are needed
for HTTP boot, as most of the required drivers are already present in firmware.

After adding `OpenNetworkBoot`, if no HTTP boot entries are seen, 
try adding just the driver `HttpBootDxe`. If this does not produce
network boot entries, try also adding `HttpDxe` and `HttpUtilitiesDxe`.
If `http://` URIs can be booted but not `https://` try adding `TlsDxe.efi`.

If the above steps do not work, proceed to the next section to identify
which drivers are required.

> **Note 1**: When using `https://` as opposed to `http://` URIs, one or
more certificates, as required to validate the connection, must
be configured on the network boot client. This can be done using
OpenNetworkBoot's certificate configuration options, as documented in the
[OpenCore documentation](https://github.com/acidanthera/OpenCorePkg/blob/master/Docs/Configuration.pdf).

> **Note 2**: In some firmware the existing `HttpBootDxe` driver may produce
options which do not work correctly (e.g. blank screen when selected,
because they are designed to work with a firmware UI which is not present
when OpenCore is running).
If so, in order to get working HTTP Boot options it may be necessary to use
the `UEFI/Unload` config setting to unload the existing `HttpBootDxe` driver
before loading the `HttpBootDxe` driver provided with OpenCore.

> **Note 3**: In some firmware the existing `HttpDxe` (and `HttpBootDxe`) drivers
may be locked down to `https://` URIs (even if the machine
has no BIOS UI for HTTP Boot; e.g. Dell OptiPlex 3070).
This means that while the `HttpBootDxe` from OpenCore can
work with the native `HttpDxe`, it will only boot from `https://` URIs, giving a
failure message otherwise. If `http://` URIs are required, this limitation can
be worked around by using the `UEFI/Unload` config setting to unload the existing
`HttpDxe` driver before loading the `HttpDxe` driver provided with OpenCore.

> **Note 4**: During HTTP Boot '*Error: Could not retrieve NBP file size from HTTP server*'
is a very generic error message for 'something went wrong'.
It could be that `http://` URIs are not allowed by `HttpDxe` or `HttpBootDxe`,
or that a file does not exist at the specified URI on the server, or that the certificates
(if any) stored in NVRAM could not be used to validate an `https://` URI, or any one of a number
of other similar problems.

> **Note 5**: During HTTP Boot, an initial error such as 'IP address not found' or 'Server response timeout',
even if preceded by the above message, may mean that no IP address was issued by DHCP, or
that the additional NBP (i.e. boot file) info requested over DHCP was not found. Using `dnsmasq` as the DHCP helper
with the logging options shown below can be helpful to resolve this: any DHCP request which reaches `dnsmasq`
will show a couple of log lines. If these are not seen during a network boot attempt then there is no chance of
`dnsmasq` responding, and network connectivity issues need to be resolved. (It is worth noting that unless blocked,
this DHCP request traffic will normally be broadcast between local networks.) If these log lines are seen but `dnsmasq` does not
additionally log that it is responding with NBP information, make sure that it is configured with the correct
subnetwork for the client machine. It can help to boot the client computer into an OS to confirm which subnetwork
it is on.

## Identifying missing network boot drivers

The `dh` command in UEFI Shell (e.g. `OpenShell` provided with
OpenCore) is useful for working out which drivers are missing for network
boot.

`dh -p LoadFile` shows available network boot entries. Handles with a device
path ending in an IPv4 or IPv6 address should indicate available PXE boot
options. Handles with a device path ending in `Uri(...)` should indicate
available HTTP boot options

> **Note 1**: On some systems, there may be additional
`LoadFile` handles with vendor-specific device paths. These may correspond, for
instance, to GUI network boot options. These will not produce boot entries when
using OpenNetworkBoot.

After identifying the handles for network boot entries,
the other handles just before and after these, in the full
list of handles displayed by `dh`, should correspond to the currently loaded
network boot drivers. If there are no LoadFile options, then
search in the full handle list for strings such as 'tcp', 'tls', 'http'
(normally the native network boot drivers will appear grouped together).
Examining the names printed by `dh` for these handles
and comparing them to the available network boot drivers (see Network Boot Stack
section) can be used to identify missing drivers.

> **Note 2**: On systems with reasonably fast console text output, the `-b`
option can be used with `dh` (as with most UEFI Shell commands) to
display results one page at a time.

> **Note 3**: For systems with slow console output, it may be more
convenient to pipe the results of `dh` to a file on a convenient file system
to examine later, within a booted OS. For example `dh > fs0:\dh_dump` or:

```
> fs0:
> dh > dh_dump
```

## Configuring a network boot server

In standard PXE and HTTP boot, the network's normal DHCP server responds to a
client device's request for an IP address, but a separate DHCP helper service
(often running on a different machine from the DHCP server) responds to the
device's DHCP extension request to know which network boot program (NBP) to
load. It is possible (but less standard, even on enterprise networks;
and usually more complex) to configure the main DHCP server to respond to both
requests.

**Note 1**: The NBP for HTTP boot can be configured by specifying the `--uri`
flag for `OpenNetworkBoot`. When using this option, only an HTTP server (and
certificate, for HTTPS), needs to be set up, but no DHCP helper service is
required.

**Note 2**: No equivalent NBP path option is provided for PXE boot, since the most
standard (and recommended) set up is for the program specifying the
NBP file and the program serving it via TFTP to be one and the same.

### PXE Boot

In PXE boot, the NBP is loaded via TFTP, which is a slow protocol not suitable
for large files. Standard PXE boot NBPs typically load any further large files
which they need using their own network stack and not via TFTP.

`dnsmasq`, WDS, or other options, such as FOGProject, may be used to specify
PXE boot responses.

#### dnsmasq

`dnsmasq` can be used to both provide the location of the PXE boot NBP file
and then serve it by TFTP.

A basic `dnsmasq` PXE boot configuration is as follows:

```
# Disable DNS Server
port=0

# Run as network boot DHCP proxy
dhcp-range=192.168.10.0,proxy

# Identify requested arch
# REF: https://wiki.archlinux.org/title/dnsmasq#PXE_server
dhcp-match=set:arch_x64,option:client-arch,7
dhcp-match=set:arch_x64,option:client-arch,9
dhcp-match=set:arch_ia32,option:client-arch,6

# Specify one or more boot options per architecture
pxe-service=tag:arch_x64,x86-64_EFI,"Open Shell",OpenShell.efi
pxe-service=tag:arch_x64,x86-64_EFI,"Boot Helper",BootHelper.efi

# Enable TFTP support
enable-tftp
tftp-root=/home/mjsbeaton/tftp
```

A more advanced configuration might serve different files to different
machines, depending on their hardware id. (The same point applies to
HTTP boot.) See `dnsmasq` documentation.

See **HTTP Boot** **dnsmasq** section below for command to quickly start
`dnsmasq` for testing.

#### WDS

Windows Deployment Services (WDS, which is incuded with Windows Server) can be
used to provide responses to PXE boot requests, and can be configured to serve
non-Windows NBP files.

**Note 1**: Certain aspects of WDS are now deprecated:
https://aka.ms/WDSSupport

**Note 2**: On certain systems, the OpenCore `TextRenderer` setting 
must be set to one of the `System` values in order to see the early output of
`wdsmgfw.efi` (the NBP served by default by WDS). If this text is not visible,
this can be worked round by pressing either `F12` or `Enter` in the pause
after the program has loaded, in order to access the next screen.
The issue of the early text of this software not appearing in some circumstances
is not unique to OpenCore: https://serverfault.com/q/683314

### HTTP Boot

#### dnsmasq

Although `dnsmasq` does not provide complete support for HTTP
boot, as it does for PXE boot, its PXE boot features can be used
to respond to requests for the location of HTTP boot NBP files.

A basic `dnsmasq` HTTP boot configuration is as follows:

```
# Disable DNS Server
port=0

# Run as PXE Boot DHCP proxy for specified network (use default /24 network size)
dhcp-range=192.168.2.0,proxy

# Trigger PXE Boot support on HTTP Boot client request
dhcp-pxe-vendor=HTTPClient

# Set triggering tag if correct arch is present in option 60
dhcp-match=set:arch_x64,option:client-arch,16

# Make PXE Boot support believe it has something to send...
pxe-service=tag:arch_x64,x86-64_EFI,"Network Boot"

# Specify bootfile-name via PXE Boot setting
dhcp-boot=tag:arch_x64,https://michaels-air.lan:8443/images/OpenShell.efi

# Force required vendor class in response, even if not requested
dhcp-option-force=tag:arch_x64,option:vendor-class,HTTPClient
```

To quickly start `dnsmasq` for testing, without running it as a server,
the following command can be used:

```
sudo dnsmasq --no-daemon -C dnsmasq.conf --log-dhcp --log-debug
```

An HTTP server (such as Apache, nginx, or multiple other options) will be
required to serve the actual NBP files over `http://` or `https://`.

References:
 - https://github.com/ipxe/ipxe/discussions/569
 - https://www.mail-archive.com/dnsmasq-discuss@lists.thekelleys.org.uk/msg16278.html

### HTTPS Boot

Note that the certificate for validating `https://` requests should be loaded into
firmware using the OpenNetworkBoot `--enroll-cert` option.

A normal https site would not serve files using a self-signed certificate
authority (CA), but since we are only attempting to serve files to HTTP boot
clients, in this case we can do so.

## Booting ISO and IMG files

Though not often noted in the documentation, the majority of HTTP Boot
implementations support loading `.iso` and `.img` files, which will be
automatically mounted as a ramdisk. If the mounted filesystem includes
`\EFI\BOOT\BOOTx64.efi` (or `\EFI\BOOT\BOOTIA32.efi` for 32-bit) then this
file will be loaded from the ramdisk and started.

The MIME types corresponding to `.iso` and `.img` files are:

 - `application/vnd.efi-iso`
 - `application/vnd.efi-img`

The MIME type for `.efi` files is:

 - `application/efi`

If the MIME type is none of the above, then the corresponding file
extensions (`.iso`, `.img` and `.efi`) are used instead to identify the NBP
file type.

Additionally, for boot options generated by `OpenNetworkBoot`, `.dmg`
and `.chunklist` files will be recognised by extension and loaded,
regardless of MIME type.

> **Note**: Files which cannot be recognised by any of the above MIME types or
file extensions will _not_ be loaded by HTTP boot drivers.

## Booting DMG files

In order to allow booting macOS Recovery, `OpenNetworkBoot` includes
additional support for loading `.dmg` files via HTTP boot. If the NBP
filename is `{filename}.dmg` or `{filename}.chunklist` then the other
file of this pair will be automatically loaded, in order to allow DMG
chunklist verification, and both files will be used for OpenCore DMG booting.

### `DmgLoading` configuration setting

The behaviour of `OpenNetworkBoot`'s DMG support depends on the OpenCore
`DmgLoading` setting as follows:

 - If `DmgLoading` is set to `Signed` then both `.chunklist` and `.dmg` files
must be available from the HTTP server. Either file can be specified as
the NBP, and the other matching file will be loaded afterwards, automatically.
 - If `DmgLoading` is set to `Disabled` and either of these two file extensions
are found as the NBP, then the HTTP boot process will be aborted. (If we allowed
these files to load and then passed them to the OpenCore DMG loading process,
they would be rejected at that point anyway.)
 - If `DmgLoading` is set to `Any` and the NBP is `{filename}.dmg` then only
the `.dmg` file will be loaded, as verification via `.chunklist` is not
carried out with this setting. If the NBP is `{filename}.chunklist` then
the `.chunklist` followed by the `.dmg` will be loaded, but only the `.dmg`
will be used.

## OVMF

OVMF can be compiled with the following flags for full network boot support:

`-D NETWORK_HTTP_BOOT_ENABLE -D NETWORK_TLS_ENABLE -D NETWORK_IP6_ENABLE`

Since a May 2024 security update to the network boot stack, Random
Number Generator (RNG) protocol support is required. If running OVMF
with an Ivy Bridge or above CPU, then the `RngDxe` driver included in
OVMF will provide the required support. For CPUs below Ivy Bridge
the qemu option `-device virtio-rng-pci` must be provided, so that
the `VirtioRngDxe` driver which is also present in OVMF can provide
the required RNG support.

If OVMF is compiled without network boot support (`-D NETWORK_ENABLE=0`), then
network boot support provided with OpenCore may be added by loading the full
Network Boot Stack (see below).

### OVMF networking

If any network boot clients (e.g. OVMF, VMWare) or server programs
(e.g. Apache, `dnsmasq`, WDS) are running on VMs, it is normally easiest
to set these up using bridged network support, which allows the VM to
appear as a separate device with its own IP address on the network.

To start OVMF with bridged network support the following macOS-specific
`qemu` options (which require `sudo`) may be used:

```
-netdev vmnet-bridged,id=mynet0,ifname=en0 \
-device e1000,netdev=mynet0,id=mynic0
```

PXE boot may also be tested in OVMF using qemu's built-in TFTP/PXE server,
available with the qemu user mode network stack, for example using the
following options:

```
-netdev user,id=net0,tftp=$HOME/tftp,bootfile=/OpenShell.efi \
-device virtio-net-pci,netdev=net0
```

No equivalent option is available for HTTP boot, so to experiment with this,
a combination such as bridged networking and `dnsmasq` should be used.

### OVMF HTTPS certificate

When using `https://` as opposed to `http://`, a certificate must be
configured on the network boot client. Within the OVMF menus this may
be done using
`Device Manager/Tls Auth Configuration/Server CA Configuration/Enroll Cert/Enroll Cert Using File`.

> **Tip**: No GUID needs to be provided in the above dialog. All zeroes will be
used if nothing is specified, which is fine if only a single certificate is going
to be configured and managed.

### Debugging network boot on OVMF

Building OVMF with the `-D DEBUG_ON_SERIAL_PORT` option and then passing the
`-serial stdio` option to qemu (and then scrolling back in the output as
needed, to the lines generated during a failed network boot) can be very
useful when trying to debug network boot setup.

OVMF can capture packets using
`-object filter-dump,netdev={net-id},id=filter0,file=$HOME/ovmf.cap`
(`{net-id}` should be replaced as appropriate with the `id` value specified in the
corresponding `-netdev` option).

For additional information on debugging OpenCore using OVMF,
see https://github.com/acidanthera/OpenCorePkg/blob/master/Debug/README.md.

## Network Boot Stack

The following drivers supplied with OpenCore make up the network boot
stack. Please follow the procedures given towards the start of this
document for deciding which drivers to add.

### Prerequisites
Various network boot drivers depend on the presence of HiiDatabase.

A recent (May 2024) security update to the EDK 2 network stack
means that various drivers also depend on the RNG and Hash2 protocols.

These protocols can be checked for in UEFI Shell with:

```
dh -p HIIDatabase
dh -p Rng
dh -p Hash2
```

If not present, the respective drivers should be loaded before
the network boot stack.

```
HiiDatabase
RngDxe
Hash2DxeCrypto
```

### RAM Disk
Required if not already present in firmware, and the user wishes
to load `.iso` or `.img` files with HTTP Boot.

Can be checked for in UEFI Shell with `dh -p RamDisk`.

```
RamDiskDxe
```

### Network Boot Base
```
DpcDxe
SnpDxe
MnpDxe
TcpDxe
```

### IPv4
```
ArpDxe
Dhcp4Dxe
Ip4Dxe
Udp4Dxe
```

### IPv6
```
Dhcp6Dxe
Ip6Dxe
Udp6Dxe
```

### HTTP Boot
```
DnsDxe
HttpDxe
HttpUtilitiesDxe
HttpBootDxe
```

### HTTPS (TLS) support for HTTP Boot
```
TlsDxe
```

### PXE Boot
```
UefiPxeBcDxe
Mtftp4Dxe (IPv4 only)
Mtftp6Dxe (IPv6 only)
```

### Firmware and option ROMs

In many situations network card firmware will already be present, but this section covers situations where it may not be.

The EDK II and AUDK versions of OVMF both include `VirtioNetDxe` by default, even if built with `NETWORK_ENABLE=0`.
This is roughly equivalent to saying that OVMF has network card firmware present, even if the EDK II network stack
is not included. Therefore, note that when using a cut-down or custom build of OVMF, this driver must be present in
order for the rest of the network stack to work.

Most (U)EFI machines include PXE boot which relies on the machine's network card firmware
(e.g. option ROM) being present. However, if using a very old (e.g pre-EFI) machine, or one with very
cut-down firmware, it may be necessary to manually load the network card's (U)EFI firmware in order for the rest
of the network boot stack to work. This may be loaded using OpenCore's `Drivers` section. The driver should be
found from the manufacturer's website or elsewhere online; for example:

 - https://winraid.level1techs.com/t/efi-lan-bios-intel-gopdriver-modules/33948
 - https://www.intel.com/content/www/us/en/download/15755/intel-ethernet-connections-boot-utility-preboot-images-and-efi-drivers.html
