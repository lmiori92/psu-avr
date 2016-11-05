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

/**
 * @file main.c
 * @author Lorenzo Miori
 * @date June 2016
 * @brief The main entry point of the program
 */

#include "app/psu.h"
#include "app/remote.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define __USE_GNU   /* pthread_yield() */
#include "pthread.h"

bool g_master_or_slave = false;
char *g_serial_port_path;

static pthread_t main_thread;

void* main_thread_worker(void *params)
{

    struct timespec tim, tim2;
    tim.tv_nsec = 50000UL;        /* cycle time */

    while (1)
    {
        /* The application already runs the code in a while
         * loop (so does a microcontroller) but */
        psu_app();

        nanosleep(&tim, &tim2);
    }

}
int counter = 0;
void uart_putchar(char c, FILE *stream)
{
    if (counter == 9) c |= 12;
    remote_buffer_to_datagram(c);
    counter++;
}
#include <stdbool.h>
t_remote_datagram_buffer buf;
bool avl = false;

int main(int argc, char *argv[])
{

    if (argc >= 3)
    {
        /* master or slave? */
        g_master_or_slave = strcmp(argv[1], "master") == 0 ? true : false;

        /* serial port path */
        g_serial_port_path = argv[2];

        /* one-time initialization */
       // psu_app_init();

        /* testing of the remote protocol */
        t_remote_datagram_buffer *rem_buf;
        remote_send_alloc(&rem_buf);
        memcpy(rem_buf->data, "Lorenzo", 7);
        rem_buf->datagram.len = 7;
        remote_send_push();
        datagram_buffer_to_remote();

        /* we should now have the decoded datagram again here */

        avl = remote_receive_get(&buf);

        printf("terminated");

        /* start the thread */
//        pthread_create(&main_thread, NULL, main_thread_worker, NULL);

//        pthread_join(main_thread, NULL);

    }
    else
    {
        printf("Arguments are missing...stopping.\r\n");
    }

}
