#ifndef APP_H
#define APP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Точка входу в застосунок. Ініціалізує черги, м'ютекси та створює таски FreeRTOS.
 * Викликається з main.c перед запуском планувальника (vTaskStartScheduler).
 */
void app_start(void);

/**
 * @brief Колбек переривання GPIO (EXTI). 
 * Перенаправляє переривання від кнопки (GPIO_PIN_0) в таску buttonTask.
 */
void HAL_GPIO_EXTI_Callback(uint16_t pin);

#ifdef __cplusplus
}
#endif

#endif // APP_H