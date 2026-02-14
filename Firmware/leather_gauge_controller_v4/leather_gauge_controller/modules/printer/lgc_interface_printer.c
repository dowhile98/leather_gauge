/*
 * lgc_interface_printer.c
 *
 *  Created on: Sep 20, 2025
 *      Author: tecna-smart-lab
 */

//-------------------------------------------------------------------
// includes
//-------------------------------------------------------------------
#include "lgc_interface_printer.h"
#include "stm32_log.h"
#include "error.h"
#include "os_port.h"
#include "usb_otg.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_stm32.h"
#include "ux_host_class_printer.h"
//--------------------------------------------------------------------
// typedefs
//--------------------------------------------------------------------
typedef enum
{
    USB_VBUS_FALSE = 0,
    USB_VBUS_TRUE,
} USB_VBUS_State;

typedef enum
{
    Mouse_Device = 1,
    Keyboard_Device,
    Printer_Device,
    Unknown_Device,
} HID_Device_Type;

typedef enum
{
    Device_disconnected = 1,
    Device_connected,
    No_Device,
} Device_state;

typedef struct
{
    HID_Device_Type Device_Type;
    Device_state Dev_state;
} ux_app_devInfotypeDef;

//--------------------------------------------------------------------
// defines
//--------------------------------------------------------------------
#define USBX_APP_STACK_SIZE 1024*4
#define APP_QUEUE_SIZE 3
//--------------------------------------------------------------------
// global constant
//--------------------------------------------------------------------
static const char *TAG = "USBX_PRINTER";

//--------------------------------------------------------------------
// global variables
//--------------------------------------------------------------------
static OsQueue MsgQueue;
static UX_HOST_CLASS_PRINTER *printer;
static OsTaskId ux_app_task;
static uint8_t lgc_printer_status = 0;
//-------------------------------------------------------------------
// private function prototype
//-------------------------------------------------------------------
static VOID ux_host_error_callback(UINT system_level, UINT system_context, UINT error_code);
static UINT MX_USB_Host_Init(void);
static void USBH_DriverVBUS(uint8_t state);
static void usbx_app_thread_entry(void *arg);
static UINT ux_host_event_callback(ULONG event, UX_HOST_CLASS *Current_class, VOID *Current_instance);
//-------------------------------------------------------------------
// public function
//-------------------------------------------------------------------

uint8_t lgc_interface_printer_init(void )
{
    OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;
    uint8_t ret = UX_SUCCESS;

    /* register a callback error function */
    _ux_utility_error_callback_register(&ux_host_error_callback);

    /// queu create
    if (osCreateQueue(&MsgQueue, "ux queue", sizeof(ux_app_devInfotypeDef), APP_QUEUE_SIZE) != TRUE)
    {
        STM32_LOGE(TAG, "Error ux app queue");

        return UX_ERROR;
    }
    /*create task*/

    params.stackSize = USBX_APP_STACK_SIZE / 4;
    params.priority = 10;
    ux_app_task = osCreateTask("ux app", usbx_app_thread_entry, 0, &params);
    if (ux_app_task == NULL)
    {
        STM32_LOGE(TAG, "Error create ux app");
        ret = UX_ERROR;
    }

    return ret;
}


uint8_t lgc_interface_printer_writeData(uint8_t *d, uint16_t len)
{
    ULONG actual_length;
    UINT status;
    if (printer == NULL)
    {
        STM32_LOGE(TAG, "Error Write Data");

        return UX_ERROR;
    }
    status = ux_host_class_printer_write(printer, d, len, &actual_length);
    if (status == UX_SUCCESS && actual_length == len)
    {
        return UX_SUCCESS;
    }
    return UX_ERROR;
}

uint8_t lgc_interface_printer_connected(void)
{
	return lgc_printer_status;
}
//-------------------------------------------------------------------
// private function definition
//-------------------------------------------------------------------
static void usbx_app_thread_entry(void *arg)
{
    ULONG status;
    ux_app_devInfotypeDef ux_dev_info;
    ULONG printer_status;
    /* Initialize USBX_Host */
    MX_USB_Host_Init();
    STM32_LOGI(TAG, " **** USB OTG FS PRINTER Host **** ");
    STM32_LOGI(TAG, "USB Host library started.");

    /* Wait for Device to be attached */
    STM32_LOGI(TAG, "Starting PRINTER Application");
    STM32_LOGI(TAG, "Connect your PRINTER Device");

    for (;;)
    {
        if (osReceiveFromQueue(&MsgQueue, &ux_dev_info, INFINITE_DELAY) != TRUE)
        {
            STM32_LOGE(TAG, "Error receive MsQueue");

            continue;
        }

        if (ux_dev_info.Dev_state == Device_connected)
        {
            switch (ux_dev_info.Device_Type)
            {
            case Printer_Device:
                STM32_LOGI(TAG, "Printer Device");
                STM32_LOGI(TAG, "PID: %#x ", (UINT)printer->ux_host_class_printer_device->ux_device_descriptor.idProduct);
                STM32_LOGI(TAG, "VID: %#x ", (UINT)printer->ux_host_class_printer_device->ux_device_descriptor.idVendor);
                STM32_LOGI(TAG, "Printer is ready...");
                status = ux_host_class_printer_status_get(printer, &printer_status);
                if (status == UX_SUCCESS)
                {
                    STM32_LOGI(TAG, "Printer Status: 0x%X", (UINT)printer_status);
                }
                lgc_printer_status = 1;
                break;
            case Unknown_Device:
                STM32_LOGI(TAG, "Unsupported USB device");
                lgc_printer_status = 0;
                break;

            default:
                break;
            }
        }
        else
        {
            /* clear hid_client local instance */
            printer = NULL;
        }
    }
}

