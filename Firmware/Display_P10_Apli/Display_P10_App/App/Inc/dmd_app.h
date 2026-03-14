/*
 * dmp_app.h
 *
 *  Created on: Aug 20, 2024
 *      Author: eplim
 */

#ifndef APP_INC_DMD_APP_H_
#define APP_INC_DMD_APP_H_

/*Includes -----------------------------------------------------------------------*/
#include "stdint.h"
#include "lwprintf.h"

/*Defines ------------------------------------------------------------------------*/
#define FIRST_ADDR	2
#define CONF_ADDR	4

#define DEFAULT_SERVER_ADDR		1



#define BAUD_2400	0
#define BAUD_4800	1
#define BAUD_9600 	2
#define BAUD_38400	3
#define BAUD_115200	4
/*Tyepdefs -----------------------------------------------------------------------*/
/**
 * enum
 */

/**
 * structs
 */
typedef struct dmd_p10_config{
	uint8_t brightnes;
	uint16_t baud;
	uint8_t addr;
	uint16_t toggleTime;
}dmd_p10_config_t;

typedef struct diplay_p10{
	dmd_p10_config_t conf;
	int temp1;
	int pressure1;
	int pressure2;
	int pressure3;
	int temp2;
	int screen;
	int numeric1;
	int numeric2;
	int numeric3;
	int numeric4;

}diplay_p10_t;
/*Extern definition -------------------------------------------------------------*/
extern diplay_p10_t data;

int dmd_lwprintf_out(int ch, lwprintf_t* lwp);

#endif /* APP_INC_DMD_APP_H_ */
