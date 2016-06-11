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

#include "psu.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool g_master_or_slave = false;
char *g_serial_port_path;

int main(int argc, char *argv[])
{

    if (argc >= 3)
    {
        /* master or slave? */
        g_master_or_slave = strcmp(argv[1], "master") == 0 ? true : false;

        /* serial port path */
        g_serial_port_path = argv[2];

        psu_app();
    }
    else
    {
        printf("Arguments are missing...stopping.\r\n");
    }

}
