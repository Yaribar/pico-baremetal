// RP2040 base addresses (from datasheet section 2.2)
#ifndef _HARDWARE_REGS_ADDRESSMAP_H
#define _HARDWARE_REGS_ADDRESSMAP_H

#define XIP_BASE        0x10000000  // Flash XIP (execute-in-place) window
#define XIP_SSI_BASE    0x18000000  // QSPI flash SSI controller
#define PADS_QSPI_BASE  0x40020000  // QSPI pad control
#define PPB_BASE        0xe0000000  // Cortex-M0+ Private Peripheral Bus

#endif
