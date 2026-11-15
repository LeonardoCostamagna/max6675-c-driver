/**
  * @file    max6675.h
  * @author  Leonardo Costamagna
  * @brief   Header file containing API declarations, bit-field definitions,
  *          and context structures for the MAX6675 thermocouple driver.
  */

#ifndef MAX6675_H
#define MAX6675_H

/* Ensures C linkage compatibility when compiled with C++, preventing name mangling. */
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <math.h>

/**
  * @defgroup MAX6675_Driver MAX6675 Thermocouple-to-Digital Converter Driver
  * @{
  */

/**
  * @defgroup MAX6675_Endianness Byte Ordering Definitions
  * @brief Configures target architecture byte alignment.
  * @{
  */

/* Detects target endianness to ensure portable and correct bit-field word mapping. */
#ifndef DRV_BYTE_ORDER
#ifndef __BYTE_ORDER__

/* Fallback definitions when toolchain does not supply built-in byte order macros */
#define DRV_LITTLE_ENDIAN 1234
#define DRV_BIG_ENDIAN    4321
#define DRV_BYTE_ORDER    DRV_LITTLE_ENDIAN

#else /* defined __BYTE_ORDER__ */

#define DRV_LITTLE_ENDIAN  __ORDER_LITTLE_ENDIAN__
#define DRV_BIG_ENDIAN     __ORDER_BIG_ENDIAN__
#define DRV_BYTE_ORDER     __BYTE_ORDER__

#endif /* __BYTE_ORDER__ */
#endif /* DRV_BYTE_ORDER */

/**
  * @} MAX6675_Endianness
  */

/**
  * @defgroup MAX6675_Bus_Abstraction Hardware Bus Context & Callbacks
  * @brief Low-level function pointers and device context abstraction.
  * @{
  */

/* Shared include guard across sensor/hardware drivers to prevent type redefinition errors for common base structures and bus callbacks. */
#ifndef MEMS_SHARED_TYPES
#define MEMS_SHARED_TYPES

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint8_t bit0       : 1;
  uint8_t bit1       : 1;
  uint8_t bit2       : 1;
  uint8_t bit3       : 1;
  uint8_t bit4       : 1;
  uint8_t bit5       : 1;
  uint8_t bit6       : 1;
  uint8_t bit7       : 1;
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint8_t bit7       : 1;
  uint8_t bit6       : 1;
  uint8_t bit5       : 1;
  uint8_t bit4       : 1;
  uint8_t bit3       : 1;
  uint8_t bit2       : 1;
  uint8_t bit1       : 1;
  uint8_t bit0       : 1;
#endif /* DRV_BYTE_ORDER */
} bitwise_t;

#define PROPERTY_DISABLE                (0U)
#define PROPERTY_ENABLE                 (1U)

typedef int32_t (*dev_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*dev_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);
typedef void (*dev_mdelay_ptr)(uint32_t millisec);

typedef struct
{
  /** Bus write callback */
  dev_write_ptr    write_reg;
  /** Bus read callback */
  dev_read_ptr     read_reg;
  /** Millisecond delay handler (optional) */
  dev_mdelay_ptr   mdelay;
  /** Hardware peripheral handler reference */
  void               *handle;
  /** Reserved private driver payload */
  void               *priv_data;
} dev_ctx_t;

#endif /* MEMS_SHARED_TYPES */

/**
  * @} MAX6675_Bus_Abstraction
  */

/**
  * @defgroup MAX6675_Device_Constants Device Constants & Masks
  * @{
  */

/** Dummy register address for SPI read-only stream compatibility */
#define MAX6675_DUMMY_REG                0x00U

/** Fixed resolution in degrees Celsius per LSB (0.25 °C) */
#define MAX6675_RESOLUTION_C             0.25f

/** Bit D2 mask: High when thermocouple input is open */
#define MAX6675_OPEN_TC_MASK             0x0004U

/** Temperature data bit-shift (Bits D14..D3) and 12-bit mask */
#define MAX6675_TEMP_SHIFT               3U
#define MAX6675_TEMP_MASK                0x0FFFU

