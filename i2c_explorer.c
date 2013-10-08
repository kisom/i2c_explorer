/*
 * Copyright (c) 2013 by Kyle Isom <kyle@tyrfingr.is>.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "i2c.h"
#include "serial.h"


#define I2C_DEVICE	0x50

char	INVALID_CMD[] = "\r\nInvalid command.\r\n";
char	ERROR[] = "\r\nError performing command.\r\n";


#define LLEN	32
enum emode {
	M_INVALID,
	M_READ,
	M_WRITE,
	M_ADDR
};


void
zeroline(char *s)
{
	int	i;

	for (i = 0; i < LLEN; i++)
		s[i] = 0;
}


int
main()
{
	enum emode	 mode = M_INVALID;
	char		 in;
	char		 line[LLEN+1];
	int		 i = 0;
	unsigned int	 addr = 0;
	char		 nstr[LLEN+1];

	i2c_init();
	serial_init(9600, 0);

	while (1) {
		line[i] = serial_block_receive_byte();
		if (i == LLEN)
			i = 0;
		if (line[i] != 0xa) {
			i++;
			continue;
		}

		if (0 == strncmp("r", line, 2)) {
			mode = M_READ;
			i = 0;
			zeroline(line);
			serial_transmit((unsigned char *)"MODE: READ\r\n", 12);
			continue;
		}

		if (0 == strncmp("w", line, 2)) {
			mode = M_WRITE;
			i = 0;
			zeroline(line);
			serial_transmit((unsigned char *)"MODE: WRITE\r\n", 13);
			continue;
		}
			
		if (0 == strncmp("a", line, 2)) {
			mode = M_ADDR;
			i = 0;
			zeroline(line);
			serial_transmit((unsigned char *)"MODE: ADDR\r\n", 12);
			continue;
		}

		addr = strtol(line, NULL, 10);
		if ((0 == addr) && (ERANGE == errno))
			mode = M_INVALID;

		switch (mode) {
		case M_READ:
			if (i2c_readbyte(addr, I2C_DEVICE, 0, &in)) {
				serial_transmit((unsigned char *)ERROR,
				    strlen(ERROR));
			} else {
				itoa((int)in, nstr, 10);	
				serial_transmit((unsigned char *)nstr,
				    strlen(nstr));
				serial_transmit((unsigned char *)"\r\n", 2);
				zeroline(nstr);
			}
			break;
		case M_WRITE:
			if (i2c_sendbyte(addr, I2C_DEVICE, 0, line[0])) {
				serial_transmit((unsigned char *)ERROR,
				    strlen(ERROR));
			} else {
				serial_transmit((unsigned char *)"ok\r\n", 4);
			}
			break;
		case M_ADDR:
			serial_transmit((unsigned char *)"address is now ", 14);
			itoa(addr, nstr, 16);
			serial_transmit((unsigned char *)nstr, strlen(nstr));
			serial_transmit((unsigned char *)"\r\n", 2);
			zeroline(nstr);
			break;
		default:
			serial_transmit((unsigned char *)INVALID_CMD, strlen(INVALID_CMD));
		}
	}
}
