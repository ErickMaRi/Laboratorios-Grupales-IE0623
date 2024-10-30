/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>,
 * Copyright (C) 2010-2015 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
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
#include <stdio.h>  
#include <math.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include "clock.h"
#include "console.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "gfx.h"

#define L3GD20_SENSITIVITY_250DPS ((float)8.75f)

void setup_spi(void);
uint16_t read_reg(int reg);
uint8_t read_xyz(int16_t vecs[3]);
int print_decimal(int num);
void display_xyz(int16_t vecs[3]);
static void usart_setup(void);
void lcd_main_structure(void);

volatile uint8_t usart_enabled = 0;
void setup_spi(void){
    //Habilitar el reloj
    rcc_periph_clock_enable(RCC_SPI5);
    rcc_periph_clock_enable(RCC_GPIOF | RCC_GPIOC);

    //Habilitar pines de GPIO
    gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN,
			GPIO7 | GPIO8 | GPIO9);
	gpio_set_af(GPIOF, GPIO_AF5, GPIO7 | GPIO8 | GPIO9);
	gpio_set_output_options(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,
				GPIO7 | GPIO9);

    //Chip Select
    gpio_set(GPIOC, GPIO1);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);

    //Inicializar y configurar SPI
    spi_set_master_mode(SPI5);
    spi_set_baudrate_prescaler(SPI5, SPI_CR1_BR_FPCLK_DIV_64);
    spi_set_clock_polarity_0(SPI5);
    spi_set_clock_phase_0(SPI5);
    spi_set_full_duplex_mode(SPI5);
    spi_set_unidirectional_mode(SPI5);
    spi_enable_software_slave_management(SPI5);
    spi_send_msb_first(SPI5);
    spi_set_nss_high(SPI5);
    spi_enable(SPI5);
}

static void gpio_setup(void)
{
	/* Enable GPIOG clock. */
	rcc_periph_clock_enable(RCC_GPIOG);

	/* Set GPIO13 (in GPIO port G) to 'output push-pull'. */
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT,
			GPIO_PUPD_NONE, GPIO13);
}

static void button_setup(void)
{
	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Set GPIO0 (in GPIO port A) to 'input open-drain'. */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
}

static void usart_setup(void)
{
	rcc_periph_clock_enable(RCC_USART1);
	/* Setup USART2 rcc_periph_clock_enableparameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART1);
}

static void adc_setup(void)
{
	rcc_periph_clock_enable(RCC_ADC1);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO3);
	adc_power_off(ADC1);
	adc_disable_scan_mode(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);
	adc_power_on(ADC1);
}

static uint16_t read_adc_naiive(uint8_t channel)
{
	uint8_t channel_array[16];
	channel_array[0] = channel;
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_start_conversion_regular(ADC1);
	while (!adc_eoc(ADC1));
	uint16_t reg16 = adc_read_regular(ADC1);
	return reg16;
}

uint16_t read_reg(int reg)
{
	uint16_t d1, d2;

	d1 = 0x80 | (reg & 0x3f); /* Read operation */
	/* Nominallly a register read is a 16 bit operation */
	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, d1);
	d2 = spi_read(SPI5);
	d2 <<= 8;
	/*
	 * You have to send as many bits as you want to read
	 * so we send another 8 bits to get the rest of the
	 * register.
	 */
	spi_send(SPI5, 0);
	d2 |= spi_read(SPI5);
	gpio_set(GPIOC, GPIO1);
	return d2;
}

uint8_t read_xyz(int16_t vecs[3])
{
	uint8_t	 buf[7];
	int		 i;

	gpio_clear(GPIOC, GPIO1); /* CS* select */
	spi_send(SPI5, 0xc0 | 0x28);
	(void) spi_read(SPI5);
	for (i = 0; i < 6; i++) {
		spi_send(SPI5, 0);
		buf[i] = spi_read(SPI5);
	}
	gpio_set(GPIOC, GPIO1); /* CS* deselect */
	vecs[0] = (buf[1] << 8 | buf[0])*L3GD20_SENSITIVITY_250DPS;
	vecs[1] = (buf[3] << 8 | buf[2])*L3GD20_SENSITIVITY_250DPS;
	vecs[2] = (buf[5] << 8 | buf[4])*L3GD20_SENSITIVITY_250DPS;
	return read_reg(0x27); /* Status register */
}

int print_decimal(int num)
{
	int		ndx = 0;
	char	buf[10];
	int		len = 0;
	char	is_signed = 0;

	if (num < 0) {
		is_signed++;
		num = 0 - num;
	}
	buf[ndx++] = '\000';
	do {
		buf[ndx++] = (num % 10) + '0';
		num = num / 10;
	} while (num != 0);
	ndx--;
	if (is_signed != 0) {
		console_putc('-');
		len++;
	}
	while (buf[ndx] != '\000') {
		console_putc(buf[ndx--]);
		len++;
	}
	return len; /* number of characters printed */
}

// void display_xyz(int16_t vects[3]){
// 	console_puts("X: ");
// 	print_decimal(vects[0]);  
// 	console_puts("\n");

// 	console_puts("Y: ");
// 	print_decimal(vects[1]);  
// 	console_puts("\n");

// 	console_puts("Z: ");
// 	print_decimal(vects[2]);  
// 	console_puts("\n\n");
// }

void lcd_main_structure(void){
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
	// lcd_show_frame();
}

int main(void)
{
	char volt[20];
	float temp;
	uint16_t input_adc3;
	int16_t vecs[3];  // Almacena información de los ejes
	char char_X[10];
    char char_Y[10];
    char char_Z[10];

	clock_setup();     // Se inicializa el reloj
	button_setup();
	gpio_setup();
	setup_spi();       //  Se inicializa SPI5
	usart_setup();
	adc_setup();
	console_setup(115200);  // Se inicializa la consola
	sdram_init();
	lcd_spi_init();
	// console_puts("Leyendo Gyroscopio...\n");

	while (1) {
		if (gpio_get(GPIOA, GPIO0)) {
            usart_enabled = !usart_enabled;  // Toggle bandera
            msleep(300);  // Debounce delay
        }
		if (usart_enabled) {
            gpio_toggle(GPIOG, GPIO13);  // encender y apagar el LED
			// display_xyz(vecs); //Mostrar X, Y, Z en consola
            msleep(500);
        }else {
            gpio_clear(GPIOG, GPIO13);  // Asegurarse de que el LED esté apagado
        }
		read_xyz(vecs);  // Leer X, Y, Z
		sprintf(char_X, "%d", vecs[0]);
        sprintf(char_Y, "%d", vecs[1]);
        sprintf(char_Z, "%d", vecs[2]);

		input_adc3 = read_adc_naiive(3);
		printf("adc3 = %u\n",
			input_adc3);
		temp = input_adc3* 9.0f / 4095.0f;
		sprintf(volt, "%2f", temp);

		lcd_main_structure();
		gfx_setTextSize(2);
		gfx_setCursor(40, 49);
		gfx_puts(char_X);
		gfx_setCursor(40, 73);
		gfx_puts(char_Y);
		gfx_setCursor(40, 97);
		gfx_puts(char_Z);
		gfx_setCursor(40, 121);
		gfx_puts(volt);
		lcd_show_frame();
		// input_adc3 = read_adc_naiive(3);
		// printf("ADC Channel 3 Value: %d\n", input_adc3);
        // display_xyz(vecs); //Mostrar X, Y, Z en consola
		// msleep(2000);
	}

	return 0;
}
