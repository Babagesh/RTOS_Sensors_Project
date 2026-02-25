#include "FreeRTOS.h"
#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"
#include "timers.h"
#include "task.h"

extern TimerHandle_t timer2_handle;
extern TimerHandle_t timer1_handle;

static TickType_t btn0_time;
static TickType_t btn1_time;


void sl_button_on_change(const sl_button_t * handle)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  TickType_t cur_time = xTaskGetTickCountFromISR();

  if(handle == &sl_button_btn0)// set temp reading to 0 and refresh
    {
      int difference = cur_time - btn0_time;
      if(difference >= pdMS_TO_TICKS(100))
        {
          btn0_time = cur_time;
          if(xTimerIsTimerActive(timer1_handle))
          {
            sl_led_turn_off(&sl_led_led0);
            xTimerStopFromISR(timer1_handle, &xHigherPriorityTaskWoken);
          }
        else
          {
            xTimerStartFromISR(timer1_handle, &xHigherPriorityTaskWoken);
          }
        }

    }
  else // set lux reading to 0 and refresh
    {
      int difference = cur_time - btn1_time;
      if(difference >= pdMS_TO_TICKS(100))
        {
          btn1_time = cur_time;
          sl_led_turn_on(&sl_led_led1);
          xTimerStartFromISR(timer2_handle, &xHigherPriorityTaskWoken);
        }
    }
}





