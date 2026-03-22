#include "input_mgr.h"
#include "esp_log.h"
#include "../services/idle_mgr.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "INPUT_MGR";

#define PIN_UP     GPIO_NUM_5
#define PIN_DOWN   GPIO_NUM_6
#define PIN_LEFT   GPIO_NUM_7
#define PIN_RIGHT  GPIO_NUM_15
#define PIN_SELECT GPIO_NUM_16

static QueueHandle_t s_input_queue = NULL;

typedef struct {
    gpio_num_t pin;
    input_event_t event;
    uint8_t state;
    uint32_t last_time;
} btn_state_t;

static btn_state_t s_buttons[5] = {
    {PIN_UP,     BTN_UP,     1, 0},
    {PIN_DOWN,   BTN_DOWN,   1, 0},
    {PIN_LEFT,   BTN_LEFT,   1, 0},
    {PIN_RIGHT,  BTN_RIGHT,  1, 0},
    {PIN_SELECT, BTN_SELECT, 1, 0}
};

static void input_poll_task(void *arg) {
    while (1) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        for (int i = 0; i < 5; i++) {
            uint8_t current = gpio_get_level(s_buttons[i].pin);
            
            // Simple 30ms Debounce
            if (current != s_buttons[i].state) {
                if (now - s_buttons[i].last_time > 30) {
                    s_buttons[i].state = current;
                    
                    // Fire event on press (falling edge due to pull-up)
                    if (current == 0) {
                        idle_mgr_feed(); // Reset sleep timer
                        if (s_input_queue) {
                            input_event_t ev = s_buttons[i].event;
                            xQueueSend(s_input_queue, &ev, 0);
                        }
                    }
                }
                s_buttons[i].last_time = now;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void input_mgr_init(void) {
    ESP_LOGI(TAG, "Initializing Input Manager (5 Buttons)");

    s_input_queue = xQueueCreate(10, sizeof(input_event_t));

    uint64_t pin_mask = (1ULL << PIN_UP) | (1ULL << PIN_DOWN) | 
                        (1ULL << PIN_LEFT) | (1ULL << PIN_RIGHT) | 
                        (1ULL << PIN_SELECT);

    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    xTaskCreatePinnedToCore(
        input_poll_task,
        "input_poll",
        2048,
        NULL,
        10,
        NULL,
        0
    );
}

input_event_t input_mgr_get_event(void) {
    if (!s_input_queue) return BTN_NONE;
    
    input_event_t ev;
    if (xQueueReceive(s_input_queue, &ev, 0) == pdTRUE) {
        return ev;
    }
    return BTN_NONE;
}
