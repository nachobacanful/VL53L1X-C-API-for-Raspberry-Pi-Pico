#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#ifdef PICO_W_BOARD
#include "pico/cyw43_arch.h"
#endif

#include <stdio.h>
#include <stdint.h>

#include "VL53L1X_api.h"
#include "VL53L1X_types.h"


int main() {
	VL53L1X_Status_t status;
	VL53L1X_Result_t results;
    uint8_t I2CDevAddr = 0x29;

    stdio_init_all();

#if PICO_W_BOARD
    if (cyw43_arch_init()) {
        printf("Failed to initialize Pico W.\n");
        return 1;
    }
#else
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif

    for (int i=0; i<10; i++) {
#ifdef PICO_W_BOARD
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
#else
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
#endif

        sleep_ms(250);

#ifdef PICO_W_BOARD
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
#else
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
#endif

        sleep_ms(250);
    }

    // Initialize Pico's I2C using PICO_DEFAULT_I2C_SDA_PIN 
    // and PICO_DEFAULT_I2C_SCL_PIN (GPIO 4 and GPIO 5, respectively)
    if (VL53L1X_I2C_Init(I2CDevAddr, i2c0) < 0) {
        printf("Error initializing sensor.\n");
        return 0;
    }

    // Ensure the sensor has booted
    uint8_t sensorState;
    do {
		status += VL53L1X_BootState(I2CDevAddr, &sensorState);
		VL53L1X_WaitMs(I2CDevAddr, 2);
	} while (sensorState == 0);
	printf("Sensor booted.\n");

    // Initialize and configure sensor
	status = VL53L1X_SensorInit(I2CDevAddr);
	status += VL53L1X_SetDistanceMode(I2CDevAddr, 1);
	status += VL53L1X_SetTimingBudgetInMs(I2CDevAddr, 100);
	status += VL53L1X_SetInterMeasurementInMs(I2CDevAddr, 100);
	status += VL53L1X_StartRanging(I2CDevAddr);

    // Measure and print continuously
	bool first_range = true;
	while (1) {
        // Wait until we have new data
		uint8_t dataReady;
		do {
			status = VL53L1X_CheckForDataReady(I2CDevAddr, &dataReady);
			sleep_us(1);
		} while (dataReady == 0);

        // Read and display result
		status += VL53L1X_GetResult(I2CDevAddr, &results);
		printf("Status = %2d, dist = %5d, Ambient = %2d, Signal = %5d, #ofSpads = %5d\n",
			results.status, results.distance, results.ambient, results.sigPerSPAD, results.numSPADs);

        // Clear the sensor for a new measurement
		status += VL53L1X_ClearInterrupt(I2CDevAddr);
		if (first_range) {  // Clear twice on first measurement
			status += VL53L1X_ClearInterrupt(I2CDevAddr);
			first_range = false;
		}
	}
}