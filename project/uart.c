/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Lorenzo Miori (C) 2016 [ 3M4|L: memoryS60<at>gmail.com ]

    Version History
        * 1.0 initial

*/

#include <avr/io.h>
#include <stdio.h>
#include "avr/power.h"

#ifndef BAUD
#define BAUD 115200
#endif
#include <util/setbaud.h>

#define BAUDRATE 115200
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)

/* http://www.cs.mun.ca/~rod/Winter2007/4723/notes/serial/serial.html */

#define    UCSRA    UCSR0A
#define    UCSRB    UCSR0B
#define    UCSRC    UCSR0C
#define    UBRRH    UBRR0H
#define    UBRRL    UBRR0L
#define    UDRE    UDRE0
#define    UDR    UDR0
#define    RXC    RXC0
#define    RXEN    RXEN0
#define    TXEN    TXEN0
#define    UCSZ1    UCSZ01
#define    UCSZ0    UCSZ00

void uart_init(void) {

    /* Enable UART power */
    power_usart0_enable();

    /* configure ports double mode */
    #if USE_2X
        UCSRA |= _BV(U2X0);
    #else
        UCSRA &= ~(_BV(U2X0));
    #endif

    /* configure the ports speed */
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

    // asynchronous, 8N1 mode
//    UCSR0C |= 0x06;
    UCSRC = _BV(UCSZ1) | _BV(UCSZ0);


    // rx/tx enable
    UCSR0B = _BV(RXEN) | _BV(TXEN);

//    UCSR0B = (1<<RXEN0)|(1<<TXEN0); /* Enable RX and TX */
//    UCSR0C = (3<<UCSZ00);           /* 8-bit data */

//    UBRRH = UBRRH_VALUE;
//    UBRRL = UBRRL_VALUE;




}

void uart_putchar(char c, FILE *stream) {
    if (c == '\n') {
        uart_putchar('\r', stream);
    }
    loop_until_bit_is_set(UCSRA, UDRE);
    UDR = c;
}

char uart_getchar(FILE *stream) {
    loop_until_bit_is_set(UCSRA, RXC);
    return UDR;
}
