/*
 * brightc_lwprintf_out_fn.c
 *
 *  Created on: Jul 30, 2024
 *      Author: jeffr
 */

/*Includes ----------------------------------------------------------------------------*/
#include "main.h"
#include "lwprintf.h"

int dmd_lwprintf_out(int ch, lwprintf_t* lwp) {

	ITM_SendChar(ch);

	return ch;
}