static VOID ux_host_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    ux_app_devInfotypeDef msg;
    switch (error_code)
    {
    case UX_DEVICE_ENUMERATION_FAILURE:
        msg.Device_Type = Unknown_Device;
        msg.Dev_state = Device_connected;
        osSendToQueue(&MsgQueue, &msg, 0);
        STM32_LOGI(TAG, "USB Device error enumeration");
        break;

    case UX_NO_DEVICE_CONNECTED:
        STM32_LOGI(TAG, "USB Device disconnected");
        lgc_printer_status = 0;
        break;

    default:
        break;
    }
}

static UINT MX_USB_Host_Init(void)
{
    UINT ret = UX_SUCCESS;

    /* The code below is required for installing the host portion of USBX. */
    if (ux_host_stack_initialize(ux_host_event_callback) != UX_SUCCESS)
    {
        return UX_ERROR;
    }
    /* Register printer class. */
    if (ux_host_stack_class_register(_ux_system_host_class_printer_name,
                                     _ux_host_class_printer_entry) != UX_SUCCESS)
    {
        return UX_ERROR;
    }
    /* Get an instance of the printer class. */

    /* Initialize the LL driver */
    MX_USB_OTG_FS_HCD_Init();
    /* Register all the USB host controllers available in this system.  */
    if (ux_host_stack_hcd_register(_ux_system_host_hcd_stm32_name,
                                   _ux_hcd_stm32_initialize, USB_OTG_FS_PERIPH_BASE,
                                   (ULONG)&hhcd_USB_OTG_FS) != UX_SUCCESS)
    {
        return UX_ERROR;
    }

    /* Drive vbus */
    USBH_DriverVBUS(USB_VBUS_TRUE);

    /* Enable USB Global Interrupt*/
    HAL_HCD_Start(&hhcd_USB_OTG_FS);
    return ret;
}
static void USBH_DriverVBUS(uint8_t state)
{
    /* USER CODE BEGIN 0 */

    /* USER CODE END 0*/

    if (state != USB_VBUS_TRUE)
    {
    }
    else
    {
    }

    HAL_Delay(200);
}
static UINT ux_host_event_callback(ULONG event, UX_HOST_CLASS *Current_class, VOID *Current_instance)
{
    UINT status;
    UX_HOST_CLASS *printer_class;
    ux_app_devInfotypeDef msg;
    switch (event)
    {
    case UX_DEVICE_INSERTION:
        /* Get current Hid Class */
        status = ux_host_stack_class_get(_ux_system_host_class_printer_name, &printer_class);

        if (status == UX_SUCCESS)
        {
            if ((printer_class == Current_class) && (printer == NULL))
            {
                /* Get current Hid Instance */
                printer = Current_instance;
                if (printer->ux_host_class_printer_class->ux_host_class_status != (ULONG)UX_HOST_CLASS_INSTANCE_LIVE)
                {
                    msg.Device_Type = Unknown_Device;
                }
                else if (ux_utility_memory_compare(printer->ux_host_class_printer_class->ux_host_class_name,
                                                   _ux_system_host_class_printer_name,
                                                   ux_utility_string_length_get(_ux_system_host_class_printer_name)) == UX_SUCCESS)
                {
                    msg.Device_Type = Printer_Device;
                    msg.Dev_state = Device_connected;
                    osSendToQueue(&MsgQueue, &msg, 0);
                    STM32_LOGI(TAG, "Printer Client Plugged");
                }
                else
                {
                    msg.Device_Type = Unknown_Device;
                    msg.Dev_state = Device_connected;
                    osSendToQueue(&MsgQueue, &msg, 0);
                }
            }
        }
        else
        {
            /* No HID class found */
            STM32_LOGI(TAG, "Printer Class found");
        }
        break;

    case UX_DEVICE_REMOVAL:

        if (Current_instance == printer)
        {
            /* Free Instance */
            printer = NULL;
            STM32_LOGI(TAG, "Printer Unplugged");

            lgc_printer_status = 0;
        }
        break;
    case UX_DEVICE_CONNECTION:
        STM32_LOGI(TAG, "Printer connected");
        break;
    case UX_NO_DEVICE_CONNECTED:
        STM32_LOGI(TAG, "Printer disconnected");
        break;
    default:
        break;
    }

    return (UINT)UX_SUCCESS;
}


