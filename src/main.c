/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) 2012 Javier Andrés Galaz Jeria <cognhuepan@gascogne.lan>
 * 
 * stee is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * stee is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define FALSE			0
#define TRUE			!0

#define APPEND			FALSE
#define IOSPEED			B9600
#define INPUT_SERIAL	"/dev/ttyS0"
#define OUTPUT_SERIAL   "/dev/ttyS1"
#define SAVE_FILE		"stee_output"

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

speed_t ispeed, ospeed;

enum ports
{
	input_serial,
	output_serial
};

/*
 *
void
print_usage()
{
	printf("Modo de empleo: stee [OPCIONES] [ENTRADA] [SALIDA] [ARCHIVO ENTRADA] [ARCHIVO SALIDA]\n");
	printf("Hace una Tee serial entre ENTRADA y SALIDA.\n\n");
	printf("Los argumentos obligatorios para las opciones largas son\n");
	printf("también obligatorios para las opciones cortas.\n");
	printf("  -a, --append			ARCHIVO es abierto en modo anexar\n");
	printf("  -i, --ispeed			Velocidad del puerto ENTRADA\n");
	printf("  -o, --ospeed			Velocidad del puerto SALIDA\n");
	printf("  -s, --speed			Velocidad de los puertos ENTRADA Y SALIDA\n");
	printf("  -h, --help			Muestra esta ayuda y termina\n");

}
 *
 */
void
print_usage()
{
	printf("Usage: stee [OPTIONS] [INPUT] [OUTPUT] [INPUT FILE] [OUTPUT FILE]\n\
Makes a Serial Tee between INPUT and OUTPUT.\n\nMandatory arguments for\
long options are also obligatory for short options.\n\
  -a, --append			FILES (INPUT AND OUTPUT) are opened on append mode\n\
  -i, --ispeed			INPUT port speed\n\
  -o, --ospeed			OUTPUT port speed\n\
  -s, --speed			INPUT AND OUTPUT ports speed\n\
  -h, --help			Shows this help and exits\n");

}


void
set_options(int serial_device,
            const char *serial_str,
            speed_t speed)
{
	struct termios options;
	tcgetattr(serial_device, &options);

	cfsetispeed(&options, speed);
	cfsetospeed(&options, speed);

	options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
	                     | INLCR | IGNCR | ICRNL | IXON);
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	options.c_cflag &= ~(CSIZE | PARENB);
	options.c_cflag |= CS8;

	tcsetattr(serial_device, TCSANOW, &options);
}

void
set_speed(speed_t speed, int speed_int)
{
	switch (speed_int)
	{
		case 9600:
			speed = B9600;
			break;
		case 19200:
			speed = B19200;
			break;
		case 38400:
			speed = B38400;
			break;
		case 57600:
			speed = B57600;
			break;
		case 115200:
			speed = B115200;
			break;
		case 230400:
			speed = B230400;
			break;
		case 460800:
			speed = B460800;
			break;
		case 500000:
			speed = B500000;
			break;
		case 576000:
			speed = B576000;
			break;
		case 921600:
			speed = B921600;
			break;
		case 1000000:
			speed = B1000000;
			break;
		case 1152000:
			speed = B1152000;
			break;
		case 1500000:
			speed = B1500000;
			break;
		case 2000000:
			speed = B2000000;
			break;
		case 2500000:
			speed = B2500000;
			break;
		case 3000000:
			speed = B3000000;
			break;
		case 3500000:
			speed = B3500000;
			break;
		case 4000000:
			speed = B4000000;
			break;
		default:
			printf("Error: speed %i not supported", speed_int);
	}
}

void
set_serial_speed(int serial_port, char *speed_str)
{
	switch (serial_port)
	{
		case input_serial:
			set_speed(ispeed, atoi(speed_str));
			break;
		case output_serial:
			set_speed(ospeed, atoi(speed_str));
			break;
	}
}

int open_serial_device(const char* serial_str, speed_t speed)
{
	int serial_device;
	char mensaje_error[75];
	serial_device = open(serial_str, O_RDWR | O_NONBLOCK);
	if(serial_device > 0)
	{
		printf("Port %s successfully opened!! (fd: %i)\n", serial_str,
		       serial_device);
	}
	else
	{
		snprintf(mensaje_error, 75, "Couldn't open port '%s'", serial_str);
		perror(mensaje_error);
		exit(-1);
	}
	set_options (serial_device, serial_str, speed);

	if(unlockpt(serial_device))
	{
		snprintf(mensaje_error,
		         75,
		         "Couldn't unlock port '%s'",
		         serial_str);
		perror(mensaje_error);
	}
	return serial_device;
}


