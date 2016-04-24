// adc_to_pwm.pde
// ADC to PWM converter
// guest - openmusiclabs.com - 1.9.13
// options table at http://wiki.openmusiclabs.com/wiki/PWMDAC
// takes in audio data from the ADC and plays it out on
// Timer1 PWM.  16b, Phase Correct, 31.25kHz - although ADC is 10b.

#include <avr/io.h>
#include <avr/interrupt.h>

#include "pwm.h"

#define PWM_FREQ 0x03FF // pwm frequency - see table
#define PWM_MODE 0 // Fast (1) or Phase Correct (0)
#define PWM_QTY 2 // number of pwms, either 1 or 2

static t_pwm_channel pwm_channels[PWM_CHANNEL_NUM];

void pwm_init(void)
{

    uint8_t i;

    for (i = 0; i < (uint8_t)PWM_CHANNEL_NUM; i++)
    {
        pwm_channels[i].duty = 0;
        pwm_channels[i].resolution = 0;
        pwm_channels[i].channel = i;
    }

    /* PWM_0 and PWM_1: 10 bit resolution */

    TCCR1A = (((PWM_QTY - 1) << 5) | 0x80 | (PWM_MODE << 1)); //
    TCCR1B = ((PWM_MODE << 3) | 0x11); // ck/1
//    TIMSK1 = 0x20; // interrupt on capture interrupt
    ICR1H = (PWM_FREQ >> 8);
    ICR1L = (PWM_FREQ & 0xff);

    pwm_channels[PWM_CHANNEL_0].resolution = 0x3FF;
    pwm_channels[PWM_CHANNEL_1].resolution = 0x3FF;

    /* PWM_2 and PWM_3: 8-bit resolution */

    /* PWM Phase Correct and Clear OCR2x on match */
    TCCR2A = (1 << COM2A1) | (1 << COM2B1) | (1 << WGM20);//
    /* set up timer with prescaler */
    TCCR2B = (0 << WGM22) | (1 << CS20);

    /* initialize counter */
    //OCR2A = 200;
    //OCR2B = 20;

    /* enable output compare match interrupt */
//    TIMSK0 |= (1 << OCIE0A);

    /* Set output pins */
    DDRB |= (1 << PIN1) | (1 << PIN2) | (1 << PIN3);
    DDRD |= (1 << PIN3);

    pwm_channels[PWM_CHANNEL_2].resolution = 0xFF;
    pwm_channels[PWM_CHANNEL_3].resolution = 0xFF;

    //OCR1AH = 0;
    //OCR1AL = 0x7F;

    //OCR1BH = 0x01;
    //OCR1BL = 0x34;

}

void pwm_set(e_pwm_channel pwm_channel, uint16_t duty)
{

    pwm_channels[pwm_channel].duty = duty;

    switch(pwm_channel)
    {
    case PWM_CHANNEL_0:
        OCR1AH = duty >> 8;
        OCR1AL = duty;
        break;
    case PWM_CHANNEL_1:
        OCR1BH = duty >> 8;
        OCR1BL = duty;
        break;
    case PWM_CHANNEL_2:
        OCR2A = duty;
        break;
    case PWM_CHANNEL_3:
        OCR2B = duty;
        break;
    default:
        break;
    }
}

uint16_t pwm_get_resolution(e_pwm_channel channel)
{
    return pwm_channels[channel].resolution;
}

  //unsigned int temp1 = 0xFF;//ADCL; // you need to fetch the low byte first
  //uint16_t temp2 = 0x7F;//ADCH;
  //unsigned int a= 0;

//ISR(TIMER1_CAPT_vect) {

  // get ADC data

  // although ADCH and ADCL are 8b numbers, they are represented
  // here by unsigned ints, just to demonstrate how you would
  // use numbers larger than 8b.  also, be sure you use unsigned
  // ints for this operation.  if you have a signed int (a regular
  // int), add 0x8000 and cast it to an unsigned int before sending
  // it out to OCR1AH or OCR1AL.
  // example:
  // int temp3 = 87;
  // unsigned int temp4 = temp3 + 0x8000;
  // OCR1AH = temp4 >> 8;
  // OCR1AL = temp4;

  // output high byte on OC1A
//  OCR1AH = temp2 >> 8; // takes top 8 bits
//  OCR1AL = temp2; // takes bottom 8 bits

  // output low byte on OC1B
  //OCR1BH = temp1 >> 8;
  //OCR1BL = temp1;


//}

