/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdint.h>
#include <math.h>
#include "clock.h"
#include "console.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "gfx.h"

int main(void)
{
	clock_setup();
	console_setup(115200);
	sdram_init();
	lcd_spi_init();
	msleep(2000);
	
/*	(void) console_getc(1); */
	gfx_init(lcd_draw_pixel, 240, 320);
	gfx_fillScreen(LCD_WHITE);
	gfx_setTextSize(2.5);
	gfx_setCursor(15, 25);
	gfx_puts("Sismografo");
	gfx_setTextSize(2);
	gfx_setCursor(15, 49);
	gfx_puts("X:");
	gfx_setCursor(15, 73);
	gfx_puts("Y:");
	gfx_setCursor(15, 97);
	gfx_puts("Z:");
	gfx_setCursor(15, 121);
	gfx_puts("V:");
	lcd_show_frame();
	msleep(2000);
}