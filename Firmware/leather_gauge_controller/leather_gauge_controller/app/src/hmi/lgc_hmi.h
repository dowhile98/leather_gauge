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

typedef enum
{
	/* Dirección base / Sin acción o Solo cambio de página */
	LGC_HMI_VAR_NONE = 0x0000,

	/* Rango 0x1000 - General Return Keys */
	LGC_HMI_VAR_RET_KEY = 0x1001,
	LGC_HMI_VAR_RET_KEY = 0x1002,

	/* Rango 0x1100 - Ajustes y Teclas */
	LGC_HMI_VAR_RET_KEY = 0x1100,
	LGC_HMI_VAR_RET_KEY = 0x1101, // Aparece múltiples veces en la lista
	LGC_HMI_VAR_RET_KEY = 0x1102,
	LGC_HMI_VAR_INC_ADJ = 0x1104,	 // Incremental Adjustment
	LGC_HMI_VAR_DRAG_ADJ = 0x1111,	 // Drag Adjustment
	LGC_HMI_VAR_ICON_DRAG = 0x1112, // ICONDragManager

	/* Rango 0x1200 - Drag y Teclas */
	LGC_HMI_VAR_RET_KEY = 0x1200,
	LGC_HMI_VAR_DRAG_ADJ = 0x1201,
	LGC_HMI_VAR_DRAG_ADJ = 0x1203,
	LGC_HMI_VAR_RET_KEY = 0x1204,
	LGC_HMI_VAR_RET_KEY = 0x1205,

	/* Rango 0x1300 - Inputs y Ajustes */
	LGC_HMI_VAR_RET_KEY = 0x1300,
	LGC_HMI_VAR_TEXT_IN = 0x1310, // ASCII Text Input
	LGC_HMI_VAR_TEXT_IN = 0x1320, // ASCII Text Input
	LGC_HMI_VAR_TEXT_IN = 0x1330, // ASCII Text Input
	LGC_HMI_VAR_DATA_IN = 0x1340, // Data Input
	LGC_HMI_VAR_INC_ADJ = 0x1341, // Incremental Adjustment
	LGC_HMI_VAR_INC_ADJ = 0x1342,
	LGC_HMI_VAR_INC_ADJ = 0x1343,
	LGC_HMI_VAR_RET_KEY = 0x1344,
	LGC_HMI_VAR_RET_KEY = 0x1345,
	LGC_HMI_VAR_INC_ADJ = 0x1346,
	LGC_HMI_VAR_INC_ADJ = 0x1347,
	LGC_HMI_VAR_INC_ADJ = 0x1348,

	/* Rango 0x1400 - 0 */
	LGC_HMI_VAR_RET_KEY = 0x1400,
	LGC_HMI_VAR_RET_KEY = 0x1500

} LGC_HMI_TOUCH_Vars_t;
#endif /* APP_SRC_HMI_LGC_HMI_H_ */
