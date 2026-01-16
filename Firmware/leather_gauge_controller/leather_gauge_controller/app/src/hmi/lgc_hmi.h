/*
 * lgc_hmi.h
 *
 *  Created on: Jan 13, 2026
 *      Author: tecna-smart-lab
 */

#ifndef APP_SRC_HMI_LGC_HMI_H_
#define APP_SRC_HMI_LGC_HMI_H_

#include "lgc.h"

typedef enum
{
	HMI_PAGEO = 0,
	HMI_PAGE1,
	HMI_PAGE2,
	HMI_PAGE3,
	HMI_PAGE4,
	HMI_PAGE5,
	HMI_PAGE6,
	HMI_PAGE7,
	HMI_PAGE8,
	HMI_PAGE9,
	HMI_PAGE10,
	HMI_PAGE11,
	HMI_PAGE12,
	HMI_PAGE13,
	HMI_PAGE14,
	HMI_PAGE15,
	HMI_PAGE16,
	HMI_PAGE17,
	HMI_PAGE18,
	HMI_PAGE19,
	HMI_PAGE20,
	HMI_PAGE21,
	HMI_PAGE22,
	HMI_PAGE23,
	HMI_PAGEMAX,
} LGC_HMI_Page_t;

/**
 * @brief DWIN Variable Address Enumeration
 *
 * Defines all VP (Variable Pointer) addresses used in the DWIN display.
 * Based on the variable list from Lista de variables.xlsx
 *
 * Format:
 *   - Page(s): Indicates on which DWIN pages this variable appears
 *   - Type: DWIN data type (Data Variable Display, Return Key Code, etc.)
 *   - Values: Range or description of valid values for the variable
 */
typedef enum
{
	LGC_HMI_VP_ICON_SPEEP = 0x1111,
	LGC_HMI_VP_BATCH_COUNT = 0x1050,
	LGC_HMI_VP_LEATHER_COUNT = 0x1051,
	LGC_HMI_VP_CURRENT_LEATHER_AREA = 0x1060,


} LGC_HMI_VAR_ADDR_TypeDef_t;
#endif /* APP_SRC_HMI_LGC_HMI_H_ */
