#include <stdio.h>

#include "esp_attr.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "esp_log.h"

//You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH 330 //Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 2600 //Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 180 //Maximum angle in degree upto which servo can rotate

#define SERVO_BOT_PIN 19
#define SERVO_TOP_PIN 18

#define SERVO_BOT_CHANNEL 0
#define SERVO_TOP_CHANNEL 1

#define SERVO_TOP_POS_UP1 105
#define SERVO_TOP_POS_UP2 165
#define SERVO_TOP_POS_DOWN 65
#define SERVO_BOT_POS_UP 60
#define SERVO_BOT_POS_DOWN 10

static void init_servo_gpio(void) {
    ESP_LOGI("example", "initializing mcpwm servo control gpio......\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_BOT_PIN);    //Set bottom servo pin as PWM0A
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, SERVO_TOP_PIN);    //Set top servo pin as PWM0B
}

static void init_servo_mcpwm(void) {
    ESP_LOGI("example", "Configuring Initial Parameters of mcpwm......\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;    //frequency = 50Hz, i.e. for every servo motor time period should be 20ms
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
}

void init_servo() {
    init_servo_gpio();
    init_servo_mcpwm();
}

/**
 * @brief Use this function to calcute pulse width for per degree rotation
 *
 * @param  degree_of_rotation the angle in degree to which servo has to rotate
 *
 * @return
 *     - calculated pulse width
 */
static uint32_t servo_per_degree_init(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (SERVO_MIN_PULSEWIDTH + (((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * (degree_of_rotation)) / (SERVO_MAX_DEGREE)));
    return cal_pulsewidth;
}

/**
 * @brief Set servo by degree and channel
 * 
 * @param degree angle to set servo to, from 0-180
 * @param channel enum, MCPWM_OPR_A = 0, MCPWM_OPR_B = 1
 */
static void servo_set_degree(uint32_t degree, uint8_t channel) {
    uint32_t angle = servo_per_degree_init(degree);
    mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, channel, angle);
}

/**
 * @brief High level servo control for visor state
 * 
 * @param state 0 = down, 1 = up
 */
void visor_set_state(uint8_t state) {
    if (state == 0) {
        // Visor down
        ESP_LOGI("example", "Setting visor down..");
        servo_set_degree(SERVO_TOP_POS_DOWN, SERVO_TOP_CHANNEL);
        servo_set_degree(SERVO_BOT_POS_DOWN, SERVO_BOT_CHANNEL);
    } else {
        ESP_LOGI("example", "Setting visor up..");
        servo_set_degree(SERVO_TOP_POS_UP1, SERVO_TOP_CHANNEL);
        servo_set_degree(SERVO_BOT_POS_UP, SERVO_BOT_CHANNEL);
        servo_set_degree(SERVO_TOP_POS_UP2, SERVO_TOP_CHANNEL);
    }
}