int main(int argc, char **argv)
{
	int c;
	int digit_optind = 0;
	char *input_serial_str, *output_serial_str, *ifilename_str, *ofilename_str;
	unsigned char buf;
	int iserialfd, oserialfd;
	FILE *ifile, *ofile;
	int append;

	ispeed = ospeed = IOSPEED;
	input_serial_str = INPUT_SERIAL;
	output_serial_str = OUTPUT_SERIAL;
	ifilename_str = SAVE_FILE;
	ofilename_str = SAVE_FILE;
	append = APPEND;

	//parsear argumentos ¡getopt_long!
	//opciones: velocidad de entrada, velocidad de salida, otras opciones de los
	//puertos serie.
	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"ispeed",  required_argument,  0, 'i'},
			{"ospeed",  required_argument,  0, 'o'},
			{"speed",   required_argument,  0, 's'},
			{"append",  no_argument,		0, 'a'},
			{"help",	no_argument,		0, 'h'},
			{0,			0,					0,  0 }
		};

		c = getopt_long(argc, argv, "a:i:o:s:h", long_options, &option_index);
		if (c == -1) /* Condición de término */
			break;

		switch (c) {
			case 0:
				printf("option %s", long_options[option_index].name);
				if (optarg)
					printf(" with arg %s", optarg);
				printf("\n");
				break;
			case '0':
			case '1':
			case '2':
		        if (digit_optind != 0 && digit_optind != this_option_optind)
				printf("digits occur in two different argv-elements.\n");
		        digit_optind = this_option_optind;
		        printf("option %c\n", c);
		        break;
			case 'i':
				set_serial_speed (input_serial, optarg);
				printf("Speed of input port: %s\n", optarg);
				break;

			case 'o':
				printf("Speed of output port: %s\n", optarg);
				set_serial_speed (output_serial, optarg);
				break;

			case 's':
				printf("Speed of input and output port: %s\n", optarg);
				//setear velocidad ispeed y ospeed a speed
				set_serial_speed (input_serial, optarg);
				set_serial_speed (output_serial, optarg);
				break;

			case 'a':
				printf("Append output to file\n");
				append = TRUE;
				break;

			case 'h':
				print_usage();
				exit(EXIT_SUCCESS);
				break;
			case '?':
				print_usage();
				break;

			default:
				printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	if (optind < argc) {
		int i;
		for (i = optind; i < argc; i++)
		{
			switch(i - optind)
			{
				case 0:
					printf("Input Port: %s\n", argv[i]);
					input_serial_str = argv[i];
					break;
				case 1:
					printf("Output Port: %s\n", argv[i]);
					output_serial_str = argv[i];
					break;
				case 2:
					printf("File to save input: %s\n", argv[i]);
					ifilename_str = argv[i];
					break;
				case 3:
					printf("File to save output: %s\n", argv[i]);
					ofilename_str = argv[i];
					break;
			}
		}
		printf("\n");
	}
	
	//abrir puerto de entrada con velocidad ispeed
	iserialfd = open_serial_device (input_serial_str, ispeed);
	//abrir puerto de salida con velocidad ospeed
	oserialfd = open_serial_device (output_serial_str, ospeed);
	if(append)
	{
		printf("Opening files in append mode\n");
		ifile = fopen(ifilename_str, "a+");
		ofile = fopen(ofilename_str, "a+");
	}
	else
	{
		printf("Opening files in write mode\n");
		ifile = fopen(ifilename_str, "a+");
		ofile = fopen(ofilename_str, "w");
	}
	//timestamp!
	fprintf(ifile, "Timestamp!\n");
	fprintf(ofile, "Timestamp!\n");
	while(TRUE)
	{
		/*
		 *
		 *Leer lo que entra por el puerto arg1, entregarlo en el puerto arg2 y
		 *stdout, escribirlo en el archivo arg3 (archivo de texto)
		 *
		 * */
		//leer lo que entra por el puerto arg1
		while(read(iserialfd, &buf, 1) > 0)
		{
			//entregarlo en el puerto arg2
			write(oserialfd, &buf, 1);
			//stdout
			printf("--> %u\n", buf);
			//escribirlo en el archivo arg3 (archivo de texto)
			fprintf(ifile, "%u ", buf);
		}
		//leer lo que entra por el puerto arg2, entregarlo en el puerto arg1 y
		//stdout, escribirlo en el archivo arg3 (archivo de texto)
		//leer lo que entra por el puerto arg2
		while(read(oserialfd, &buf, 1) > 0)
		{
			//entregarlo en el puerto arg1
			write(iserialfd, &buf, 1);
			//stdout
			printf("%u <--\n", buf);
			//escribirlo en el archivo arg3 (archivo de texto)
			fprintf(ofile, "%u ", buf);
		}
	}
	exit(EXIT_SUCCESS);
}
