#include <stdint.h>
#include <stdbool.h>
#include "ch.h" // ChibiOS

typedef struct  {
	uint32_t freq;
	uint32_t duration;
} beap_t;


typedef struct {
	uint32_t num;
	beap_t beeps[20];
} beep_sequence_t;


typedef struct  {
    systime_t last_beep_time;
    systime_t min_beep_period;
	const beep_sequence_t * sequence;
} beap_guarded_t;



extern const beep_sequence_t BEEP_SEQUENCE_START_BEEP;
extern beap_guarded_t BEEP_LOW_VOLTAGE;
extern beap_guarded_t BEEP_HIGH_VOLTAGE;
extern beap_guarded_t BEEP_FET_TEMP;
extern beap_guarded_t BEEP_SWITCH_OFF;
extern beap_guarded_t BEEP_DUTY_CYCLE;


void buzzer_start(void);


void issue_beep_sequence(const beep_sequence_t * beep);
void issue_beep_guarded(beap_guarded_t * beep);