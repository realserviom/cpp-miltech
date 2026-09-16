#include <driver/gpio.h>

// init simple output for led
void init_gpio_simple_output(gpio_num_t pin, uint32_t initial_level) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,           // Налаштування як вихід
        .pull_up_en = GPIO_PULLUP_DISABLE,  
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Встановлюємо початковий стан (0 - вимкнено, 1 - увімкнено)
    gpio_set_level(pin, initial_level); 
}


static void IRAM_ATTR button_isr_handler(void* arg) {

    if (gpio_get_level(BUTTON_GPIO) != 0) {
        return;
    }

    uint64_t current_time = esp_timer_get_time(); // Поточний час у мкс

    // Пропускаємо виклик, якщо з моменту 
    // останнього минуло менше 200 мс (200,000 мкс)
    if (current_time - last_interrupt_time > 300000) {
        last_interrupt_time = current_time;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        //esp_rom_printf("Button Pressed (ISR)!\n");

        // Відправляємо сигнал у Event Group
        // xEventGroupSetBitsFromISR(displayEventGroup, EVENT_BUTTON_PRESSED,
        // &xHigherPriorityTaskWoken);

        // Сповіщаємо конкретну задачу напряму з ISR
        vTaskNotifyGiveFromISR(displayTaskHandle, &xHigherPriorityTaskWoken);
        
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

// Тепер функція приймає вказівник на обробник і номер піна
void init_gpio_button(gpio_num_t pin) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE // Реакція на натискання (High -> Low)
    };
    gpio_config(&io_conf);

    // Встановлюємо та підключаємо переданий обробник
    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin, button_isr_handler, NULL);
}