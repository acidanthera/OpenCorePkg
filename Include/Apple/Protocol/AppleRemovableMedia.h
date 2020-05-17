/** @file
  Protocol interface for any device that supports removable media
  (i.e. optical drives).

  Copyright (c) 2005 Apple Computer, Inc.  All rights reserved.
  Portions Copyright (C) 2017, Download-Fritz.  All rights reserved.<BR>

  Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
  Computer, Inc. ("Apple") in consideration of your agreement to the
  following terms, and your use, installation, modification or
  redistribution of this Apple software constitutes acceptance of these
  terms.  If you do not agree with these terms, please do not use,
  install, modify or redistribute this Apple software.

  In consideration of your agreement to abide by the following terms, and
  subject to these terms, Apple grants you a personal, non-exclusive
  license, under Apple's copyrights in this original Apple software (the
  "Apple Software"), to use, reproduce, modify and redistribute the Apple
  Software, with or without modifications, in source and/or binary forms;
  provided that if you redistribute the Apple Software in its entirety and
  without modifications, you must retain this notice and the following
  text and disclaimers in all such redistributions of the Apple Software. 
  Neither the name, trademarks, service marks or logos of Apple Computer,
  Inc. may be used to endorse or promote products derived from the Apple
  Software without specific prior written permission from Apple.  Except
  as expressly stated in this notice, no other rights or licenses, express
  or implied, are granted by Apple herein, including but not limited to
  any patent rights that may be infringed by your derivative works or by
  other works in which the Apple Software may be incorporated.

  The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.

  IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
  MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
  STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
**/

#ifndef APPLE_REMOVABLE_MEDIA_H
#define APPLE_REMOVABLE_MEDIA_H

// APPLE_REMOVABLE_MEDIA_PROTOCOL_GUID
/// Global Id for Removable_Media Interface
#define APPLE_REMOVABLE_MEDIA_PROTOCOL_GUID  \
  { 0x2EA9743A, 0x23D9, 0x425E,              \
    { 0x87, 0x2C, 0xF6, 0x15, 0xAA, 0x19, 0x57, 0x88 } }

#define	APPLE_REMOVABLE_MEDIA_PROTOCOL_REVISION		0x00000001

typedef struct APPLE_REMOVABLE_MEDIA_PROTOCOL APPLE_REMOVABLE_MEDIA_PROTOCOL;

// APPLE_REMOVABLE_MEDIA_EJECT
/** Eject removable media from drive (such as a CD/DVD).

  @param[in] This  Protocol instance pointer.

  @retval EFI_SUCCESS  The media was ejected.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_REMOVABLE_MEDIA_EJECT) (
  IN APPLE_REMOVABLE_MEDIA_PROTOCOL  *This
  );

// APPLE_REMOVABLE_MEDIA_INJECT
/** Inject removable media into drive (such as a CD/DVD).

  @param[in] This  Protocol instance pointer.

  @retval EFI_SUCCESS  The media was injected.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_REMOVABLE_MEDIA_INJECT) (
  IN APPLE_REMOVABLE_MEDIA_PROTOCOL  *This
  );

// APPLE_REMOVABLE_MEDIA_ALLOW_REMOVAL
/** Allow the media to be removed from the drive.

  @param[in] This  Protocol instance pointer.

  @retval EFI_SUCCESS      The media can now be removed.
	@retval EFI_UNSUPPORTED  The media cannot be removed.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_REMOVABLE_MEDIA_ALLOW_REMOVAL) (
  IN APPLE_REMOVABLE_MEDIA_PROTOCOL  *This
  );

// APPLE_REMOVABLE_MEDIA_PREVENT_REMOVAL
/** Prevent the media from being removed from the drive.

  @param[in] This  Protocol instance pointer.

  @retval EFI_SUCCESS      The drive is locked, and the media cannot be
                           removed.
  @retval EFI_UNSUPPORTED  The drive cannot be locked.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_REMOVABLE_MEDIA_PREVENT_REMOVAL) (
  IN APPLE_REMOVABLE_MEDIA_PROTOCOL  *This
  );

// APPLE_REMOVABLE_MEDIA_DETECT_TRAY_STATE
/** Get the status of the drive tray.

  @param[in]  This      Protocol instance pointer.
	@param[out] TrayOpen  Status of the drive tray.

  @retval EFI_SUCCESS  The status has been returned in TrayOpen.
**/
typedef
EFI_STATUS
(EFIAPI *APPLE_REMOVABLE_MEDIA_DETECT_TRAY_STATE) (
  IN  APPLE_REMOVABLE_MEDIA_PROTOCOL  *This,
  OUT BOOLEAN                         *TrayOpen
  );

//
// Protocol definition
//
typedef struct APPLE_REMOVABLE_MEDIA_PROTOCOL {
  UINT32                                  Revision;
  BOOLEAN                                 RemovalAllowed;
  APPLE_REMOVABLE_MEDIA_EJECT             Eject;
  APPLE_REMOVABLE_MEDIA_INJECT            Inject;
  APPLE_REMOVABLE_MEDIA_ALLOW_REMOVAL     AllowRemoval;
  APPLE_REMOVABLE_MEDIA_PREVENT_REMOVAL   PreventRemoval;
  APPLE_REMOVABLE_MEDIA_DETECT_TRAY_STATE DetectTrayState;

} APPLE_REMOVABLE_MEDIA_PROTOCOL;

extern EFI_GUID gAppleRemovableMediaProtocolGuid;

#endif // APPLE_REMOVABLE_MEDIA_H
