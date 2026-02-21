// RP2040 PADS_QSPI register offsets and bit definitions (from datasheet section 2.19.6.4)
#ifndef _HARDWARE_REGS_PADS_QSPI_H
#define _HARDWARE_REGS_PADS_QSPI_H

// Register offsets from PADS_QSPI_BASE
#define PADS_QSPI_GPIO_QSPI_SCLK_OFFSET  0x04
#define PADS_QSPI_GPIO_QSPI_SD0_OFFSET   0x08
#define PADS_QSPI_GPIO_QSPI_SD1_OFFSET   0x0c
#define PADS_QSPI_GPIO_QSPI_SD2_OFFSET   0x10
#define PADS_QSPI_GPIO_QSPI_SD3_OFFSET   0x14

// Pad register bit fields (same layout for all QSPI pads)
// Bit 0: SLEWFAST — set for fast slew rate
// Bit 1: SCHMITT  — enable Schmitt trigger
// Bits 5:4: DRIVE — drive strength (0=2mA, 1=4mA, 2=8mA, 3=12mA)
#define PADS_QSPI_GPIO_QSPI_SCLK_DRIVE_LSB      4
#define PADS_QSPI_GPIO_QSPI_SCLK_SLEWFAST_BITS   0x01
#define PADS_QSPI_GPIO_QSPI_SD0_SCHMITT_BITS      0x02

#endif
