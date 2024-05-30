/*
    nfx9000rtu.c
    
    Based on a work (test-modbus program, part of libmodbus) which is
    Copyright (C) 2001-2005 StÃ©phane Raimbault <stephane.raimbault@free.fr>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.


    This is a userspace HAL program, which may be loaded using the halcmd "loadusr" command:
        loadusr nfx9000rtu

    There are several command-line options.  Options that have a set list of possible values may
        be set by using any number of characters that are unique.  For example, --rate 9 will use
        a baud rate of 9600, since no other available baud rates start with "9"
    -b or --bits <n> (default 8)
        Set number of data bits to <n>, where n must be from 7 or 8
    -d or --device <path> (default /dev/ttyS0)
        Set the name of the serial device node to use
    -g or --debugloadusr linuxcnc
        Turn on debugging messages.  This will also set the verbose flag.  Debug moC-style format specified de will cause
        all modbus messages to be printed in hex on the terminal.
    -n or --name <string> (default nfx9000rtu)
        Set the name of the HAL module.  The HAL comp name will be set to <string>, and all pin
        and parameter names will begin with <string>.
    -p or --parity {even,odd,none} (defalt none)
        Set serial parity to even, odd, or none.
    -r or --rate <n> (default 9600)
        Set baud rate to <n>.  It is an error if the rate is not one of the following:
        4800, 9600, 19200, 38400
    -s or --stopbits {1,2} (default 2)
        Set serial stop bits to 1 or 2C-style format specified 
    -t or --target <n> (default 1)loadusr linuxcnc
        Set MODBUS target (slave) number.  This must match the device number you set on the nfx9000.
    -v or --verbose
        Turn on debug messages.  Note that if there are serial errors, this may become annoying.
        At the moment, it doesn't make much difference most of the time.
    -h
        Displays this help message.
    
    An example:
        loadusr nfx9000rtu --bits 8 --device /dev/ttyS0 --parity none --rate 9600 --stopbits 2 --target 1

*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include "rtapi.h"
#include "hal.h" 
#include "modbus.h"

/* 
Avaiable Modbus Commands
03h - Read N registers, upto 12
06H - Write 1 register
*/

/* Must define the number of READ registers and  their starting address. Also 
define how many write registers. Write bit addresses are defined in write function*/
#define MAX_READ_HOLD_REGS 10
#define MAX_WRITE_HOLD_REGS 2

#define START_REGISTER_R 0x2100 /* For NFX9000 Drives only */
#define NUM_REGISTERS_R 7

#undef DEBUG
//#define DEBUG

/* modbus slave data struct - variables needed for reads and writes to nfx9000 */
typedef struct {
	int slave;              /* slave address */
	int read_reg_start;	/* starting read register number */
	int read_reg_count;     /* number of registers to read */
	int write_reg_start;    /* starting write register number */
	int write_reg_count;    /* number of registers to write */
} slavedata_t;

/* HAL data struct */
typedef struct {
    hal_u32_t *errorstatus;
    hal_u32_t *drivestatus;
    hal_float_t *commfrequency;
    hal_float_t *outfrequency;
    hal_float_t *outcurrent;
    hal_float_t *horsepower;
    hal_float_t *outvoltage;
    hal_float_t *dcbusvoltage;
    hal_float_t *freqset;
    hal_bit_t *run;
    hal_bit_t *forward;
    hal_bit_t *nofault;
    hal_s32_t errorcount;
    hal_float_t looptime;
    hal_s32_t retval;
} haldata_t;

static int done;
char *modname = "nfx9000rtu";

static struct option long_options[] = {
    {"bits", 1, 0, 'b'},
    {"device", 1, 0, 'd'},
    {"debug", 0, 0, 'g'},
    {"help", 0, 0, 'h'},
    {"name", 1, 0, 'n'},
    {"parity", 1, 0, 'p'},
    {"rate", 1, 0, 'r'},
    {"stopbits", 1, 0, 's'},
    {"target", 1, 0, 't'},
    {"verbose", 0, 0, 'v'},
    {0,0,0,0}
};

static char *option_string = "b:d:hn:p:r:s:t:v";
static char *bitstrings[] = {"7", "8", NULL};
static char *paritystrings[] = {"even", "odd", "none", NULL};
static char *ratestrings[] = {"4800", "9600", "19200", NULL};
static char *stopstrings[] = {"1", "2", NULL};

/*************/
/* Functions */
/*************/

