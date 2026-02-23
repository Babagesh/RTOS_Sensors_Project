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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "mystruct-def.h"
#include "sl_uartdrv_instances.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

static UARTDRV_Handle_t uart_handle;
QueueHandle_t temp_queue_handle;
QueueHandle_t light_queue_handle;
StaticQueue_t temp_queue;
StaticQueue_t light_queue;
uint8_t temp_queue_buffer[sizeof(temp_command) * 5];
uint8_t light_queue_buffer[sizeof(light_command) * 5];

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

static void uart_recieve_task(void *arg);

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize blink example.
 ******************************************************************************/
void uart_recieve_init(void)
{
  TaskHandle_t xHandle = NULL;
  temp_queue_handle = xQueueCreateStatic(5,                   // queue length
                                            sizeof(temp_command),   // item size
                                            temp_queue_buffer,  // storage area
                                            &temp_queue);
  light_queue_handle = xQueueCreateStatic(5,                   // queue length
                                          sizeof(light_command),   // item size
                                          light_queue_buffer,  // storage area
                                          &light_queue);

#if (EXAMPLE_USE_STATIC_ALLOCATION == 1)

  static StaticTask_t xTaskBuffer;
  static StackType_t  xStack[BLINK_TASK_STACK_SIZE];

  // Create Blink Task without using any dynamic memory allocation
  xHandle = xTaskCreateStatic(uart_recieve_task,
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
  xReturned = xTaskCreate(uart_recieve_task,
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
static void uart_recieve_task(void *arg)
{

  (void)&arg;
  //Use the provided calculation macro to convert milliseconds to OS ticks
  uart_handle = sl_uartdrv_get_default();
  char uart_tx_string[128];
  char uart_rx_bytes[15];
  char uart_rx_byte;
  int buffer_index = 0;

  sprintf(uart_tx_string, "\033[2J\033[1;1H");
  UARTDRV_TransmitB(uart_handle, uart_tx_string, strlen(uart_tx_string));
  sprintf(uart_tx_string, " UART RX started... enter a command on the keyboard\n");
  UARTDRV_TransmitB(uart_handle, uart_tx_string, strlen(uart_tx_string));

  while (1) {
      UARTDRV_ReceiveB(uart_handle, &uart_rx_byte, 1);
      UARTDRV_TransmitB(uart_handle, &uart_rx_byte, 1);
      if(uart_rx_byte =='\n' || uart_rx_byte == '\r')
      {
          uart_rx_bytes[buffer_index] = '\0';
          if(strncmp(uart_rx_bytes, "sc", 2) == 0)
            {
               temp_command t_cmd;
               strcpy(t_cmd.cmd, "sc");
               t_cmd.time = 0;
               xQueueSend(temp_queue_handle, &t_cmd, portMAX_DELAY);

               light_command l_cmd;
               strcpy(l_cmd.cmd, "sc");
               l_cmd.time = 0;
               xQueueSend(light_queue_handle, &l_cmd, portMAX_DELAY);
            }
          else if (strncmp(uart_rx_bytes, "or1", 3) == 0)
            {
              // We need to send to temp sensor. We will just send or
              temp_command cmd;
              strcpy(cmd.cmd, "or");
              cmd.time = 0;
              xQueueSend(temp_queue_handle,&cmd, portMAX_DELAY);

            }
          else if (strncmp(uart_rx_bytes, "or2", 3) == 0)
            {
              light_command cmd;
              strcpy(cmd.cmd, "or");
              cmd.time = 0;
              xQueueSend(light_queue_handle,&cmd, portMAX_DELAY);

          }
          else if (strncmp(uart_rx_bytes, "pr1", 3) == 0)
          {
              temp_command cmd;
              strcpy(cmd.cmd, "pr");
              cmd.time = atoi(&uart_rx_bytes[4]);
              xQueueSend(temp_queue_handle, &cmd, portMAX_DELAY);
          }
          else if (strncmp(uart_rx_bytes, "pr2", 3) == 0)
          {
              light_command cmd;
              strcpy(cmd.cmd, "pr");
              cmd.time = atoi(&uart_rx_bytes[4]);
              xQueueSend(light_queue_handle, &cmd, portMAX_DELAY);

          }
          else if (strncmp(uart_rx_bytes, "cr1", 3) == 0)
          {
              temp_command cmd;
              strcpy(cmd.cmd, "cr");
              cmd.time = 0;
              xQueueSend(temp_queue_handle, &cmd, portMAX_DELAY);
          }
          else if (strncmp(uart_rx_bytes, "cr2", 3) == 0)
          {
              light_command cmd;
              strcpy(cmd.cmd, "cr");
              cmd.time = 0;
              xQueueSend(light_queue_handle, &cmd, portMAX_DELAY);
          }
          buffer_index = 0;
      }
      else
        {
          if(buffer_index < 14)
            {
              uart_rx_bytes[buffer_index] = uart_rx_byte;
              buffer_index ++;
            }
          else
            {
               buffer_index = 0;
            }
        }


  }
}
