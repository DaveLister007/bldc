#include "ch.h" // ChibiOS
#include "hal.h"

static THD_WORKING_AREA(buzzer_thread_wa, 256);
static THD_FUNCTION(buzzer_thread_fn, arg);

static stm32_gpio_t *ADC3_PORT = GPIOC;
static int ADC3_PIN = 5;


// Settings
#define TIM_CLOCK			1000000 // Hz
#define TIM_PERIOD          1000

static void buzzer_on(void);
static void buzzer_off(void);


void pwm_setup(void)
{
    
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	palSetPadMode(HW_SERVO_PORT, HW_SERVO_PIN, PAL_MODE_ALTERNATE(HW_SERVO_GPIO_AF) |
			PAL_STM32_OSPEED_HIGHEST | PAL_STM32_PUDR_FLOATING);

	HW_SERVO_TIM_CLK_EN();

	TIM_TimeBaseStructure.TIM_Period = 500;
	TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)((168000000 / 2) / TIM_CLOCK) - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(HW_SERVO_TIMER, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC1Init(HW_SERVO_TIMER, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(HW_SERVO_TIMER, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(HW_SERVO_TIMER, ENABLE);

	HW_SERVO_TIMER->CCR1 = (uint32_t)TIM_PERIOD/2;

	TIM_Cmd(HW_SERVO_TIMER, ENABLE);
}

void buzzer_start(void) 
{
    chThdCreateStatic(buzzer_thread_wa, sizeof(buzzer_thread_wa), LOWPRIO, buzzer_thread_fn, NULL);
}


static void buzzer_on(void)
{
   TIM_Cmd(HW_SERVO_TIMER, ENABLE);
}

static void buzzer_off(void)
{
   TIM_Cmd(HW_SERVO_TIMER, DISABLE);
}


static THD_FUNCTION(buzzer_thread_fn, arg) 
{
    (void)arg;

    chRegSetThreadName("buzzer_thread");
    (void)ADC3_PIN;
    (void)ADC3_PORT;

    pwm_setup();

    for(;;) {
        buzzer_on();
        chThdSleepMilliseconds(1000);
        buzzer_off();
        chThdSleepMilliseconds(1000);
    }

}