void usage(int argc, char **argv) {
    printf("Usage:  %s [options]\n", argv[0]);
    printf(
    "This is a userspace HAL program, typically loaded using the halcmd \"loadusr\" command:\n"
    "    loadusr nfx9000rtu\n"
    "There are several command-line options.  Options that have a set list of possible values may\n"
    "    be set by using any number of characters that are unique.  For example, --rate 9 will use\n"
    "    a baud rate of 9600, since no other available baud rates start with \"9\"\n"
    "-b or --bits <n> (default 8)\n"
    "    Set number of data bits to <n>, where n must be from 7 or 8\n"
    "-d or --device <path> (default /dev/ttyS0)\n"
    "    Set the name of the serial device node to use\n"
    "-g or --debug\n"
    "    Turn on debugging messages.  This will also set the verbose flag.  Debug mode will cause\n"
    "    all modbus messages to be printed in hex on the terminal.\n"
    "-h or --help\n"
    "    Displays this help message.\n"
    "-n or --name <string> (default nfx9000rtu)\n"
    "    Set the name of the HAL module.  The HAL comp name will be set to <string>, and all pin\n"
    "    and parameter names will begin with <string>.\n"
    "-p or --parity {even,odd,none} (defalt none)\n"
    "    Set serial parity to even, odd, or none.\n"
    "-r or --rate <n> (default 9600)\n"
    "    Set baud rate to <n>.  It is an error if the rate is not one of the following:\n"
    "    4800, 9600, 19200, 38400\n"
    "-s or --stopbits {1,2} (default 2)\n"
    "    Set serial stop bits to 1 or 2\n"
    "-t or --target <n> (default 1)\n"
    "    Set MODBUS target (slave) number.  This must match the device number you set on the nfx9000.\n"
    "-v or --verbose\n"
    "    Turn on debug messages.  Note that if there are serial errors, this may become annoying.\n"
    "    At the moment, it doesn't make much difference most of the time.\n");
}

int match_string(char *string, char **matches) {
    int len, which, match;
    which=0;
    match=-1;
    if ((matches==NULL) || (string==NULL)) return -1;
    len = strlen(string);
    while (matches[which] != NULL) {
        if ((!strncmp(string, matches[which], len)) && (len <= strlen(matches[which]))) {
            if (match>=0) return -1;        // multiple matches
            match=which;
        }
        ++which;
    }
    return match;
}

int read_data(modbus_param_t *param, slavedata_t *slavedata, haldata_t *hal_data_block) {
    short int receive_data[MAX_READ_HOLD_REGS];	/* a little padding in there */
    int retval;
    float commfrequency, outfrequency, outcurrent, outvoltage, dcbusvoltage;

    /* can't do anything with a null HAL data block */
    if (hal_data_block == NULL)
        return -1;
    /* but we can signal an error if the other params are null */
    if ((param==NULL) || (slavedata == NULL)) {
        hal_data_block->errorcount++;
        return -1;
    }
    retval = read_holding_registers(param, slavedata->slave, slavedata->read_reg_start,
                                slavedata->read_reg_count, receive_data);
    if (retval==slavedata->read_reg_count) {
        retval = 0;
        hal_data_block->retval = retval;
        if (retval==0) {
        *(hal_data_block->errorstatus) = receive_data[0];
        *(hal_data_block->drivestatus) = receive_data[1];
	commfrequency = receive_data[2];
        *(hal_data_block->commfrequency) = commfrequency/10;
	outfrequency = receive_data[3];
        *(hal_data_block->outfrequency) = outfrequency/10;
        outcurrent = receive_data[4];
        *(hal_data_block->outcurrent) = outcurrent/10;
	dcbusvoltage = receive_data[5];
        *(hal_data_block->dcbusvoltage) = dcbusvoltage/10;
	outvoltage = receive_data[6];
        *(hal_data_block->outvoltage) = outvoltage;
	*(hal_data_block->horsepower) = (outvoltage*(outcurrent/10))/746;
        retval = 0;
        }
    } else {
        hal_data_block->retval = retval;
        hal_data_block->errorcount++;
        retval = -1;
    }
    return retval;
}

