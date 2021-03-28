/** @file
  Copyright (C) 2021, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#ifndef NVIDIA_MPC_MEMORY_CONTROLLER_H
#define NVIDIA_MPC_MEMORY_CONTROLLER_H

/**
  I/O port to obtain current DDR frequency on NVIDIA nForce MPC89
  and possibly others. Example value: 1066666664.
**/
#define R_NVIDIA_MPC89_DDR_PLL 0x580

/**
  MMIO base to NVIDIA nForce MPC79 and MPC89 memory controller.
**/
#define B_NVIDIA_MPC_MC_BASE 0xF001B000

/**
  Common PCI identifiers for nForce MPC79 and MPC89 memory controllers.
**/
#define V_NVIDIA_MPC_MC_VENDOR    0x10DE
#define V_NVIDIA_MPC79_MC_DEVICE  0x0A89
#define V_NVIDIA_MPC89_MC_DEVICE  0x0D69

/**
  MPC79 and MPC89 memory controller registers, not much is known.
**/
#define R_NVIDIA_MPC_MC_UN44 0x44
#define R_NVIDIA_MPC_MC_MPCC 0x48
#define R_NVIDIA_MPC_MC_UN78 0x78
#define R_NVIDIA_MPC_MC_MPLM 0xC8
#define R_NVIDIA_MPC_MC_MPLN 0xC9

#define NVIDIA_MPC79_GET_FSB_FREQUENCY_DIVIDEND(Un44, Un78) \
  (25000000ULL * (((Un44) >> 9U) & 0xFFU) * (((Un78) >> 8U) & 0xFFU))

#define NVIDIA_MPC79_GET_FSB_FREQUENCY_DIVISOR(Un44, Un78) \
  (((Un44) & 0xFFU) * ((Un78) & 0xFFU) * (1 << (((Un44) >> 26U) & 3U)))

#endif // NVIDIA_MPC_MEMORY_CONTROLLER_H
