/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Randy D. Wood
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "lcd.h"
#include "kernel.h"
#include "sprintf.h"
#include "button.h"
#include "file.h"
#include "mpeg.h"
#include "system.h"
#include "i2c.h"
#include "string.h"
#include "buffer.h"

#define IRQ0_EDGE_TRIGGER 0x80

/* test code, to be removed later */
extern union /* defined in main.c */
{
    unsigned char  port8 [512];
    unsigned short port16[256];
    unsigned       port32[128];
} startup_io;


bool rolo_io_load(char* filename)
{
    int fd;
    fd = open(filename, O_RDONLY);
    if(fd >= 0) /* no complaint if it doesn't exit */
    {
        read(fd, (void *)&startup_io, sizeof(startup_io));
        close(fd);
        return true; /* success */
    }
    return false; /* not found */
}

void rolo_io_restore(void)
{
    int i;
    for (i = 0; i < 512; i++)
    {   /* most can be written with 8 bit access */
        *((volatile unsigned char*)0x5FFFE00 + i) = startup_io.port8[i];
    }
    /* some don't allow that, write with 32 bit if aligned */
    *((volatile unsigned char*)0x5FFFF40) = startup_io.port32[0x140/4];
    *((volatile unsigned char*)0x5FFFF44) = startup_io.port32[0x144/4];
    *((volatile unsigned char*)0x5FFFF50) = startup_io.port32[0x150/4];
    *((volatile unsigned char*)0x5FFFF54) = startup_io.port32[0x154/4];
    *((volatile unsigned char*)0x5FFFF60) = startup_io.port32[0x160/4];
    *((volatile unsigned char*)0x5FFFF70) = startup_io.port32[0x170/4];
    *((volatile unsigned char*)0x5FFFF74) = startup_io.port32[0x174/4];

    /* write the rest with 16 bit */
    *((volatile unsigned short*)0x5FFFF4A) = startup_io.port16[0x14A/2];
    *((volatile unsigned short*)0x5FFFF5A) = startup_io.port16[0x15A/2];
    *((volatile unsigned short*)0x5FFFF6A) = startup_io.port16[0x16A/2];
    *((volatile unsigned short*)0x5FFFF7A) = startup_io.port16[0x17A/2];
}
/* end of test code */



static void rolo_error(char *text)
{
    lcd_clear_display();
    lcd_puts(0, 0, "ROLO error:");
    lcd_puts_scroll(0, 1, text);
    lcd_update();
    button_get(true);
    button_get(true);
    button_get(true);
    lcd_stop_scroll();
}
/***************************************************************************
 *
 * Name: rolo_load_app(char *filename,int scrambled)
 * Filename must be a fully defined filename including the path and extension
 *
 ***************************************************************************/
int rolo_load(char* filename) __attribute__ ((section (".topcode")));
int rolo_load(char* filename)
{
    int fd,slen;
    unsigned long length,file_length,i;
    unsigned short checksum,file_checksum;
    unsigned char* ramstart = (void*)0x09000000;
    void (*start_func)(void) = (void*)ramstart + 0x200;
    bool restore_io; /* debug value */

    lcd_clear_display();
    lcd_puts(0, 0, "ROLO...");
    lcd_puts(0, 1, "Loading");
    lcd_update();

    mpeg_stop();

    fd = open(filename, O_RDONLY);
    if(-1 == fd) {
        rolo_error("File not found");
        return -1;
    }

    /* Read file length from header and compare to real file length */
    length=lseek(fd,0,SEEK_END)-FIRMWARE_OFFSET_FILE_DATA;
    lseek(fd, FIRMWARE_OFFSET_FILE_LENGTH, SEEK_SET);
    if(read(fd, &file_length, 4) != 4) {
        rolo_error("Error Reading File Length");
        return -1;
    }
    if (length != file_length) {
        rolo_error("File length mismatch");
        return -1;
    }

    /* Read and save checksum */
    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);
    if (read(fd, &file_checksum, 2) != 2) {
        rolo_error("Error Reading checksum");
        return -1;
    }
    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    /* verify that file can be read and descrambled */
    if ((mp3buf + (2*length)+4) >= mp3end) {
        rolo_error("Not enough room to load file");
        return -1;
    }

    if (read(fd, &mp3buf[length], length) != (int)length) {
        rolo_error("Error Reading File");
        return -1;
    }

    lcd_puts(0, 1, "Descramble");
    lcd_update();

    /* descramble */
    slen = length/4;
    for (i = 0; i < length; i++) {
        unsigned long addr = ((i % slen) << 2) + i/slen;
        unsigned char data = mp3buf[i+length];
        data = ~((data >> 1) | ((data << 7) & 0x80)); /* poor man's ROR */
        mp3buf[addr] = data;
    }

    /* Compute checksum and verify against checksum from file header */
    checksum=0;
    for (i=0; i<length; i++)
        checksum += mp3buf[i];

    if (checksum != file_checksum) {
        rolo_error("Checksum Error");
        return -1;
    }

    restore_io = rolo_io_load("/startup_io.bin"); /* test code */

    lcd_puts(0, 1, "Executing     ");
    lcd_update();

    /* Disable interrupts */
    asm("mov #15<<4,r6\n"
        "ldc r6,sr");

    /* Calling these 2 initialization routines was necessary to get the
       the origional Archos version of the firmware to load and execute. */
    system_init();           /* Initialize system for restart */
    i2c_init();              /* Init i2c bus - it seems like a good idea */
    ICR = IRQ0_EDGE_TRIGGER; /* Make IRQ0 edge triggered */
	PAIOR = 0x0FA0;          /* needed when flashed, probably model-specific */

    if (restore_io) /* test code */
        rolo_io_restore(); /* restore the I/Os from the file content */

    /* move firmware to start of ram */
    for ( i=0; i < length/4+1; i++ )
        ((unsigned int*)ramstart)[i] = ((unsigned int*)mp3buf)[i];

    start_func(); /* start new firmware */

    return 0; /* this is never reached */
}
