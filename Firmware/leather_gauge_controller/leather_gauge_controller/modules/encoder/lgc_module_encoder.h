/*
 * lgc_module_encoder.h
 *
 *  Created on: Jan 14, 2026
 *      Author: tecna-smart-lab
 */

#ifndef MODULES_ENCODER_LGC_MODULE_ENCODER_H_
#define MODULES_ENCODER_LGC_MODULE_ENCODER_H_

#include <stdint.h>
#include <stddef.h>
#include "error.h"


typedef void (*lgc_module_encoder_callback_cb_t)(void);


error_t lgc_module_encoder_init(lgc_module_encoder_callback_cb_t callback);

#endif /* MODULES_ENCODER_LGC_MODULE_ENCODER_H_ */
