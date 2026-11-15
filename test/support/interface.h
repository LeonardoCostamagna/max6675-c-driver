/**
 * @file interface.h
 * @brief Abstract interface for CMock generation (MAX6675 SPI driver).
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>

/**
 * @brief Mock function for SPI stream reading (replaces mock_i2c_read).
 * @param handle Peripheral or device instance pointer.
 * @param reg    Dummy register address (unused by MAX6675, kept for signature compatibility).
 * @param data   Pointer to receive buffer (expects 2 bytes / 16-bit frame).
 * @param len    Length in bytes (typically 2 for MAX6675).
 * @return Execution status (0 on success, < 0 on communication error).
 */
int32_t mock_spi_read(void *handle, uint8_t reg, uint8_t *data, uint16_t len);

/**
 * @brief Mock function for SPI stream writing (optional/unused since MAX6675 is read-only).
 * @param handle Peripheral or device instance pointer.
 * @param reg    Dummy register address.
 * @param data   Pointer to transmit data.
 * @param len    Length in bytes.
 * @return Execution status (0 on success, < 0 on error).
 */
int32_t mock_spi_write(void *handle, uint8_t reg, const uint8_t *data, uint16_t len);

#endif // INTERFACE_H