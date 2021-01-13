/**
 * \file fsw_efi_edk2_base.h
 * Base definitions for the EDK EFI Toolkit environment.
 */
/*
 * Copyright (c) 2012 Stefan Agner
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSW_EFI_EDK2_BASE_H_
#define _FSW_EFI_EDK2_BASE_H_
/*
 * Here is common declarations for EDK<->EDK2 compatibility
 */
# include <Base.h>
# include <Uefi.h>
# include <Library/DebugLib.h>
# include <Library/BaseLib.h>
# include <Protocol/DriverBinding.h>
# include <Library/BaseMemoryLib.h>
# include <Library/UefiRuntimeServicesTableLib.h>
# include <Library/UefiDriverEntryPoint.h>
# include <Library/UefiBootServicesTableLib.h>
# include <Library/MemoryAllocationLib.h>
# include <Library/DevicePathLib.h>
# include <Protocol/DevicePathFromText.h>
# include <Protocol/DevicePathToText.h>
# include <Protocol/DebugPort.h>
# include <Protocol/DebugSupport.h>
# include <Library/PrintLib.h>
# include <Library/UefiLib.h>
# include <Protocol/SimpleFileSystem.h>
# include <Protocol/BlockIo.h>
# include <Protocol/DiskIo.h>
# include <Guid/FileSystemInfo.h>
# include <Guid/FileInfo.h>
# include <Guid/FileSystemVolumeLabelInfo.h>
# include <Protocol/ComponentName.h>

# define BS gBS

# define EFI_FILE_HANDLE_REVISION EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION
# define SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL_INFO  SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL
# define EFI_FILE_SYSTEM_VOLUME_LABEL_INFO EFI_FILE_SYSTEM_VOLUME_LABEL
# define EFI_SIGNATURE_32(a, b, c, d) SIGNATURE_32(a, b, c, d)
# define DivU64x32(x,y,z) DivU64x32((x),(y))


#endif
