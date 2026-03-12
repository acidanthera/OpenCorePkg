BootInstall
===========

This tool installs legacy DuetPkg environment on GPT-formatted disk
to enable UEFI environment on BIOS-based systems.

Linux & macOS
-------------

The tool supports Linux and macOS natively.

Windows
-------

For Windows one will have to perform steps manually with PowerShell. Rough description is provided below.

1. Dump MBR (replace `PHYSICALDRIVE0` with your drive). You might want to copy `old_mbr.bin` somewhere for backup purposes.

```powershell
$disk = [System.IO.File]::Open("\\.\PHYSICALDRIVE0", [System.IO.FileMode]::Open)
$buffer = New-Object byte[] 512
$disk.Read($buffer, 0, 512) | Out-Null
[System.IO.File]::WriteAllBytes("old_mbr.bin", $buffer)
$disk.Close()
```
2. Add `boot0` to MBR (uses `dd` command from WSL).

```sh
cp old_mbr.bin mbr.bin
dd if=boot0 of=mbr.bin bs=1 count=446 conv=notrunc
```

3. Assign a letter to the EFI partition to perform raw access. Use your own disk number (here 0) and partition number (here 1) in `diskpart` commands. Select another letter if `X` is unavailable. 

```powershell
dispart
> select disk 0
> select part 1
> assign letter X
> quit
fsutil volume dismount X:
```

4. Dump PBR (replace `X:` with your volume). You might want to copy `old_pbr.bin` somewhere for backup purposes.

```powershell
$disk = [System.IO.File]::Open("\\.\X:", [System.IO.FileMode]::Open)
$buffer = New-Object byte[] 512
$disk.Read($buffer, 0, 512) | Out-Null
[System.IO.File]::WriteAllBytes("old_pbr.bin", $buffer)
$disk.Close()
```

5. Update PBR with `boot1f32` (uses `dd` command from WSL).

```sh
cp boot1f32 pbr.bin
dd if=old_pbr.bin of=pbr.bin skip=3 seek=3 bs=1 count=87 conv=notrunc
dd if=/dev/random of=pbr.bin skip=496 seek=496 bs=1 count=14 conv=notrunc
```

6. Write MBR (replace `PHYSICALDRIVE0` with your drive).

```powershell
$disk = [System.IO.File]::Open("\\.\PHYSICALDRIVE0:", [System.IO.FileMode]::Open)
$buffer = [System.IO.File]::ReadAllBytes("mbr.bin")
$disk.Write($buffer, 0, $buffer.Length)
$disk.Close()
```

7. Write PBR (replace `X:` with your volume).

```powershell
$disk = [System.IO.File]::Open("\\.\X:", [System.IO.FileMode]::Open)
$buffer = [System.IO.File]::ReadAllBytes("pbr.bin")
$disk.Write($buffer, 0, $buffer.Length)
$disk.Close()
```
