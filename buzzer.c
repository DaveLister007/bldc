#include "buzzer.h"
#include "ch.h" // ChibiOS
#include "hal.h"
#include "terminal.h"
#include <stdio.h>
#include "commands.h"




static THD_WORKING_AREA(buzzer_thread_wa, 256);
static THD_FUNCTION(buzzer_thread_fn, arg);
static thread_t *thread = NULL;
static void buzzer_on(void);
static void buzzer_off(void);
static mailbox_t mailbox;
static msg_t mailbox_messages[5];




const beep_sequence_t BEEP_SEQUENCE_START_BEEP = 
{
	4,
	{{1000,200},{1200,200},{1300,200},{1500,200}},
};


const beep_sequence_t BEEP_SEQUENCE_LOW_VOLTAGE =
{
	4,
	{
		{1500,200},{1200,200},{1000,200}, {0,200},
	},
};
beap_guarded_t BEEP_LOW_VOLTAGE = 
{
	.last_beep_time = 0,
	.min_beep_period = S2ST(30), 
	.sequence = &BEEP_SEQUENCE_LOW_VOLTAGE,
};

const beep_sequence_t BEEP_SEQUENCE_HIGH_VOLTAGE =
{
	4,
	{
		{1000,200}, {1200,200}, {1500,200}, {0,200},
	},
};
 beap_guarded_t BEEP_HIGH_VOLTAGE = 
{
	.last_beep_time = 0,
	.min_beep_period = S2ST(30), 
	.sequence = &BEEP_SEQUENCE_HIGH_VOLTAGE,
};

const beep_sequence_t BEEP_SEQUENCE_FET_TEMP = 
{
	4*3,
	{
		{1000,200}, {1500,200},{1000,200}, {0,200},
		{1000,200}, {1500,200},{1000,200}, {0,200},
		{1000,200}, {1500,200},{1000,200}, {0,200},
	},
};
beap_guarded_t BEEP_FET_TEMP = 
{
	.last_beep_time = 0,
	.min_beep_period = S2ST(10), 
	.sequence = &BEEP_SEQUENCE_FET_TEMP,
};


const beep_sequence_t BEEP_SEQUENCE_SWITCH_OFF = 
{
	6,
	{
		{1000,100},{0, 100},{1000,100},{0, 100},{1000,100},{0, 100}
	},
};

beap_guarded_t BEEP_SWITCH_OFF = 
{
	.last_beep_time = 0,
	.min_beep_period = S2ST(1), 
	.sequence = &BEEP_SEQUENCE_SWITCH_OFF,
};

const beep_sequence_t BEEP_SEQUENCE_DUTY_CYCLE = 
{
	1,
	{
		{1000,1000},
	},
};

beap_guarded_t BEEP_DUTY_CYCLE = 
{
	.last_beep_time = 0,
	.min_beep_period = S2ST(3), 
	.sequence = &BEEP_SEQUENCE_DUTY_CYCLE,
};



void pwm_setup(uint32_t freq)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	static const uint32_t TIM_CLOCK = 1000000; // 1Mhz

	TIM_TimeBaseStructure.TIM_Period = (uint32_t)(TIM_CLOCK / freq);
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

	HW_SERVO_TIMER->CCR1 = TIM_TimeBaseStructure.TIM_Period/2;

	TIM_Cmd(HW_SERVO_TIMER, ENABLE);
}

void buzzer_start(void) 
{
	chMBObjectInit(&mailbox, mailbox_messages, sizeof(mailbox_messages) / sizeof(mailbox_messages[0]));

	HW_SERVO_TIM_CLK_EN();

	thread = chThdCreateStatic(buzzer_thread_wa, sizeof(buzzer_thread_wa), LOWPRIO, buzzer_thread_fn, NULL);
}

void issue_beep_sequence(const beep_sequence_t * beep)
{
	chMBPostI(&mailbox, (msg_t)beep);
}

void issue_beep_guarded(beap_guarded_t * beep)
{
	if (chVTTimeElapsedSinceX(beep->last_beep_time) > beep->min_beep_period)
	{
		beep->last_beep_time = chVTGetSystemTime();	
		chMBPostI(&mailbox, (msg_t)beep->sequence);
	}
}

static void buzzer_on(void)
{
	palSetPadMode(HW_SERVO_PORT, HW_SERVO_PIN, PAL_MODE_ALTERNATE(HW_SERVO_GPIO_AF) |
			                                   PAL_STM32_OSPEED_HIGHEST | PAL_STM32_PUDR_FLOATING);
}

static void buzzer_off(void)
{
   palSetPadMode(HW_SERVO_PORT, HW_SERVO_PIN, PAL_MODE_OUTPUT_PUSHPULL);
   palClearPad(HW_SERVO_PORT, HW_SERVO_PIN);
}


static THD_FUNCTION(buzzer_thread_fn, arg) 
{
    (void)arg;

    chRegSetThreadName("buzzer_thread");

    for(;;) {

		msg_t message;
		if (MSG_OK == chMBFetch(&mailbox, &message, TIME_INFINITE))
		{
			beep_sequence_t * current_beep_sequence = (beep_sequence_t *)message;
			
			if (current_beep_sequence != NULL)
			{
				for(uint32_t pos = 0; pos < current_beep_sequence->num; pos++)
				{
					beap_t beap = current_beep_sequence->beeps[pos];

					if (beap.freq == 0)
					{
						buzzer_off();
					}
					else 
					{
						buzzer_off();
						pwm_setup(beap.freq);
						buzzer_on();
					}
					chThdSleepMilliseconds(beap.duration);
				}
				buzzer_off();

				// wait before next beep
				chThdSleepMilliseconds(500);
			}
			else 
			{
				buzzer_off();
			}
		}
    }

}

