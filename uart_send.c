/***************************************************************************//**
 * @file
 * @brief Blink examples functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include "sl_simple_led.h"
#include "sl_simple_led_instances.h"
#include "mystruct-def.h"
#include "FreeRTOS.h"
#include "sl_uartdrv_instances.h"
#include "queue.h"
#include "task.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/



#ifndef TOOGLE_DELAY_MS
#define TOOGLE_DELAY_MS            1000
#endif

#ifndef BLINK_TASK_STACK_SIZE
#define BLINK_TASK_STACK_SIZE      configMINIMAL_STACK_SIZE
#endif

#ifndef BLINK_TASK_PRIO
#define BLINK_TASK_PRIO            20
#endif

#ifndef EXAMPLE_USE_STATIC_ALLOCATION
#define EXAMPLE_USE_STATIC_ALLOCATION      1
#endif

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

static void uart_send_task(void *arg);
QueueHandle_t transmit_queue_handle;
StaticQueue_t transmit_queue;
uint8_t transmit_queue_buffer[sizeof(sensor_data) * 5];
static UARTDRV_Handle_t uart_send_handle;


/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize blink example.
 ******************************************************************************/
void uart_send_init(void)
{
  TaskHandle_t xHandle = NULL;

#if (EXAMPLE_USE_STATIC_ALLOCATION == 1)

  static StaticTask_t xTaskBuffer;
  static StackType_t  xStack[BLINK_TASK_STACK_SIZE];

  // Create Blink Task without using any dynamic memory allocation
  xHandle = xTaskCreateStatic(uart_send_task,
                              "blink task",
                              BLINK_TASK_STACK_SIZE,
                              ( void * ) NULL,
                              tskIDLE_PRIORITY + 1,
                              xStack,
                              &xTaskBuffer);

  // Since puxStackBuffer and pxTaskBuffer parameters are not NULL,
  // it is impossible for xHandle to be null. This check is for
  // rigorous example demonstration.
  EFM_ASSERT(xHandle != NULL);

#else

  BaseType_t xReturned = pdFAIL;

  // Create Blink Task using dynamic memory allocation
  xReturned = xTaskCreate(uart_send_task,
                          "blink task",
                          BLINK_TASK_STACK_SIZE,
                          ( void * ) NULL,
                          tskIDLE_PRIORITY + 1,
                          &xHandle);

  // Unlike task creation using static allocation, dynamic task creation can very likely
  // fail due to lack of memory. Checking the return value is relevant.
  EFM_ASSERT(xReturned == pdPASS);

#endif
}

/*******************************************************************************
 * Blink task.
 ******************************************************************************/
static void uart_send_task(void *arg)
{
  (void)&arg;
  sensor_data data;
  transmit_queue_handle = xQueueCreateStatic(5,                   // queue length
                                           sizeof(sensor_data),   // item size
                                           transmit_queue_buffer,  // storage area
                                           &transmit_queue);

  uart_send_handle = sl_uartdrv_get_default();
  while (1)
  {
      xQueueRecieve(transmit_queue_handle, &data, pdMS_TO_TICKS(portMAX_DELAY));
      if(data.type == 0) // Temp sensor reading
        {
          char uart_tx_string[128];
          sprintf("Temp Sensor Reading: %f", data.value);
          UARTDRV_TransmitB(uart_send_handle, uart_tx_string, strlen(uart_tx_string));
        }
      else if(data.type == 1) // Lux Sensor reading
        {
          char uart_tx_string[128];
          sprintf("Lux Sensor Reading: %f", data.value);
          UARTDRV_TransmitB(uart_send_handle, uart_tx_string, strlen(uart_tx_string));
        }
      else if(data.type == 2) // Temp sensor status
        {
          if(data.value == 1)
            {

            }


        }
      else if(data.type == 3) // Lux Sensor Status
        {

        }
  }
}
