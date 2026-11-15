/**
  * @file    max6675.c
  * @author  Leonardo Costamagna
  * @brief   MAX6675 Thermocouple-to-Digital Converter driver implementation.
  */

#include "max6675.h"

/* Low-level SPI Frame Read Weak Function */

int32_t __weak max6675_read_raw(const dev_ctx_t *ctx, uint8_t *data, uint16_t len)
{
  if (ctx == NULL || ctx->read_reg == NULL || data == NULL)
  {
    return -1;
  }

  /* Note: Platform read_reg wrapper must handle CS assertion/de-assertion around SPI transfer */
  return ctx->read_reg(ctx->handle, MAX6675_DUMMY_REG, data, len);
}

/* Helper Utilities */

float_t max6675_from_lsb_to_celsius(uint16_t lsb)
{
  return ((float_t)(lsb & MAX6675_TEMP_MASK) * MAX6675_RESOLUTION_C);
}

/* Temperature Measurements */

int32_t max6675_temperature_raw_get(const dev_ctx_t *ctx, uint16_t *buff)
{
  uint8_t rx_buf[2];
  uint16_t raw_word;
  int32_t ret;

  if (buff == NULL)
  {
    return -1;
  }

  ret = max6675_read_raw(ctx, rx_buf, 2);
  if (ret == 0)
  {
    /* Reconstruct 16-bit frame ensuring MSB-first endianness independence */
    raw_word = ((uint16_t)rx_buf[0] << 8) | (uint16_t)rx_buf[1];

    /* Check bit D2 for open/disconnected thermocouple fault */
    if ((raw_word & MAX6675_OPEN_TC_MASK) != 0U)
    {
      return -2;
    }

    *buff = (raw_word >> MAX6675_TEMP_SHIFT) & MAX6675_TEMP_MASK;
  }

  return ret;
}

int32_t max6675_temperature_get(const dev_ctx_t *ctx, float_t *val)
{
  uint16_t raw_temp = 0U;
  int32_t ret;

  if (val == NULL)
  {
    return -1;
  }

  ret = max6675_temperature_raw_get(ctx, &raw_temp);
  if (ret == 0)
  {
    *val = max6675_from_lsb_to_celsius(raw_temp);
  }

  return ret;
}

/* Device Status Management */

int32_t max6675_status_get(const dev_ctx_t *ctx, max6675_stat_t *val)
{
  uint8_t rx_buf[2];
  max6675_reg_t reg;
  int32_t ret;

  if (val == NULL)
  {
    return -1;
  }

  ret = max6675_read_raw(ctx, rx_buf, 2);
  if (ret == 0)
  {
    reg.word = ((uint16_t)rx_buf[0] << 8) | (uint16_t)rx_buf[1];

    val->open_tc   = (uint8_t)reg.frame.tc_input;
    val->device_id = (uint8_t)reg.frame.device_id;
  }

  return ret;
}