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
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

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

    /* Configure the timer to expire after 100 nanoseconds... */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 100;
    /* ... and every 100 nanoseconds after that. */
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

void timer_delay_ms(uint16_t ms)
{
    int signum = 0;
    int retval = 0;
    sigset_t set;
    uint32_t counts = 0;

    /* Wait for n SIGALRM, where n is cycles of 100 nanoseconds.
     * Hence, we multiply the count number by the granularity
     * of the timer tick and of course we normalize milliseconds to nanoseconds.
     * NOTE: if sigwait returns error, timer_delay_ms will immediately return */

    sigaddset(&set, SIGALRM);   /* The signal we are interested in */

    do
    {
        retval = sigwait(&set, &signum);
        counts++;
    } while((retval != -1) && ((counts) < ((uint32_t)ms * 10)));
}

/**
 * timer_handler
 *
 * @brief Timer comparator interrupt routine
 * */
static void timer_handler (int signum)
{
    g_timestamp+=100;
}

