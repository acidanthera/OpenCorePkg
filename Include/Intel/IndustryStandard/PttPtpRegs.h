/** @file
  Register definitions for PTT HCI (Platform Trust Technology - Host Controller Interface).

  Conventions:
    Prefixes:
    Definitions beginning with "R_" are registers
    Definitions beginning with "B_" are bits within registers
    Definitions beginning with "V_" are meaningful values of bits within the registers
    Definitions beginning with "S_" are register sizes
    Definitions beginning with "N_" are the bit position

  Copyright (c) 2012 - 2016, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PTT_HCI_REGS_H_
#define _PTT_HCI_REGS_H_

//
// FTPM HCI register base address
//
#define R_PTT_HCI_BASE_ADDRESS_A            0xFED40000
#define R_PTT_HCI_BASE_ADDRESS_B            0xFED70000

//
// FTPM HCI Control Area
//
#define R_PTT_LOCALITY_STATE                0x00
#define R_TPM_LOCALITY_CONTROL              0X08
#define R_TPM_LOCALITY_STATUS               0x0C
#define R_TPM_INTERFACE_ID                  0x30
#define R_CRB_CONTROL_EXT                   0x38
#define R_CRB_CONTROL_REQ                   0x40
#define R_CRB_CONTROL_STS                   0x44
#define R_CRB_CONTROL_CANCEL                0x48
#define R_CRB_CONTROL_START                 0x4C
#define R_CRB_CONTROL_INT                   0x50
#define R_CRB_CONTROL_CMD_SIZE              0x58
#define R_CRB_CONTROL_CMD_LOW               0x5C
#define R_CRB_CONTROL_CMD_HIGH              0x60
#define R_CRB_CONTROL_RESPONSE_SIZE         0x64
#define R_CRB_CONTROL_RESPONSE_ADDR         0x68

//
// R_CRB_CONTROL_STS Bits
//
#define B_CRB_CONTROL_STS_TPM_STATUS             0x00000001 ///< BIT0
#define B_CRB_CONTROL_STS_TPM_IDLE               0x00000002 ///< BIT1

//
// R_CRB_CONTROL_REQ Bits
//
#define B_R_CRB_CONTROL_REQ_COMMAND_READY        0x00000001 ///< BIT0
#define B_R_CRB_CONTROL_REQ_GO_IDLE              0x00000002 ///< BIT1

//
// R_CRB_CONTROL_START Bits
//
#define B_CRB_CONTROL_START                  0x00000001 ///< BIT0

//
// R_TPM_LOCALITY_STATUS Bits
//
#define B_CRB_LOCALITY_STS_GRANTED               0x00000001 ///< BIT0
#define B_CRB_LOCALITY_STS_BEEN_SEIZED           0x00000002 ///< BIT1

//
// R_TPM_LOCALITY_CONTROL Bits
//
#define B_CRB_LOCALITY_CTL_REQUEST_ACCESS       0x00000001 ///< BIT0
#define B_CRB_LOCALITY_CTL_RELINQUISH           0x00000002 ///< BIT1
#define B_CRB_LOCALITY_CTL_SEIZE                0x00000004 ///< BIT2

//
// R_PTT_LOCALITY_STATE Bits
//
#define B_CRB_LOCALITY_STATE_TPM_ESTABLISHED    0x00000001 ///< BIT0
#define B_CRB_LOCALITY_STATE_LOCALITY_ASSIGNED  0x00000002 ///< BIT1
#define B_CRB_LOCALITY_STATE_REGISTER_VALID     0x00000080 ///< BIT7

//
// R_PTT_LOCALITY_STATE Mask Values
//
#define V_CRB_LOCALITY_STATE_ACTIVE_LOC_MASK    0x0000001C /// Bits [4:2]

//
// Value written to R_PTT_HCI_CMD and CA_START
// to indicate that a command is available for processing
//
#define V_PTT_HCI_COMMAND_AVAILABLE_START  0x00000001
#define V_PTT_HCI_COMMAND_AVAILABLE_CMD    0x00000000
#define V_PTT_HCI_BUFFER_ADDRESS_RDY       0x00000003

//
// Ignore bit setting mask for WaitRegisterBits
//
#define V_PTT_HCI_IGNORE_BITS              0x00000000

//
// All bits clear mask for WaitRegisterBits
//
#define V_PTT_HCI_ALL_BITS_CLEAR           0xFFFFFFFF
#define V_PTT_HCI_START_CLEAR              0x00000001

//
// Max FTPM command/reponse buffer length
//
#define S_PTT_HCI_CRB_LENGTH               3968 ///< 0xFED40080:0xFED40FFF = 3968 Bytes

#endif

