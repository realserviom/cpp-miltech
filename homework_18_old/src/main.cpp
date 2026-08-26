#include "main.h"
#include "cmsis_os.h"  // Підключаємо FreeRTOS

/* --- 1. Оголошення хендла (номера телефону) задачі --- */
TaskHandle_t btnTaskHandle = NULL;

/* Прототипи функцій */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void StartBtnTask(void *argument);

int main(void)
{
  /* Ініціалізація периферії HAL */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  /* --- 2. Створення задачі обробки кнопки --- */
  // Функція повертає хендл створеної задачі в btnTaskHandle (останній аргумент)
  xTaskCreate(
      StartBtnTask,       // Функція, яку виконуватиме задача
      "ButtonTask",       // Назва задачі (для відладки)
      128,                // Розмір стеку (в словах)
      NULL,               // Параметри задачі
      osPriorityNormal,   // Пріоритет задачі
      &btnTaskHandle      // Записуємо хендл задачі сюди!
  );

  /* Запуск планувальника FreeRTOS (система починає працювати) */
  osKernelStart();

  while (1) 
  {
    // Сюди управління ніколи не дійде
  }
}

/* --- 3. ОБРОБНИК ПЕРЕРИВАННЯ (ISR) --- */
// Ця функція викликається автоматично при натисканні кнопки ( спадаючий/зростаючий фронт на PA0 )
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_0) // Перевіряємо, що переривання саме від PA0
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* НАДСИЛАЄМО СИГНАЛ (Штовхаємо задачу) */
    // Будимо задачу btnTaskHandle з переривання
    vTaskNotifyGiveFromISR(btnTaskHandle, &xHigherPriorityTaskWoken);

    /* МИТТЄВА ЗМІНА КОНТЕКСТУ */
    // Якщо розбуджена задача має вищий пріоритет, 
    // мікроконтролер одразу перемикається на неї після виходу з ISR
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

/* --- 4. ЗАДАЧА ОБРОБКИ (TASK) --- */
void StartBtnTask(void *argument)
{
  for (;;)
  {
    /* СПИМО ТА ЧЕКАЄМО ДЗВІНКА */
    // Задача засинає і спалює 0% CPU.
    // pdTRUE — обнулити лічильник сповіщень після пробудження.
    // portMAX_DELAY — чекати безкінечно довго, поки не прийде сповіщення з ISR.
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    /* --- ВАЖКА РОБОТА ВИКОНУЄТЬСЯ ТУТ --- */

    // Апаратний антибрязкіт (Debounce delay) на 50 мс
    vTaskDelay(pdMS_TO_TICKS(50));

    // Перевіряємо, чи кнопка все ще натиснута (для захисту від хибних спрацювань)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
    {
      // Перемикаємо стан світлодіода на PC13
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
  }
}

/* --- 5. ІНІЦІАЛІЗАЦІЯ ПІНІВ СТМ32 --- */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Вмикаємо тактування портів A та C */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* Конфігурація PC13 (Світлодіод) -> Output */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // Вимикаємо LED за замовчуванням
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Конфігурація PA0 (Кнопка KEY) -> EXTI Interrupt */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Переривання по спаду сигналу (при натисканні)
  GPIO_InitStruct.Pull = GPIO_PULLUP;          // Підтяжка до 3.3V
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Вмикаємо лінію переривання EXTI0 у контролері NVIC */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0); // Пріоритет переривання (має бути >= 5 для FreeRTOS!)
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void SystemClock_Config(void)
{
  // Стандартне налаштування тактування (генериться CubeMX)
}
