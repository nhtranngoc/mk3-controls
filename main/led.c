#include <stdio.h>
#include "driver/ledc.h"
#include "esp_log.h"

#define LEDC_LS_TIMER LEDC_TIMER_1
#define LEDC_LS_MODE LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH3_GPIO (5)
#define LEDC_LS_CH3_CHANNEL LEDC_CHANNEL_3
#define LEDC_TEST_DUTY         (4000)
#define LEDC_TEST_FADE_TIME    (1000)

/*
    * Prepare individual configuration
    * for each channel of LED Controller
    * by selecting:
    * - controller's channel number
    * - output duty cycle, set initially to 0
    * - GPIO number where LED is connected to
    * - speed mode, either high or low
    * - timer servicing selected channel
    *   Note: if different channels use one timer,
    *         then frequency and bit_num of these channels
    *         will be the same
    */
ledc_channel_config_t ledc_channel = {
    .channel    = LEDC_LS_CH3_CHANNEL,
    .duty       = 0,
    .gpio_num   = LEDC_LS_CH3_GPIO,
    .speed_mode = LEDC_LS_MODE,
    .hpoint     = 0,
    .timer_sel  = LEDC_LS_TIMER
};

void init_led(void) {
    ESP_LOGI("led", "Initializing LED...");
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,           // timer mode
        .timer_num = LEDC_LS_TIMER,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    // Set configuration of timer1 for low speed channels
    ledc_timer_config(&ledc_timer);

    // Set LED Controller with previously prepared configuration
    ledc_channel_config(&ledc_channel);

    // Initialize fade service.
    ledc_fade_func_install(0);
}

void led_set_duty(uint8_t duty, int time) {
    ESP_LOGI("led", "Fading LED to duty %d over %d time", duty, time);

    ledc_set_fade_with_time(ledc_channel.speed_mode, ledc_channel.channel, duty, time);
    ledc_fade_start(ledc_channel.speed_mode, ledc_channel.channel, LEDC_FADE_NO_WAIT);
}