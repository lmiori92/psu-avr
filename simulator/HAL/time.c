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

    Lorenzo Miori (C) 2015 [ 3M4|L: memoryS60<at>gmail.com ]

    Version History
        * 1.0 initial

*/

/**
 * @file time.c
 * @author Lorenzo Miori
 * @date Oct 2015
 * @brief Time keeping routines
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* linux libs */
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>


#define TIMER_0_PRESCALER_8     (1 << CS01)                     /**< Prescaler 8 */
#define TIMER_0_PRESCALER_64    ((1 << CS01) | (1 << CS00))     /**< Prescaler 64 */
#define TIMER_0_PRESCALER_256   (1 << CS02)                     /**< Prescaler 256 */

uint32_t g_timestamp;           /**< Global time-keeping variable (resolution: 100us) */

static void timer_handler (int signum);

/**
 * timer_init
 *
 * @brief Initialize timer, interrupt and variable.
 *        Assumes that interrupts are disabled while intializing
 *
 */
void timer_init(void)
{

    struct sigaction sa;
    struct itimerval timer;

    /* Install timer_handler as the signal handler for SIGVTALRM. */
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;
    sigaction (SIGALRM, &sa, NULL);

    /* Configure the timer to expire after 250 msec... */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 100;
    /* ... and every 250 msec after that. */
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 100;
    /* Start a virtual timer. It counts down whenever this process is
       executing. */
    if (setitimer (ITIMER_REAL, &timer, NULL))
    {
        printf("Failed to set the timer :-(\r\n");
    }

}

#ifdef TIMER_DEBUG
#include "stdio.h"
void timer_debug(void)
{
    static uint32_t ts = 0;
    if (g_timestamp > (ts + 1000000L))
    {
        ts = g_timestamp;
        printf("%s\r\n", "1 second trigger");
    }

}
#else
/* No function is available */
#endif

/**
 * timer_handler
 *
 * @brief Timer comparator interrupt routine
 * */
static void timer_handler (int signum)
{
    g_timestamp+=100;
}

