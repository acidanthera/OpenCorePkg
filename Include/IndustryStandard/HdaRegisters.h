/*
 * File: HdaRegisters.h
 *
 * Copyright (c) 2018, 2020 John Davis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EFI_HDA_REGS_H
#define EFI_HDA_REGS_H

#include <Uefi.h>

//
// Global Capabilities, Status, and Control.
//

// Global Capabilities; 2 bytes.
#define HDA_REG_GCAP            0x00
#define HDA_REG_GCAP_64OK       BIT0
#define HDA_REG_GCAP_NSDO(a)    ((UINT8)((a >> 1) & 0x3))
#define HDA_REG_GCAP_BSS(a)     ((UINT8)((a >> 3) & 0x1F))
#define HDA_REG_GCAP_ISS(a)     ((UINT8)((a >> 8) & 0xF))
#define HDA_REG_GCAP_OSS(a)     ((UINT8)((a >> 12) & 0xF))

// Minor Version; 1 byte.
#define HDA_REG_VMIN        0x02

// Major Version; 1 byte.
#define HDA_REG_VMAJ        0x03

// Output Payload Capability; 2 bytes.
#define HDA_REG_OUTPAY      0x04

// Input Payload Capability; 2 bytes.
#define HDA_REG_INPAY       0x06

// Global Control; 4 bytes.
#define HDA_REG_GCTL        0x08
#define HDA_REG_GCTL_CRST   BIT0
#define HDA_REG_GCTL_FCNTRL BIT1
#define HDA_REG_GCTL_UNSOL  BIT8

// Wake Enable; 2 bytes.
#define HDA_REG_WAKEEN      0x0C

// State Change Status; 2 bytes.
#define HDA_REG_STATESTS            0x0E
#define HDA_REG_STATESTS_INDEX(i)   ((UINT16)(1 << (i)))
#define HDA_REG_STATESTS_CLEAR      0xEFFF

// Global Status; 2 bytes
#define HDA_REG_GSTS        0x10
#define HDA_REG_GSTS_FSTS   BIT1

// Output Stream Payload Capability; 2 bytes.
#define HDA_REG_OUTSTRMPAY  0x18

// Input Stream Payload Capability; 2 bytes.
#define HDA_REG_INSTRMPAY   0x1A

//
// Interrupt Status and Control.
//

// Interrupt Control; 4 bytes.
#define HDA_REG_INTCTL      0x20

// Interrupt Status; 4 bytes.
#define HDA_REG_INTSTS      0x24

// Wall Clock Counter; 4 bytes.
#define HDA_REG_WALLCLOCK   0x30

// Stream Synchronization; 4 bytes.
#define HDA_REG_SSYNC       0x38

// CORB Lower Base Address; 4 bytes.
#define HDA_REG_CORBLBASE   0x40

// CORB Upper Base Address; 4 bytes.
#define HDA_REG_CORBUBASE   0x44

// CORB Write Pointer; 2 bytes.
#define HDA_REG_CORBWP      0x48

// CORB Read Pointer; 2 bytes.
#define HDA_REG_CORBRP          0x4A
#define HDA_REG_CORBRP_RP(a)    ((UINT8)a)
#define HDA_REG_CORBRP_RST      BIT15

// CORB Control; 1 byte.
#define HDA_REG_CORBCTL         0x4C
#define HDA_REG_CORBCTL_CMEIE   BIT0
#define HDA_REG_CORBCTL_CORBRUN BIT1

// CORB Status; 1 byte.
#define HDA_REG_CORBSTS         0x4D
#define HDA_REG_CORBSTS_CMEI    BIT0

// CORB Size; 1 byte.
#define HDA_REG_CORBSIZE                0x4E
#define HDA_REG_CORBSIZE_MASK           (BIT0 | BIT1)
#define HDA_REG_CORBSIZE_ENT2           0    // 8 B = 2 entries.
#define HDA_REG_CORBSIZE_ENT16          BIT0 // 64 B = 16 entries.
#define HDA_REG_CORBSIZE_ENT256         BIT1 // 1 KB = 256 entries.
#define HDA_REG_CORBSIZE_CORBSZCAP_2    BIT4 // 8 B = 2 entries.
#define HDA_REG_CORBSIZE_CORBSZCAP_16   BIT5 // 64 B = 16 entries.
#define HDA_REG_CORBSIZE_CORBSZCAP_256  BIT6 // 1 KB = 256 entries.

// RIRB Lower Base Address; 4 bytes.
#define HDA_REG_RIRBLBASE   0x50

// RIRB Upper Base Address; 4 bytes.
#define HDA_REG_RIRBUBASE   0x54

// RIRB Write Pointer; 2 bytes.
#define HDA_REG_RIRBWP          0x58
#define HDA_REG_RIRBWP_WP(a)    ((UINT8)a)
#define HDA_REG_RIRBWP_RST      BIT15

// Response Interrupt Count; 2 bytes.
#define HDA_REG_RINTCNT     0x5A

// RIRB Control; 1 byte.
#define HDA_REG_RIRBCTL             0x5C
#define HDA_REG_RIRBCTL_RINTCTL     BIT0
#define HDA_REG_RIRBCTL_RIRBDMAEN   BIT1
#define HDA_REG_RIRBCTL_RIRBOIC     BIT2

// RIRB Status; 1 byte.
#define HDA_REG_RIRBSTS         0x5D
#define HDA_REG_RIRBSTS_RINTFL  BIT0
#define HDA_REG_RIRBSTS_RIRBOIS BIT2

// RIRB Size; 1 byte.
#define HDA_REG_RIRBSIZE                0x5E
#define HDA_REG_RIRBSIZE_MASK           (BIT0 | BIT1)
#define HDA_REG_RIRBSIZE_ENT2           0    // 16 B = 2 entries.
#define HDA_REG_RIRBSIZE_ENT16          BIT0 // 128 B = 16 entries.
#define HDA_REG_RIRBSIZE_ENT256         BIT1 // 2 KB = 256 entries.
#define HDA_REG_RIRBSIZE_RIRBSZCAP_2    BIT4 // 16 B = 2 entries.
#define HDA_REG_RIRBSIZE_RIRBSZCAP_16   BIT5 // 128 B = 16 entries.
#define HDA_REG_RIRBSIZE_RIRBSZCAP_256  BIT6 // 2 KB = 256 entries.

// DMA Position Lower Base Address; 4 bytes.
#define HDA_REG_DPLBASE         0x70
#define HDA_REG_DPLBASE_EN      BIT0

// DMA Position Upper Base Address; 4 bytes.
#define HDA_REG_DPUBASE         0x74

//
// Immediate Command Input and Output Registers.
//

// Immediate Command Output Interface; 4 bytes.
#define HDA_REG_ICOI        0x60

// Immediate Command Input Interface; 4 bytes.
#define HDA_REG_ICII        0x64

// Immediate Command Status; 2 bytes.
#define HDA_REG_ICIS            0x68
#define HDA_REG_ICIS_ICB        BIT0
#define HDA_REG_ICIS_IRV        BIT1
#define HDA_REG_ICIS_ICV        BIT2
#define HDA_REG_ICIS_IRRUNSOL   BIT3
#define HDA_REG_ICIS_IRRADD(a)  ((UINT8)((a >> 4) & 0xF))

//
// Stream Descriptors.
//

// Input/Output/Bidirectional Stream Descriptor n Control; 3 bytes.
// Byte 1.
#define HDA_REG_SDNCTL1(n)      (0x80 + (0x20 * (n)))
#define HDA_REG_SDNCTL1_SRST    BIT0
#define HDA_REG_SDNCTL1_RUN     BIT1
#define HDA_REG_SDNCTL1_IOCE    BIT2
#define HDA_REG_SDNCTL1_FEIE    BIT3
#define HDA_REG_SDNCTL1_DEIE    BIT4

// Byte 2.
#define HDA_REG_SDNCTL2(n)      (0x81 + (0x20 * (n)))

// Byte 3.
#define HDA_REG_SDNCTL3(n)              (0x82 + (0x20 * (n)))
#define HDA_REG_SDNCTL3_TP              BIT2
#define HDA_REG_SDNCTL3_DIR             BIT3
#define HDA_REG_SDNCTL3_STRM_GET(a)     ((UINT8)((a >> 4) & 0xF))
#define HDA_REG_SDNCTL3_STRM_SET(a, s)  ((UINT8)(((a) & 0x0F) | (((s) & 0xF)) << 4))

// Input/Output/Bidirectional Stream Descriptor n Status; 1 byte.
#define HDA_REG_SDNSTS(n)       (0x83 + (0x20 * (n)))
#define HDA_REG_SDNSTS_BCIS     BIT2
#define HDA_REG_SDNSTS_FIFOE    BIT3
#define HDA_REG_SDNSTS_DESE     BIT4
#define HDA_REG_SDNSTS_FIFORDY  BIT5

// Input/Output/Bidirectional Stream Descriptor n Link Position in Buffer; 4 bytes.
#define HDA_REG_SDNLPIB(n)      (0x84 + (0x20 * (n)))

// Input/Output/Bidirectional Stream Descriptor n Cyclic Buffer Length; 4 bytes.
#define HDA_REG_SDNCBL(n)       (0x88 + (0x20 * (n)))

// Input/Output/Bidirectional Stream Descriptor n Last Valid Index; 2 bytes.
#define HDA_REG_SDNLVI(n)       (0x8C + (0x20 * (n)))

// Input/Output/Bidirectional Stream Descriptor n FIFO Size; 2 bytes.
#define HDA_REG_SDNFIFOS(n)     (0x90 + (0x20 * (n)))

// Input/Output/Bidirectional Stream Descriptor n Format; 2 bytes.
#define HDA_REG_SDNFMT(n)           (0x92 + (0x20 * (n)))
#define HDA_REG_SDNFMT_CHAN(a)      ((UINT8)((a) & 0xF))
#define HDA_REG_SDNFMT_BITS(a)      ((UINT8)(((a) >> 4) & 0x3))
#define HDA_REG_SDNFMT_BITS_8       0x0
#define HDA_REG_SDNFMT_BITS_16      0x1
#define HDA_REG_SDNFMT_BITS_20      0x2
#define HDA_REG_SDNFMT_BITS_24      0x3
#define HDA_REG_SDNFMT_BITS_32      0x4
#define HDA_REG_SDNFMT_DIV(a)       ((UINT8)(((a) >> 8) & 0x3))
#define HDA_REG_SDNFMT_MULT(a)      ((UINT8)(((a) >> 11) & 0x3))
#define HDA_REG_SDNFMT_BASE_44KHZ   BIT14
#define HDA_REG_SDNFMT_SET(chan, bits, div, mult, base) \
  ((UINT16)(((chan) & 0xF) | (((bits) & 0x3) << 4) | (((div) & 0x3) << 8) | \
  (((mult) & 0x3) << 11) | ((base) ? HDA_REG_SDNFMT_BASE_44KHZ : 0)))

// Input/Output/Bidirectional Stream Descriptor n BDL Pointer Lower Base Address; 4 bytes.
#define HDA_REG_SDNBDPL(n)          (0x98 + (0x20 * (n)))

// Input/Output/Bidirectional Stream Descriptor n BDL Pointer Upper Base Address; 4 bytes.
#define HDA_REG_SDNBDPU(n)          (0x9C + (0x20 * (n)))

// Wall Clock Counter Alias; 4 bytes.
#define HDA_REG_WALCLKA             0x2030

// Input/Output/Bidirectional Stream Descriptor n Link Position in Buffer Alias; 4 bytes.
#define HDA_REG_SDNLPIBA(n)         (0x2084 + (0x20 * (n)))


//
// Ring buffer register offsets (CORB and RIRB).
//
#define HDA_REG_CORB_BASE                 0x40
#define HDA_REG_RIRB_BASE                 0x50

#define HDA_OFFSET_RING_BASE              0x00
#define HDA_OFFSET_RING_UBASE             0x04
#define HDA_OFFSET_RING_WP                0x08
#define HDA_OFFSET_RING_RP                0x0A

#define HDA_OFFSET_RING_CTL               0x0C
#define HDA_OFFSET_RING_CTL_RUN           BIT1

#define HDA_OFFSET_RING_STS               0x0D

#define HDA_OFFSET_RING_SIZE              0x0E
#define HDA_OFFSET_RING_SIZE_MASK         (BIT0 | BIT1)
#define HDA_OFFSET_RING_SIZE_ENT2         0    // 2 entries.
#define HDA_OFFSET_RING_SIZE_ENT16        BIT0 // 16 entries.
#define HDA_OFFSET_RING_SIZE_ENT256       BIT1 // 256 entries.
#define HDA_OFFSET_RING_SIZE_SZCAP_2      BIT4 // 2 entries.
#define HDA_OFFSET_RING_SIZE_SZCAP_16     BIT5 // 16 entries.
#define HDA_OFFSET_RING_SIZE_SZCAP_256    BIT6 // 256 entries.


#endif // EFI_HDA_REGS_H
