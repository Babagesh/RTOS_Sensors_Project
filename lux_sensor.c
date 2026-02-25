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
#include "queue.h"
#include "sl_i2cspm_instances.h"
#include "mystruct-def.h"
#include "task.h"
#include "string.h"

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

static void lux_task(void *arg);
extern QueueHandle_t light_queue_handle;
extern QueueHandle_t transmit_queue_handle;

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * Initialize blink example.
 ******************************************************************************/
void lux_init(void)
{
  TaskHandle_t xHandle = NULL;

#if (EXAMPLE_USE_STATIC_ALLOCATION == 1)

  static StaticTask_t xTaskBuffer;
  static StackType_t  xStack[BLINK_TASK_STACK_SIZE];

  // Create Blink Task without using any dynamic memory allocation
  xHandle = xTaskCreateStatic(lux_task,
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
  xReturned = xTaskCreate(lux_task,
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
static void lux_task(void *arg)
{
  (void)&arg;

  //Use the provided calculation macro to convert milliseconds to OS ticks
    TickType_t cur_delay = portMAX_DELAY;
    I2C_TransferSeq_TypeDef i2c_req;
    uint8_t tx_buf[1];
    uint8_t rx_buf[2];

    uint8_t init_buf[3] = {0x00, 0x00, 0x00};
    i2c_req.addr = (0x10 << 1);
    i2c_req.flags = I2C_FLAG_WRITE;
    i2c_req.buf[0].data = init_buf;
    i2c_req.buf[0].len = 3;
    I2CSPM_Transfer(sl_i2cspm_qwiic, &i2c_req);
  while (1) {
      light_command cmd;

      if(xQueueReceive(light_queue_handle, &cmd, cur_delay) == pdPASS)
        {
            if(!strncmp(cmd.cmd, "sc", 2))
              {
                tx_buf[0] = 0x07;
                i2c_req.addr = (0x10 << 1);
                i2c_req.flags = I2C_FLAG_WRITE_READ;
                i2c_req.buf[0].data = tx_buf;
                i2c_req.buf[0].len = 1;
                i2c_req.buf[1].data = rx_buf;
                i2c_req.buf[1].len = 2;

                bool i2c_ret = I2CSPM_Transfer(sl_i2cspm_qwiic, &i2c_req);

                if(!i2c_ret)
                  {
                    if(rx_buf[0] == 0x81)
                    {
                      sensor_data status_msg;
                      status_msg.type = 3; // DIFFERENCE 6: Type 3 is Lux Status
                      status_msg.value = 1.0;
                      xQueueSend(transmit_queue_handle, &status_msg, 0);
                    }
                  }
              }
            else if(!strncmp(cmd.cmd, "pr", 2))
              {
                if(cmd.time != 0)
                  {
                    cur_delay = pdMS_TO_TICKS(cmd.time);
                  }
              }
            else if(!strncmp(cmd.cmd, "cr", 2))
              {
                cur_delay = portMAX_DELAY;
              }

            if(!strncmp(cmd.cmd, "or", 2) || !strncmp(cmd.cmd, "pr", 2))
              {
                tx_buf[0] = 0x04;
                i2c_req.addr = (0x10 << 1);
                i2c_req.flags = I2C_FLAG_WRITE_READ;
                i2c_req.buf[0].data = tx_buf;
                i2c_req.buf[0].len = 1;
                i2c_req.buf[1].data = rx_buf;
                i2c_req.buf[1].len = 2;

                bool i2c_ret = I2CSPM_Transfer(sl_i2cspm_qwiic, &i2c_req);
                if(!i2c_ret)
                  {
                    uint16_t raw_lux = (rx_buf[1] << 8) | rx_buf[0];
                    float lux_final = (float)raw_lux * 0.0576f;

                    sensor_data data;
                    data.type = 1;
                    data.value = lux_final;
                    xQueueSend(transmit_queue_handle, &data, 0);
                  }
              }

        }
      else if(cur_delay != portMAX_DELAY)
        {
                tx_buf[0] = 0x04;
                i2c_req.addr = (0x10 << 1);
                i2c_req.flags = I2C_FLAG_WRITE_READ;
                i2c_req.buf[0].data = tx_buf;
                i2c_req.buf[0].len = 1;
                i2c_req.buf[1].data = rx_buf;
                i2c_req.buf[1].len = 2;

                if (I2CSPM_Transfer(sl_i2cspm_qwiic, &i2c_req) == 0) {
                    uint16_t raw_lux = (rx_buf[1] << 8) | rx_buf[0];
                    float lux_final = (float)raw_lux * 0.0576f;

                    sensor_data data;
                    data.type = 1;
                    data.value = lux_final;
                    xQueueSend(transmit_queue_handle, &data, 0);
                }
        }

  }
}
