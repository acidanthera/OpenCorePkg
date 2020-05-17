/** @file
  Abstraction of a Text mode or GOP/UGA screen

  Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
**/

#ifndef CONSOLE_CONTROL_H
#define CONSOLE_CONTROL_H

#define EFI_CONSOLE_CONTROL_PROTOCOL_GUID  \
  { 0xF42F7782, 0x012E, 0x4C12,            \
    { 0x99, 0x56, 0x49, 0xF9, 0x43, 0x04, 0xF7, 0x21 } }

typedef struct EFI_CONSOLE_CONTROL_PROTOCOL EFI_CONSOLE_CONTROL_PROTOCOL;

typedef enum {
  EfiConsoleControlScreenText,
  EfiConsoleControlScreenGraphics,
  EfiConsoleControlScreenMaxValue
} EFI_CONSOLE_CONTROL_SCREEN_MODE;

// EFI_CONSOLE_CONTROL_PROTOCOL_GET_MODE
/** Return the current video mode information. Also returns info about
    existence of Graphics Output devices or UGA Draw devices in system, and if
    the Std In device is locked. All the arguments are optional and only
    returned if a non NULL pointer is passed in.

    @param[in]  This          Protocol instance pointer.
    @param[out] Mode          Are we in text of grahics mode.
    @param[out] GopUgaExists  TRUE if Console Spliter has found a GOP or UGA
                              device
    @param[out] StdInLocked   TRUE if StdIn device is keyboard locked

    @retval EFI_SUCCESS  Mode information returned.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_GET_MODE) (
  IN  EFI_CONSOLE_CONTROL_PROTOCOL     *This,
  OUT EFI_CONSOLE_CONTROL_SCREEN_MODE  *Mode,
  OUT BOOLEAN                          *GopUgaExists OPTIONAL,
  OUT BOOLEAN                          *StdInLocked OPTIONAL
  );

// EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE
/** Set the current mode to either text or graphics. Graphics is for Quiet
    Boot.

    @param[in] This  Protocol instance pointer.
    @param[in] Mode  Mode to set the

    @retval EFI_SUCCESS  Mode information returned.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE) (
  IN EFI_CONSOLE_CONTROL_PROTOCOL     *This,
  IN EFI_CONSOLE_CONTROL_SCREEN_MODE  Mode
  );

// EFI_CONSOLE_CONTROL_PROTOCOL_LOCK_STD_IN
/** Lock Std In devices until Password is typed.

  @param[in] This      Protocol instance pointer.
  @param[in] Password  Password needed to unlock screen. NULL means unlock
                       keyboard

  @retval EFI_SUCCESS       Mode information returned.
  @retval EFI_DEVICE_ERROR  Std In not locked
**/
typedef
EFI_STATUS
(EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_LOCK_STD_IN) (
  IN EFI_CONSOLE_CONTROL_PROTOCOL  *This,
  IN CHAR16                        *Password
  );

struct EFI_CONSOLE_CONTROL_PROTOCOL {
  EFI_CONSOLE_CONTROL_PROTOCOL_GET_MODE    GetMode;
  EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE    SetMode;
  EFI_CONSOLE_CONTROL_PROTOCOL_LOCK_STD_IN LockStdIn;
};

extern EFI_GUID gEfiConsoleControlProtocolGuid;

#endif // CONSOLE_CONTROL_H