/**
  * @} MAX6675_Device_Constants
  */

/**
  * @defgroup MAX6675_Data_Frame SPI 16-Bit Output Frame Mapping
  * @{
  */

typedef struct
{
#if DRV_BYTE_ORDER == DRV_LITTLE_ENDIAN
  uint16_t state           : 1;  /**< D0: Three-state */
  uint16_t device_id       : 1;  /**< D1: Device ID (always 0) */
  uint16_t tc_input        : 1;  /**< D2: Thermocouple input (1 = open) */
  uint16_t temp_data       : 12; /**< D14..D3: 12-bit temperature reading */
  uint16_t dummy_sign      : 1;  /**< D15: Dummy sign bit (always 0) */
#elif DRV_BYTE_ORDER == DRV_BIG_ENDIAN
  uint16_t dummy_sign      : 1;  /**< D15: Dummy sign bit (always 0) */
  uint16_t temp_data       : 12; /**< D14..D3: 12-bit temperature reading */
  uint16_t tc_input        : 1;  /**< D2: Thermocouple input (1 = open) */
  uint16_t device_id       : 1;  /**< D1: Device ID (always 0) */
  uint16_t state           : 1;  /**< D0: Three-state */
#endif /* DRV_BYTE_ORDER */
} max6675_raw_word_t;

typedef union
{
  max6675_raw_word_t frame;
  bitwise_t          bitwise[2];
  uint8_t            byte[2];
  uint16_t           word;
} max6675_reg_t;

/**
  * @} MAX6675_Data_Frame
  */

#ifndef __weak
#define __weak __attribute__((weak))
#endif /* __weak */

/**
  * @defgroup MAX6675_Low_Level Low-Level Communication Prototypes
  * @brief Weak read wrapper for target HAL integration.
  * @{
  */

int32_t max6675_read_raw(const dev_ctx_t *ctx, uint8_t *data, uint16_t len);

/**
  * @} MAX6675_Low_Level
  */

/**
  * @defgroup MAX6675_Helper_Utilities Format Conversion Helpers
  * @brief Temperature scaling and unit conversion routines.
  * @{
  */

extern float_t max6675_from_lsb_to_celsius(uint16_t lsb);

/**
  * @} MAX6675_Helper_Utilities
  */

/**
  * @defgroup MAX6675_Core_API High-Level Application Interface
  * @brief Primary API structures and function declarations.
  * @{
  */

/**
  * @brief Unpacked status flags structure.
  */
typedef struct
{
  uint8_t open_tc          : 1;  /**< Thermocouple disconnected flag (Bit D2) */
  uint8_t device_id        : 1;  /**< Device ID bit state (Bit D1, expected 0) */
} max6675_stat_t;

/**
  * @brief  Reads unscaled raw 12-bit temperature value from MAX6675 SPI frame.
  * @param  ctx   Pointer to driver device context.
  * @param  buff  Pointer to variable storing 12-bit right-aligned raw output.
  * @return Execution status (0: Success, -1: Communication error, -2: Open thermocouple).
  */
int32_t max6675_temperature_raw_get(const dev_ctx_t *ctx, uint16_t *buff);

/**
  * @brief  Reads thermocouple sensor and returns temperature in degrees Celsius.
  * @param  ctx  Pointer to driver device context.
  * @param  val  Pointer to float output variable storing converted temperature.
  * @return Execution status (0: Success, -1: Communication error, -2: Open thermocouple).
  */
int32_t max6675_temperature_get(const dev_ctx_t *ctx, float_t *val);

/**
  * @brief  Retrieves current diagnostic status flags (open thermocouple & ID) from device.
  * @param  ctx  Pointer to driver device context.
  * @param  val  Pointer to target structure storing status flags.
  * @return Execution status (0: Success, <0: Communication failure).
  */
int32_t max6675_status_get(const dev_ctx_t *ctx, max6675_stat_t *val);

/**
  * @} MAX6675_Core_API
  */

/**
  * @} MAX6675_Driver
  */

#ifdef __cplusplus
}
#endif

#endif /* MAX6675_H */