// ########### слухає ввід із консолі (Serial Monitor) ###################
// ################# але працює тільки із сайт wokwi з VS code не працює ##############
void uartCommandTask(void *pvParameters) {
    printf("Введіть команду (напр: SENSOR_PERIOD 500 або DISPLAY_PERIOD 5000) і натисніть Enter:\n");

    char buffer[64];
    int index = 0;

    while (1) {
        uint8_t ch;
        // Читаємо по одному байту з UART0 (або іншого порту) з таймаутом 50 мс
        int len = uart_read_bytes(UART_NUM, &ch, 1, pdMS_TO_TICKS(50));

        if (len <= 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue; // Нічого не прийшло, йдемо далі
        }
        
        if (ch == '\n' || ch == '\r') {
            buffer[index] = '\0'; // Завершуємо рядок
            
            if (index > 0) {
                buffer[strcspn(buffer, "\r\n")] = 0;

                if (strncmp(buffer, "SENSOR_PERIOD ", 14) == 0) {
                    handle_period_command(SENSOR_PERIOD, buffer, 14, &sensor_period_ms, 20, 500, "Sensor");
                } 
                else if (strncmp(buffer, "DISPLAY_PERIOD ",
                     15) == 0) {
                    handle_period_command(DISPLAY_PERIOD, buffer, 15, &display_period_ms, 100, 30000, "Display");
                } 
                else {
                    printf("ACK: Unknown command -> %s\n", buffer);
                    fflush(stdout);
                }
            }
            index = 0;
        } 
        else if (index < sizeof(buffer) - 1) {
            printf("ch: Unknown command ch-> %c\n", ch);
            buffer[index++] = (char)ch;
        }
    }
}