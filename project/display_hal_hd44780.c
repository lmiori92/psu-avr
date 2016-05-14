
#include "display.h"

#include <avr/io.h>

#define SHIFT_LATCH_PIN     PIN3   /**< STCP 74HC595 PIN */
#define SHIFT_CLOCK_PIN     PIN4   /**< SHCP 74HC595 PIN */
#define SHIFT_DATA_PIN      PIN5   /**< DS   74HC595 PIN */

#define SHIFT_PORT          PORTB  /**< Shift PINs PORT */
#define SHIFT_DDR           DDRB   /**< Shift PINs DDR */

#define SHIFT_LATCH_LOW      SHIFT_PORT &= ~(1 << SHIFT_LATCH_PIN);asm("nop")  /**< latch LOW */
#define SHIFT_LATCH_HIGH     SHIFT_PORT |=  (1 << SHIFT_LATCH_PIN);asm("nop")  /**< latch HIGH */
#define SHIFT_CLOCK_LOW      SHIFT_PORT &= ~(1 << SHIFT_CLOCK_PIN);asm("nop")  /**< clock LOW */
#define SHIFT_CLOCK_HIGH     SHIFT_PORT |=  (1 << SHIFT_CLOCK_PIN);asm("nop")  /**< clock HIGH */
#define SHIFT_DATA_LOW       SHIFT_PORT &= ~(1 << SHIFT_DATA_PIN);asm("nop")   /**< data LOW */
#define SHIFT_DATA_HIGH      SHIFT_PORT |=  (1 << SHIFT_DATA_PIN);asm("nop")   /**< data HIGH */

static void shift_init(void)
{
    SHIFT_DDR |= (1 << SHIFT_LATCH_PIN);
    SHIFT_DDR |= (1 << SHIFT_CLOCK_PIN);
    SHIFT_DDR |= (1 << SHIFT_DATA_PIN);
}

static void shift_out(uint8_t byte)
{
    uint8_t i;
    /* Do not transfer the temporary register to the outputs */
    SHIFT_CLOCK_LOW;
    SHIFT_LATCH_LOW;

    for (i = 0; i < 8; i++)
    {
        /* Shift bits out */
        if (((byte >> i) & 0x01U) == 0x01U)
        {
            SHIFT_DATA_HIGH;
        }
        else
        {
            SHIFT_DATA_LOW;
        }
        /* Clocks the bits */
        SHIFT_CLOCK_HIGH;
        SHIFT_CLOCK_LOW;
    }

    /* Transfer the temporary register to the outputs */
    SHIFT_LATCH_HIGH;
    SHIFT_LATCH_LOW;

}

void display_hal_init(void)
{
    shift_init();
    shift_out(0xAA);
}

void display_hal_set_cursor(uint8_t line, uint8_t chr)
{

}

void display_hal_write_char(uint8_t chr)
{

}

void display_hal_cursor_visibility(bool visible)
{

}