int write_data(modbus_param_t *param, slavedata_t *slavedata, haldata_t *haldata) {
/*
Command Register Bits
...543210T
...01....  Forward
...10....  Reverse
.......01  Stop
.......10  Run
*/

    uint16_t write_data[MAX_WRITE_HOLD_REGS];
    int retval, fault;
    int command = 0; 

    /* can't do anything with a null HAL data block */
    if (haldata == NULL)
        return -1;
    /* but we can signal an error if the other params are null */
    if ((param==NULL) || (slavedata == NULL)) {
        haldata->errorcount++;
        return -1;
    }

    if (*(haldata->forward)==0) { 
        // set the spindle to reverse
        command = 0b00100000;
    } else {
        // if we reach here, keep the spindle in forward mode as default.
        command = 0b00010000;
    }

    if (*(haldata->run)==0){
        command |= (1<<0);
    } else {
        command |= (1<<1);
    }

    if(*(haldata->nofault)==0) {
        fault = 0b001; // set the drive to fault mode (EF is displayed on VFD display).
    } else {
        fault = 0b010; // reset any faults and enable the VFD.
    }

    *(haldata->freqset) = abs(*(haldata->freqset)); // ensure only positive rpms.
    if(*(haldata->freqset) >= 120) {
        write_data[1] = 120*100; // max frequency is 120hz.
    } else if(*(haldata->freqset) <= 7) {
        write_data[1] = 7*100; // min frequency is 7hz.
    } else {
	write_data[1] = *(haldata->freqset)*100; //frequency set
    }

    write_data[0] = command; //controls both run,stop,forward,reverse
    write_data[2] = fault; //set the drive to fault or reset a fault
    /* write the data. */
    preset_single_register(param, slavedata->slave, 0x2000, write_data[0]);
    preset_single_register(param, slavedata->slave, 0x2001, write_data[1]);
    preset_single_register(param, slavedata->slave, 0x2002, write_data[2]);
    haldata->retval = retval;
    return retval;
}

static void quit(int sig) {
    done = 1;
}

/*************/
/* Main      */
/*************/

