# MAX6675 Driver

## Overview

The **MAX6675** is a cold-junction-compensated K-type thermocouple-to-digital converter that digitizes the signal from a Type-K thermocouple and outputs the data in a 12-bit resolution, SPI-compatible, read-only format. This converter resolves temperatures to **0.25°C**, allows readings as high as **+1024°C**, and exhibits thermocouple accuracy of 8 LSBs for temperatures ranging from 0°C to +700°C. Additionally, it features built-in open-thermocouple fault detection (Bit `D2`), making it ideal for industrial, medical, and embedded thermal monitoring applications.

### Key Features

* **High Portability & Shared Abstraction:** Complete communication layer abstraction using the standardized `dev_ctx_t` context structure, allowing multi-sensor coexistence (e.g., DS3231 and MAX6675) without type redefinition conflicts.
* **Dual Integration Schemes:** Supports both **Dynamic Callbacks** via the context structure and **Static Redefinition** using `__weak` function overrides for seamless integration into any platform HAL.
* **Robust Design Patterns:** Implements *Hardware Proxy* and *Opaque Handles* patterns alongside cross-architecture endianness mapping (`DRV_BYTE_ORDER`) to ensure predictable memory usage and portable bit-field register parsing.
* **Quality & Verification:** Designed for hardware-decoupled unit testing via Ceedling/Unity, featuring full Doxygen-compatible inline documentation.

---

## Project Structure

```text
max6675/
├── docs/                 # Doxygen documentation and configuration file
├── max6675.h             # Public interface, bit fields, and driver declarations
├── max6675.c             # Driver logic implementation
├── test/                 # Unit testing environment
│   ├── support/          # Auxiliary headers and mocks
│   ├── project.yml       # Ceedling configuration
│   └── test_max6675.c    # Unit test suite
└── README.md             # Main project documentation

```

---

## Integration and Getting Started

### Integration Schemes

The driver offers two independent mechanisms to connect with the target platform's SPI HAL:

#### Option A: Via Function Pointers / Callbacks (Dynamic)

Transport functions (`read_reg`) are defined and assigned to members of the `dev_ctx_t` context structure at runtime. **Does not require** overriding the `__weak` functions.

#### Option B: Via `__weak` Function Overriding (Static)

Functions declared with the `__weak` attribute (`max6675_read_raw`) are re-implemented directly in the application code or BSP layer to handle SPI communication and Chip Select (CS) toggling.

---

### Integration Steps

1. Copy `max6675.h` and `max6675.c` into your project's directory tree and include them in your build system (Makefile, CMake, IDE).
2. Select the integration option that best fits your architecture (Option A or Option B).
3. Instantiate the `dev_ctx_t` context structure, associating the generic pointer to your microcontroller's SPI peripheral handle.

---

### Code Examples

#### Example 1: Integration via Callbacks (Option A)

```c
#include <stdio.h>
#include "max6675.h"

/* 1. Definition of platform-specific wrapper function handling SPI receive & CS toggling */
static int32_t platform_spi_read(void *handle, uint8_t reg, uint8_t *data, uint16_t len)
{
    /* Integration with native platform HAL:
     * 1. Drive CS Pin Low (Active)
     * 2. Receive 16-bit (2 bytes) SPI stream into data buffer
     * 3. Drive CS Pin High (Idle / Trigger next internal conversion)
     * Return 0 on success, < 0 on error.
     */
    return 0; 
}

int main(void)
{
    /* Pointer/handle to MCU SPI peripheral (e.g., &hspi1) */
    void *mcu_spi_handle = (void *)0x40013000; 

    /* Context declaration and initialization */
    dev_ctx_t dev_ctx;
    dev_ctx.write_reg = NULL;             /* MAX6675 is read-only */
    dev_ctx.read_reg  = platform_spi_read;
    dev_ctx.mdelay    = NULL;             /* Optional delay handler */
    dev_ctx.handle    = mcu_spi_handle;
    dev_ctx.priv_data = NULL;

    float current_temp = 0.0f;

    /* Read temperature (returns 0 on success, -1 on comm error, -2 on open thermocouple) */
    if (max6675_temperature_get(&dev_ctx, &current_temp) == 0) {
        printf("Temperature: %.2f °C\n", current_temp);
    } else {
        printf("Error reading temperature or thermocouple disconnected.\n");
    }

    return 0;
}

```

#### Example 2: Integration via `__weak` Redefinition (Option B)

```c
#include "max6675.h"

/* Direct override of the weak read raw function */
int32_t max6675_read_raw(const dev_ctx_t *ctx, uint8_t *data, uint16_t len)
{
    if (ctx == NULL || ctx->handle == NULL || data == NULL) {
        return -1;
    }

    /* Direct call to platform HAL using ctx->handle */
    /* Hypothetical example using STM32 HAL:
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)ctx->handle;
    
    // Assert CS Low
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    
    HAL_StatusTypeDef status = HAL_SPI_Receive(hspi, data, len, 100);
    
    // Deassert CS High (starts new conversion)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    
    return (status == HAL_OK) ? 0 : -1;
    */

    return 0;
}

int main(void)
{
    void *mcu_spi_handle = (void *)0x40013000; 

    /* Simplified context initialization (callbacks not mandatory if using __weak override) */
    dev_ctx_t dev_ctx = {
        .handle = mcu_spi_handle
    };

    float current_temp = 0.0f;
    max6675_stat_t status;

    if (max6675_temperature_get(&dev_ctx, &current_temp) == 0) {
        printf("Current Temperature: %.2f °C\n", current_temp);
    }

    /* Optional: Check diagnostics */
    if (max6675_status_get(&dev_ctx, &status) == 0) {
        if (status.open_tc) {
            printf("Warning: Thermocouple is open/disconnected!\n");
        }
    }

    return 0;
}

```

---

## Running Unit Tests

The development environment uses **Ceedling** (along with **Unity** and **CMock**) for isolated execution of unit tests.

### Prerequisites

* **Ruby** (for executing the Ceedling engine)
* **Ceedling** (unit testing framework for C)
* **gcovr** (optional, for coverage reporting)

### Execution Commands

1. Navigate to the test directory:

```bash
cd test

```

2. Run the full test suite:

```bash
ceedling test:all

```

3. Generate code coverage report:

```bash
ceedling gcov:all

```

The resulting HTML report will be available at:
`test/build/artifacts/gcov/gcovr/index.html`

---

## Generating Documentation

The public API is documented using **Doxygen**-compatible comment blocks.

### Option 1: Via Command Line

1. Navigate to the `docs/` directory:

```bash
cd docs

```

2. Build the documentation using the `doxyfile` configuration file:

```bash
doxygen doxyfile

```

3. Open the generated file in your web browser:

```bash
xdg-open html/index.html

```

### Option 2: Via Graphical User Interface

1. Launch Doxywizard from the terminal:

```bash
doxywizard

```

2. Select **File > Open** and open the `doxyfile` inside the `docs/` directory.
3. **Run** tab -> Click **Run doxygen**.
4. Open `html/index.html` in your browser.

---

## License

This project is distributed under the **MIT** License.