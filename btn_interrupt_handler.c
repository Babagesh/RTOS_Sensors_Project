#include "FreeRTOS.h"
#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"
#include "mystruct-def.h"
#include "timers.h"
#include "task.h"
#include "queue.h"

extern QueueHandle_t transmit_queue_handle;

void sl_button_on_change(const sl_button_t * handle)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  sensor_data clear;

  if(sl_button_get_state(handle) == SL_SIMPLE_BUTTON_PRESSED)
    {

  if(handle == &sl_button_btn0)// set temp reading to 0 and refresh
    {
      clear.type = 4;
      clear.value = 0;
      xQueueSendFromISR(transmit_queue_handle, &clear, &xHigherPriorityTaskWoken);

    }
  else // set lux reading to 0 and refresh
    {
      clear.type = 4;
      clear.value = 1;
      xQueueSendFromISR(transmit_queue_handle, &clear, &xHigherPriorityTaskWoken);
    }
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

}





