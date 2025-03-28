#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"

#define BUTTON_GPIO 5 // Defina o pino do botão
#define BUTTON_PRESSED 1
#define BUTTON_RELEASED 0

QueueHandle_t  buttonQueue; // Fila para comunicação entre as tasks

void Task_ReadButton(void *pvParameters) {
    gpio_init(BUTTON_GPIO);
    gpio_set_dir(BUTTON_GPIO, GPIO_IN);
    gpio_pull_up(BUTTON_GPIO); // Habilita pull-up interno
    int buttonState = BUTTON_RELEASED;
    while (1) {
        buttonState = gpio_get(BUTTON_GPIO) ? BUTTON_RELEASED : BUTTON_PRESSED; // Botão ativo em LOW
        xQueueSend(buttonQueue, &buttonState, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100)); // Executa a cada 100ms
    }
}

void Task_ProcessButton(void *pvParameters) {
    int receivedState;
    while (1) {
        if (xQueueReceive(buttonQueue, &receivedState, portMAX_DELAY)) {
            xQueueSend(buttonQueue, &receivedState, portMAX_DELAY); // Enviar para a Task do LED
        }
    }
}

void Task_ControlLED(void *pvParameters) {
    cyw43_arch_init(); // Inicializa a biblioteca do Wi-Fi (necessário para o LED do Pico W)
    
    int ledState = 0;
    while (1) {
        if (xQueueReceive(buttonQueue, &ledState, portMAX_DELAY)) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, ledState); // Liga/desliga o LED
            printf("LED %s!\n", ledState ? "Ligado" : "Desligado");
        }
    }
}

int main(void) {
    stdio_init_all(); // Inicializa saída padrão para debug
    
    buttonQueue = xQueueCreate(5, sizeof(int));
    if (buttonQueue == NULL) {
        printf("Erro ao criar a fila!\n");
        return 1;
    }

    xTaskCreate(Task_ReadButton, "ReadButton", 1000, NULL, 1, NULL);
    xTaskCreate(Task_ProcessButton, "ProcessButton", 1000, NULL, 2, NULL);
    xTaskCreate(Task_ControlLED, "ControlLED", 1000, NULL, 3, NULL);

    vTaskStartScheduler(); // Inicia o agendador do FreeRTOS
    for (;;);
}


