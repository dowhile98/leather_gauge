/*
 * lgc_module_input.h
 *
 *  Created on: Jan 14, 2026
 *      Author: tecna-smart-lab
 */

#ifndef MODULES_DI_LGC_MODULE_INPUT_H_
#define MODULES_DI_LGC_MODULE_INPUT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include "error.h"
#include "lgc_typedefs.h"


typedef void (*lgc_module_input_callback_cb_t)(uint8_t di, uint32_t evt);

error_t lgc_module_input_init(lgc_module_input_callback_cb_t callback);



#endif /* MODULES_DI_LGC_MODULE_INPUT_H_ */
