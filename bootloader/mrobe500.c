/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Karl Kurbjun
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
 
#include "inttypes.h"
#include "string.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include "ata.h"
#include "fat.h"
#include "disk.h"
#include "font.h"
#include "adc.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "file.h"
#include "common.h"
#include "rbunicode.h"
#include "usb.h"
#include "spi.h"
#include "uart-target.h"
#include "tsc2100.h"

extern int line;

void main(void)
{
/*    unsigned char* loadbuffer;
    int buffer_size;
    int rc;
    int(*kernel_entry)(void);
*/
    power_init();
    system_init();
    kernel_init();
    adc_init();
    button_init();
    backlight_init();
    uartSetup();
    lcd_init();
    font_init();
    spi_init();

    lcd_setfont(FONT_SYSFIXED);

    /* Show debug messages if button is pressed */
//    if(button_read_device())
        verbose = true;

    printf("Rockbox boot loader");
    printf("Version %s", APPSVERSION);

    usb_init();

    #if 0
    /* Enter USB mode without USB thread */
    if(usb_detect())
    {
        const char msg[] = "Bootloader USB mode";
        reset_screen();
        lcd_putsxy( (LCD_WIDTH - (SYSFONT_WIDTH * strlen(msg))) / 2,
                    (LCD_HEIGHT - SYSFONT_HEIGHT) / 2, msg);
        lcd_update();

        ide_power_enable(true);
        ata_enable(false);
        sleep(HZ/20);
        usb_enable(true);

        while (usb_detect())
        {
            ata_spin(); /* Prevent the drive from spinning down */
            sleep(HZ);
        }

        usb_enable(false);

        reset_screen();
        lcd_update();
    }
    #endif

    printf("ATA");
    
    outw(inw(IO_GIO_DIR1)&~(1<<10), IO_GIO_DIR1); // set GIO26 (reset pin) to output
    while(true)
    {
        if (button_read_device() == BUTTON_POWER)
        {
            printf("reset");
            outw(1<<10, IO_GIO_BITSET1);
        }
   //     if ((inw(IO_GIO_BITSET0)&(1<<14)) == 0)
        {
            short x,y,z1,z2, reg;
            tsc2100_read_values(&x, &y, &z1, &z2);
            printf("x: %04x y: %04x z1: %04x z2: %04x", x, y, z1, z2);
            printf("tsadc: %4x", tsc2100_readreg(TSADC_PAGE, TSADC_ADDRESS)&0xffff);
            printf("current tick: %04x", current_tick);
            tsc2100_keyclick(); /* doesnt work :( */
            line -= 3;
        }

    }
#if 0
    rc = ata_init();
    if(rc)
    {
        reset_screen();
        error(EATA, rc);
    }

    printf("disk");
    disk_init();

    printf("mount");
    rc = disk_mount_all();
    if (rc<=0)
    {
        error(EDISK,rc);
    }

    printf("Loading firmware");

    loadbuffer = (unsigned char*) 0x00900000;
    buffer_size = (unsigned char*)0x00100000 - loadbuffer;

    rc = load_firmware(loadbuffer, BOOTFILE, buffer_size);
    if(rc < 0)
        error(EBOOTFILE, rc);

    if (rc == EOK)
    {
        kernel_entry = (void*) loadbuffer;
        rc = kernel_entry();
    }
#endif
}
