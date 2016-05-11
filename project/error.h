/*
 * error_codes.h
 *
 *  Created on: 11 mag 2016
 *      Author: lorenzo
 */

#ifndef ERROR_H_
#define ERROR_H_

typedef enum _e_error
{
    E_OK,
    E_UNKNOWN,
    E_CRC,
    E_OVERFLOW,
    E_TIMEOUT,
} e_error;

#endif /* ERROR_H_ */
