/**
 * @file test_max6675.c
 * @brief Unit tests for the MAX6675 thermocouple driver using Ceedling, Unity, and CMock.
 */

#include "unity.h"
#include "max6675.h"
#include "mock_interface.h"

static dev_ctx_t ctx;
static uint32_t dummy_spi_handle = 0x5678;

void setUp(void) {
    ctx.write_reg = NULL;             /* MAX6675 is read-only */
    ctx.read_reg  = mock_spi_read;    /* Reuses mock read interface for SPI stream / dummy register */
    ctx.mdelay    = NULL;
    ctx.handle    = &dummy_spi_handle;
    ctx.priv_data = NULL;
}

void tearDown(void) {
}

/* ============================================================================
   1. UTILITY AND CONVERSION TESTS (Pure Functions)
   ============================================================================ */

void test_max6675_from_lsb_to_celsius(void) {
    /* Zero Value: 0 LSB -> 0.0 °C */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, max6675_from_lsb_to_celsius(0));

    /* Positive Temperature: 25.0 °C -> LSB = 25 / 0.25 = 100 (0x0064) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, max6675_from_lsb_to_celsius(100));

    /* Positive Temperature with Fractional Part: +25.75 °C -> 25.75 / 0.25 = 103 (0x0067) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.75f, max6675_from_lsb_to_celsius(103));

    /* Maximum Scale Test: +1023.75 °C -> 0x0FFF (4095) */
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1023.75f, max6675_from_lsb_to_celsius(0x0FFF));
}

/* ============================================================================
   2. DEFENSIVE PROGRAMMING TESTS (NULL Pointers & Arguments)
   ============================================================================ */

void test_max6675_null_context_and_pointer_guards(void) {
    uint8_t dummy_buf[2];
    uint16_t raw_data;
    float_t temp;
    max6675_stat_t stat;

    /* Low-level read_raw guards */
    TEST_ASSERT_EQUAL_INT32(-1, max6675_read_raw(NULL, dummy_buf, 2));
    TEST_ASSERT_EQUAL_INT32(-1, max6675_read_raw(&ctx, NULL, 2));

    /* High-level API NULL pointer guards */
    TEST_ASSERT_EQUAL_INT32(-1, max6675_temperature_raw_get(NULL, &raw_data));
    TEST_ASSERT_EQUAL_INT32(-1, max6675_temperature_raw_get(&ctx, NULL));

    TEST_ASSERT_EQUAL_INT32(-1, max6675_temperature_get(NULL, &temp));
    TEST_ASSERT_EQUAL_INT32(-1, max6675_temperature_get(&ctx, NULL));

    TEST_ASSERT_EQUAL_INT32(-1, max6675_status_get(NULL, &stat));
    TEST_ASSERT_EQUAL_INT32(-1, max6675_status_get(&ctx, NULL));
}

/* ============================================================================
   3. TEMPERATURE MEASUREMENTS (max6675_temperature_raw_get / max6675_temperature_get)
   ============================================================================ */

void test_max6675_temperature_get_success(void) {
    float_t temp_celsius = 0.0f;
    /* Simulated 16-bit SPI frame:
       Bits D14..D3 = 100 (0x0064 -> 25.0 °C) shifted << 3 -> 0x0320
       Bit D2 (open TC) = 0, Bit D1 (ID) = 0, Bit D0 (state) = 0
       Big Endian bytes to receive: rx_buf[0] = 0x03, rx_buf[1] = 20 (0x14) -> 0x0314 ... wait:
       Let's construct raw word = (100 << 3) | 0 = 0x0320.
       rx_buf[0] = 0x03, rx_buf[1] = 0x20. */
    uint8_t simulated_spi_frame[2] = { 0x03, 0x20 };

    mock_spi_read_ExpectAndReturn(&dummy_spi_handle, MAX6675_DUMMY_REG, NULL, 2, 0);
    mock_spi_read_IgnoreArg_data();
    mock_spi_read_ReturnArrayThruPtr_data(simulated_spi_frame, 2);

    TEST_ASSERT_EQUAL_INT32(0, max6675_temperature_get(&ctx, &temp_celsius));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, temp_celsius);
}

