/*
 * dmd_config.h
 *
 *  Created on: Aug 19, 2024
 *      Author: eplim
 */

#ifndef COMMON_DMD_CONFIG_H_
#define COMMON_DMD_CONFIG_H_

#include <FreeRTOS.h>
#include <cmsis_os.h>
#include "main.h"
#include "tim.h"
#include "spi.h"
// Pins on the DMDESP connector board.
#define DMD_PIN_A 16             //D0 // A PHASE_LSB
#define DMD_PIN_B 12             //D6 // B PHASE_MSB
#define DMD_PIN_LATCH 0          //D3 // SCLK
#define DMD_PIN_OUTPUT_ENABLE 15 //D8 // nOE
#define DMD_PIN_SPI_MOSI 13      //D7 // R SPI Master Out, Slave In
#define DMD_PIN_SPI_SCK 14       //D5 // CLK SPI Serial Clock

// Dimension information for the display.
#define DMDESP_NUM_COLUMNS 32 // Number of columns in a panel.
#define DMDESP_NUM_ROWS 16    // Number of rows in a panel.

// Refresh times.
#define DMDESP_REFRESH_US 100


#define dmd_free(x)				vPortFree(x)
#define dmd_malloc(x)			pvPortMalloc(x)
#endif /* COMMON_DMD_CONFIG_H_ */
