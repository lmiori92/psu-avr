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

#include "uart.h"

#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define __USE_GNU   /* pthread_yield() */
#include "pthread.h"

FILE uart_output;
FILE uart_input;

extern char *g_serial_port_path;

static t_uart_cb uart_cb = NULL;

static int fd;
static pthread_t uart_rx_thread;

static void set_blocking (int fd, int should_block)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr", errno);
                return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
                printf ("error %d setting term attributes", errno);
}

static int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                printf ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

void *uart_rx_thread_worker(void *params)
{
    int uart_fd = *((int*)params);
    char tmp;
    uint32_t retval;

    while(1)
    {
        retval = read(uart_fd, &tmp, 1);
        if (retval > 0)
        {
            uart_cb(tmp);
        }

        pthread_yield();
    }

}

void uart_init(void)
{

    fd = open (g_serial_port_path, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
            printf("error %d opening %s: %s", errno, g_serial_port_path, strerror (errno));
            return;

    }

    set_interface_attribs(fd, B9600, 0);    /* set speed to 9600 bps, 8n1 (no parity) */
    set_blocking(fd, 1);                      /* set blocking operations */

    pthread_create(&uart_rx_thread, NULL, uart_rx_thread_worker, &fd);

}

void uart_callback(t_uart_cb cb)
{
    uart_cb = cb;
}

void uart_putcharu(char c, FILE *stream)
{
    write(fd, &c, 1);
    /* blocking operation, no buffering! */
    tcdrain(fd);
    fsync(fd);
}

void uart_putstring(char *str)
{
    while (*str != 0)
    {
        uart_putchar(*str, NULL);
        str++;
    }
}

char uart_getchar(FILE *stream)
{
    char chr;
    return read(fd, &chr, 1);
}
