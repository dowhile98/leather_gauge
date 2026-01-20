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
	LGC_HMI_VP_ICON_SPEEP = 0x1111, 			//Icon indicador de velocidad
	LGC_HMI_VP_BATCH_COUNT = 0x1050, 			//Contador de lotes
	LGC_HMI_VP_LEATHER_COUNT = 0x1051, 			//Contador de cueros
	LGC_HMI_VP_CURRENT_LEATHER_AREA = 0x1060, 	//Medida de Area actual
	LGC_HMI_VP_ACUMULATED_LEATHER_AREA = 0x1080,//Medida de Area acumulada por lote

	LGC_HMI_VP_TEST_CHOICED_SENSOR = 0x1101,	//Icon indica que sensor se escogió para revisarlo
	LGC_HMI_VP_TEST_BIT_SENSOR = 0x1104,		//Dirección que se utiliza para ver los photodiodos que detectan
	LGC_HMI_VP_TEST_SLIDER_THRESHOLD_SENSOR = 0x1108,	//Slider que modifica el umbral
	LGC_HMI_VP_TEST_NUMBER_THRESHOLD_SENSOR = 0x1109,	//Número que indica el valor del umbral en pantalla

	LGC_HMI_VP_PATTERN_ICON_3048 = 0x121A,		//Icon que indica que se ha escogido esta opción
	LGC_HMI_VP_PATTERN_RETURN_3048 = 0x121B,	//Botón que nos permite escoger esta opción

	LGC_HMI_VP_PATTERN_ICON_3000 = 0x122A,		//Icon que indica que se ha escogido esta opción
	LGC_HMI_VP_PATTERN_RETURN_3000 = 0x122B,	//Botón que nos permite escoger esta opción

	LGC_HMI_VP_PATTERN_ICON_2800 = 0x123A,		//Icon que indica que se ha escogido esta opción
	LGC_HMI_VP_PATTERN_RETURN_280 = 0x124B,		//Botón que nos permite escoger esta opción

	LGC_HMI_VP_CONFIG_TEXT_NAME_CLIENT = 0x1310,	//Texto: Nombre del cliente
	LGC_HMI_VP_CONFIG_TEXT_NAME_COLOR = 0x1320,		//Texto: Nombre del color
	LGC_HMI_VP_CONFIG_TEXT_NAME_LEATHER = 0x1330,	//Texto: Nombre del cuero
	LGC_HMI_VP_CONFIG_NUMBER_NAME_LEATHER = 0x1340,	//Numero de cueros máximos por lote

	//Configuración del día
	LGC_HMI_VP_CONFIG_DAY = 0x1341,
	LGC_HMI_VP_CONFIG_MONTH = 0x1342,
	LGC_HMI_VP_CONFIG_YEAR = 0x1343,

	//Configuración de la hora
	LGC_HMI_VP_CONFIG_SECOND = 0x1348,
	LGC_HMI_VP_CONFIG_MINUTE = 0x1347,
	LGC_HMI_VP_CONFIG_HOUR = 0x1346,

	LGC_HMI_VP_CONFIG_UNITS = 0x1350,				//Unidades entre ft2 y m2

	LGC_HMI_VP_PRINT = 0x1400,						//Botón que indica que se cierra el lote tal como está

	LGC_HMI_VP_LIST_DELETE = 0x1501,				//Botón que elimina el último cuero medido
	LGC_HMI_VP_LIST_ADDRESS_LEATHER_BASE = 0x1601	//Dirección base del primer cuero guardado.

} LGC_HMI_VAR_ADDR_TypeDef_t;
#endif /* APP_SRC_HMI_LGC_HMI_H_ */