void test_max6675_temperature_get_fractional_success(void) {
    float_t temp_celsius = 0.0f;
    /* +25.75 °C -> LSB = 103 (0x0067).
       Raw word = (103 << 3) = 0x0338.
       rx_buf[0] = 0x03, rx_buf[1] = 0x38. */
    uint8_t simulated_spi_frame[2] = { 0x03, 0x38 };

    mock_spi_read_ExpectAndReturn(&dummy_spi_handle, MAX6675_DUMMY_REG, NULL, 2, 0);
    mock_spi_read_IgnoreArg_data();
    mock_spi_read_ReturnArrayThruPtr_data(simulated_spi_frame, 2);

    TEST_ASSERT_EQUAL_INT32(0, max6675_temperature_get(&ctx, &temp_celsius));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.75f, temp_celsius);
}

/* ============================================================================
   4. FAULT DETECTION (Open / Disconnected Thermocouple)
   ============================================================================ */

void test_max6675_open_thermocouple_fault_detection(void) {
    float_t temp_celsius = 0.0f;
    uint16_t raw_data = 0;
    /* Bit D2 is set (MAX6675_OPEN_TC_MASK = 0x0004).
       Let's simulate raw word = 0x0324 (Temp data + D2 high).
       rx_buf[0] = 0x03, rx_buf[1] = 0x24. */
    uint8_t simulated_fault_frame[2] = { 0x03, 0x24 };

    mock_spi_read_ExpectAndReturn(&dummy_spi_handle, MAX6675_DUMMY_REG, NULL, 2, 0);
    mock_spi_read_IgnoreArg_data();
    mock_spi_read_ReturnArrayThruPtr_data(simulated_fault_frame, 2);

    /* Must return -2 indicating open thermocouple fault */
    TEST_ASSERT_EQUAL_INT32(-2, max6675_temperature_raw_get(&ctx, &raw_data));

    mock_spi_read_ExpectAndReturn(&dummy_spi_handle, MAX6675_DUMMY_REG, NULL, 2, 0);
    mock_spi_read_IgnoreArg_data();
    mock_spi_read_ReturnArrayThruPtr_data(simulated_fault_frame, 2);

    TEST_ASSERT_EQUAL_INT32(-2, max6675_temperature_get(&ctx, &temp_celsius));
}

/* ============================================================================
   5. DEVICE STATUS AND DIAGNOSTICS MANAGEMENT
   ============================================================================ */

void test_max6675_status_get_success(void) {
    max6675_stat_t stat;
    /* Simulate frame where open_tc (Bit D2) = 1, device_id (Bit D1) = 0.
       Raw word = 0x0004 (Bit D2 high).
       rx_buf[0] = 0x00, rx_buf[1] = 0x04. */
    uint8_t simulated_status_frame[2] = { 0x00, 0x04 };

    mock_spi_read_ExpectAndReturn(&dummy_spi_handle, MAX6675_DUMMY_REG, NULL, 2, 0);
    mock_spi_read_IgnoreArg_data();
    mock_spi_read_ReturnArrayThruPtr_data(simulated_status_frame, 2);

    TEST_ASSERT_EQUAL_INT32(0, max6675_status_get(&ctx, &stat));
    TEST_ASSERT_EQUAL_UINT8(1, stat.open_tc);
    TEST_ASSERT_EQUAL_UINT8(0, stat.device_id);
}

/* ============================================================================
   6. SPI BUS ERROR HANDLING AND PROPAGATION
   ============================================================================ */

void test_max6675_temperature_get_bus_error_propagation(void) {
    float_t temp_celsius = 0.0f;

    /* Simulate SPI communication error (-1) */
    mock_spi_read_ExpectAndReturn(&dummy_spi_handle, MAX6675_DUMMY_REG, NULL, 2, -1);
    mock_spi_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL_INT32(-1, max6675_temperature_get(&ctx, &temp_celsius));
}

void test_max6675_status_get_bus_error_propagation(void) {
    max6675_stat_t stat;

    /* Simulate SPI communication error (-1) */
    mock_spi_read_ExpectAndReturn(&dummy_spi_handle, MAX6675_DUMMY_REG, NULL, 2, -1);
    mock_spi_read_IgnoreArg_data();

    TEST_ASSERT_EQUAL_INT32(-1, max6675_status_get(&ctx, &stat));
}