// RP2040 SSI (Synopsys DW_apb_ssi) register offsets and bit definitions
// From RP2040 datasheet section 4.10
#ifndef _HARDWARE_REGS_SSI_H
#define _HARDWARE_REGS_SSI_H

// ── Register offsets from XIP_SSI_BASE ──────────────────────────────────────
#define SSI_CTRLR0_OFFSET         0x00  // Control register 0
#define SSI_CTRLR1_OFFSET         0x04  // Control register 1
#define SSI_SSIENR_OFFSET         0x08  // SSI enable
#define SSI_BAUDR_OFFSET          0x14  // Baud rate
#define SSI_SR_OFFSET             0x28  // Status register
#define SSI_DR0_OFFSET            0x60  // Data register 0
#define SSI_RX_SAMPLE_DLY_OFFSET  0xf0  // RX sample delay
#define SSI_SPI_CTRLR0_OFFSET    0xf4  // SPI control register 0

// ── CTRLR0 bit fields ──────────────────────────────────────────────────────
// DFS_32: data frame size (5-bit field, value = bits_per_frame - 1)
#define SSI_CTRLR0_DFS_32_LSB    16

// TMOD: transfer mode
#define SSI_CTRLR0_TMOD_LSB      8
#define SSI_CTRLR0_TMOD_VALUE_TX_AND_RX      0  // Transmit & receive
#define SSI_CTRLR0_TMOD_VALUE_EEPROM_READ    3  // EEPROM read (send addr, recv data)

// SPI_FRF: SPI frame format
#define SSI_CTRLR0_SPI_FRF_LSB              21
#define SSI_CTRLR0_SPI_FRF_VALUE_QUAD        2  // Quad I/O

// ── SR (status register) bit fields ────────────────────────────────────────
#define SSI_SR_BUSY_BITS  0x01  // Bit 0: SSI busy
#define SSI_SR_TFE_BITS   0x04  // Bit 2: TX FIFO empty

// ── SPI_CTRLR0 bit fields ─────────────────────────────────────────────────
// TRANS_TYPE: address and instruction transfer format
#define SSI_SPI_CTRLR0_TRANS_TYPE_LSB            0
#define SSI_SPI_CTRLR0_TRANS_TYPE_VALUE_1C2A     1  // Command serial, address quad
#define SSI_SPI_CTRLR0_TRANS_TYPE_VALUE_2C2A     2  // Both quad

// ADDR_L: address length in 4-bit units
#define SSI_SPI_CTRLR0_ADDR_L_LSB                2

// INST_L: instruction length
#define SSI_SPI_CTRLR0_INST_L_LSB                8
#define SSI_SPI_CTRLR0_INST_L_VALUE_NONE         0  // No instruction
#define SSI_SPI_CTRLR0_INST_L_VALUE_8B           2  // 8-bit instruction

// WAIT_CYCLES: dummy clock cycles after address
#define SSI_SPI_CTRLR0_WAIT_CYCLES_LSB          11

// XIP_CMD: command to send in XIP continuous read mode
#define SSI_SPI_CTRLR0_XIP_CMD_LSB              24

#endif