int main(int argc, char **argv)
{
    int retval;
    modbus_param_t mb_param;
    haldata_t *haldata;
    slavedata_t slavedata;
    int hal_comp_id;
    struct timespec loop_timespec, remaining;
    int baud, bits, stopbits, debug, verbose;
    char *device, *parity, *endarg;
    int opt;
    int argindex, argvalue;
    done = 0;

    // assume that nothing is specified on the command line, set to ModIO normal settings
    baud = 9600;
    bits = 8;
    stopbits = 2;
    debug = 0;
    verbose = 0;
    device = "/dev/ttyS0";
    parity = "none";

    /* slave / register info */
    slavedata.slave = 1; // this is the slave address.
    slavedata.read_reg_start = START_REGISTER_R;
    slavedata.read_reg_count = NUM_REGISTERS_R;

    // process command line options
    while ((opt=getopt_long(argc, argv, option_string, long_options, NULL)) != -1) {
        switch(opt) {
            case 'b':   // serial data bits, probably should be 8 (and defaults to 8)
                argindex=match_string(optarg, bitstrings);
                if (argindex<0) {
                    printf("nfx9000rtu: ERROR: invalid number of bits: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                bits = atoi(bitstrings[argindex]);
                break;
            case 'd':   // device name, default /dev/ttyS0
                // could check the device name here, but we'll leave it to the library open
                if (strlen(optarg) > FILENAME_MAX) {
                    printf("nfx9000rtu: ERROR: device node name is too long: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                device = strdup(optarg);
                break;
            case 'g':
                debug = 1;
                verbose = 1;
                break;
            case 'n':   // module base name
                if (strlen(optarg) > HAL_NAME_LEN-20) {
                    printf("nfx9000rtu: ERROR: HAL module name too long: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                modname = strdup(optarg);
                break;
            case 'p':   // parity, should be a string like "even", "odd", or "none"
                argindex=match_string(optarg, paritystrings);
                if (argindex<0) {
                    printf("nfx9000rtu: ERROR: invalid parity: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                parity = paritystrings[argindex];
                break;
            case 'r':   // Baud rate, 38400 default
                argindex=match_string(optarg, ratestrings);
                if (argindex<0) {
                    printf("nfx9000rtu: ERROR: invalid baud rate: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                baud = atoi(ratestrings[argindex]);
                break;
            case 's':   // stop bits, defaults to 1
                argindex=match_string(optarg, stopstrings);
                if (argindex<0) {
                    printf("nfx9000rtu: ERROR: invalid number of stop bits: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                stopbits = atoi(stopstrings[argindex]);
                break;
            case 't':   // target number (MODBUS ID), default 1
                argvalue = strtol(optarg, &endarg, 10);
                if ((*endarg != '\0') || (argvalue < 1) || (argvalue > 254)) {
                    printf("nfx9000rtu: ERROR: invalid slave number: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                slavedata.slave = argvalue;
                break;
            case 'v':   // verbose mode (print modbus errors and other information), default 0
                verbose = 1;
                break;
            case 'h':
            default:
                usage(argc, argv);
                exit(0);
                break;
        }
    }

    printf("%s: device='%s', baud=%d, bits=%d, parity='%s', stopbits=%d, address=%d, verbose=%d\n",
           modname, device, baud, bits, parity, stopbits, slavedata.slave, debug);
    /* point TERM and INT signals at our quit function */
    /* if a signal is received between here and the main loop, it should prevent
            some initialization from happening */
    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    /* Assume serial settings of 9600 Baud, 8 bits, no partiy, 2 stop bits, Modbus device 1 */
//    modbus_init_rtu(&mb_param, device, baud, parity, bits, stopbits, verbose);
    modbus_init_rtu(&mb_param, device, baud, parity, bits, stopbits);
    mb_param.debug = debug;
    /* the open has got to work, or we're out of business */
    if (((retval = modbus_connect(&mb_param))!=0) || done) {
        printf("%s: ERROR: couldn't open serial device\n", modname);
        goto out_noclose;
    }

    /* create HAL component */
    hal_comp_id = hal_init(modname);
    if ((hal_comp_id < 0) || done) {
        printf("%s: ERROR: hal_init failed\n", modname);
        retval = hal_comp_id;
        goto out_close;
    }

    /* grab some shmem to store the HAL data in */
    haldata = (haldata_t *)hal_malloc(sizeof(haldata_t));
    if ((haldata == 0) || done) {
        printf("%s: ERROR: unable to allocate shared memory\n", modname);
        retval = -1;
        goto out_close;
    }

    // Make HAL pins
    retval = hal_pin_u32_newf(HAL_OUT, &(haldata->errorstatus), hal_comp_id, "%s.errorstatus", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_u32_newf(HAL_OUT, &(haldata->drivestatus), hal_comp_id, "%s.drivestatus", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->commfrequency), hal_comp_id, "%s.commfrequency", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->outfrequency), hal_comp_id, "%s.outfrequency", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->outcurrent), hal_comp_id, "%s.outcurrent", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->outvoltage), hal_comp_id, "%s.outvoltage", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->dcbusvoltage), hal_comp_id, "%s.dcbusvoltage", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_OUT, &(haldata->horsepower), hal_comp_id, "%s.horsepower", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_float_newf(HAL_IN, &(haldata->freqset), hal_comp_id, "%s.freqset", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_bit_newf(HAL_IN, &(haldata->run), hal_comp_id, "%s.run", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_bit_newf(HAL_IN, &(haldata->forward), hal_comp_id, "%s.forward", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_bit_newf(HAL_IN, &(haldata->nofault), hal_comp_id, "%s.nofault", modname);
    if (retval!=0) goto out_closeHAL;

    /* make default data match what we expect to use */
    *(haldata->freqset) = 0; //set to lowest possible speed.
    *(haldata->run) = 0; //for safety.
    *(haldata->forward) = 1; //start in forward state.
    haldata->errorcount = 0;
    haldata->looptime = 0.8; // Adjust for nfx9000 response time
    hal_ready(hal_comp_id);
    
    /* here's the meat of the program.  loop until done (which may be never) */
    while (done==0) {
        read_data(&mb_param, &slavedata, haldata);
        write_data(&mb_param, &slavedata, haldata);
        /* don't want to scan too fast, and shouldn't delay more than a few seconds */
        if (haldata->looptime < 0.001) haldata->looptime = 0.001;
        if (haldata->looptime > 2.0) haldata->looptime = 2.0;
        loop_timespec.tv_sec = (time_t)(haldata->looptime);
        loop_timespec.tv_nsec = (long)((haldata->looptime - loop_timespec.tv_sec) * 1000000000l);
        nanosleep(&loop_timespec, &remaining);
    }
    
    retval = 0;	/* if we get here, then everything is fine, so just clean up and exit */
    out_closeHAL:
    hal_exit(hal_comp_id);
    out_close:
    modbus_close(&mb_param);
    out_noclose:
    return retval;
}

