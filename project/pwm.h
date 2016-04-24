/*
 * pwm.h
 *
 *  Created on: 23 apr 2016
 *      Author: lorenzo
 */

#ifndef PWM_H_
#define PWM_H_

typedef enum
{
    PWM_CHANNEL_0,
    PWM_CHANNEL_1,
    PWM_CHANNEL_2,
    PWM_CHANNEL_3,

    PWM_CHANNEL_NUM
} e_pwm_channel;

typedef struct
{
    e_pwm_channel channel;
    uint16_t      resolution;
    uint16_t      duty;
} t_pwm_channel;

void pwm_init(void);
void pwm_set(e_pwm_channel pwm_channel, uint16_t duty);
uint16_t pwm_get_resolution(e_pwm_channel channel);

#endif /* PWM_H_ */
