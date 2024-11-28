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

//Definiciones para configurar SPI
#define L3GD20_SENSITIVITY_250DPS (0.00875F)
#define GYR_RNW			(1 << 7) /* Write when zero */
#define GYR_MNS			(1 << 6) /* Multiple reads when 1 */
#define GYR_WHO_AM_I		0x0F
#define GYR_OUT_TEMP		0x26
#define GYR_STATUS_REG		0x27
#define GYR_CTRL_REG1		0x20
#define GYR_CTRL_REG1_PD	(1 << 3)
#define GYR_CTRL_REG1_XEN	(1 << 1)
#define GYR_CTRL_REG1_YEN	(1 << 0)
#define GYR_CTRL_REG1_ZEN	(1 << 2)
#define GYR_CTRL_REG1_BW_SHIFT	4
#define GYR_CTRL_REG4		0x23
#define GYR_CTRL_REG4_FS_SHIFT	4
//Utilizados para leer XYZ
#define GYR_OUT_X_L		0x28
#define GYR_OUT_X_H		0x29
#define GYR_OUT_Y_L     0x2A
#define GYR_OUT_Y_H     0x2B
#define GYR_OUT_Z_L     0x2C
#define GYR_OUT_Z_H     0x2D 

static void setup_spi(void);  
uint8_t read_reg(uint8_t command); 
void write_reg(uint8_t reg, uint16_t value); 
void read_xyz(int16_t vecs[3]);
int print_decimal(int num);
static void usart_setup(void);
void lcd_main_structure(void);
//Variables globales
volatile uint8_t usart_enabled = 0;

static void setup_spi(void)
{
	//Configuración e inicialización para SPI
    rcc_periph_clock_enable(RCC_SPI5);  
    rcc_periph_clock_enable(RCC_GPIOC); 
    rcc_periph_clock_enable(RCC_GPIOF); 
	
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,GPIO1);
    gpio_set(GPIOC, GPIO1); // Establecer el pin GPIOC1 en alto
	gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE,GPIO7 | GPIO8 | GPIO9);   
    gpio_set_af(GPIOF, GPIO_AF5, GPIO7 | GPIO8 | GPIO9);

    // SPI5
    spi_set_master_mode(SPI5);                  
    spi_set_baudrate_prescaler(SPI5, SPI_CR1_BR_FPCLK_DIV_64); 
    spi_set_clock_polarity_0(SPI5);             
    spi_set_clock_phase_0(SPI5);                
    spi_set_full_duplex_mode(SPI5);             
    spi_set_unidirectional_mode(SPI5);          
    spi_enable_software_slave_management(SPI5); 
    spi_send_msb_first(SPI5);                   
    spi_set_nss_high(SPI5);                     
    SPI_I2SCFGR(SPI5) &= ~SPI_I2SCFGR_I2SMOD;   
    spi_enable(SPI5);                           

	//Escribir a registros
    write_reg(GYR_CTRL_REG1, GYR_CTRL_REG1_PD | GYR_CTRL_REG1_XEN | GYR_CTRL_REG1_YEN | GYR_CTRL_REG1_ZEN | (3 << GYR_CTRL_REG1_BW_SHIFT));
    write_reg(GYR_CTRL_REG4, (1 << GYR_CTRL_REG4_FS_SHIFT));
}

static void gpio_setup(void)
{
	//Configuración e inicialización de GPIOs
	/* Enable GPIOG clock. */
	rcc_periph_clock_enable(RCC_GPIOG);

	/* Set GPIO13 (in GPIO port G) to 'output push-pull'. */
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT,
			GPIO_PUPD_NONE, GPIO13);
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, 
			GPIO_PUPD_NONE, GPIO14);
}

static void button_setup(void)
{
	//Configuración e inicialización para botones
	/* Enable GPIOA clock. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Set GPIO0 (in GPIO port A) to 'input open-drain'. */
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
}

static void usart_setup(void)
{
	//Configuración e inicialización para USART
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
	//Configuración e inicialización para ADC
	rcc_periph_clock_enable(RCC_ADC1);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO3);
	adc_power_off(ADC1);
	adc_disable_scan_mode(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);
	adc_power_on(ADC1);
}

void write_reg(uint8_t reg, uint16_t value){
	//Escribir a registro, utilizado para el giroscopio.
	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, reg);
	(void) spi_read(SPI5);
	spi_send(SPI5, value);
	(void) spi_read(SPI5);
	gpio_set(GPIOC, GPIO1); 
}


static uint16_t read_adc_naiive(uint8_t channel)
{
	//Leer entrada de ADC
	uint8_t channel_array[16];
	channel_array[0] = channel;
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_start_conversion_regular(ADC1);
	while (!adc_eoc(ADC1));
	uint16_t reg16 = adc_read_regular(ADC1);
	return reg16;
}
uint8_t read_reg(uint8_t command) {
	//Leer registro, utilizado para el giroscopio.
    gpio_clear(GPIOC, GPIO1);        
    spi_send(SPI5, command);         
    spi_read(SPI5);                  
    spi_send(SPI5, 0);                 
    uint8_t result = spi_read(SPI5);   
    gpio_set(GPIOC, GPIO1);            
    return result;                     
}

void read_xyz(int16_t vecs[3])
{
	//Leer XYZ, utilizado para el giroscopio.
	read_reg(GYR_WHO_AM_I | 0x80);
	read_reg(GYR_STATUS_REG | GYR_RNW);
	
	vecs[0] = read_reg(GYR_OUT_X_L | GYR_RNW) | read_reg(GYR_OUT_X_H | GYR_RNW) << 8;
	vecs[1] = read_reg(GYR_OUT_Y_L | GYR_RNW) | read_reg(GYR_OUT_Y_H | GYR_RNW) << 8;
	vecs[2] = read_reg(GYR_OUT_Z_L | GYR_RNW) | read_reg(GYR_OUT_Z_H | GYR_RNW) << 8;

	for (int i=0; i < 4; i++ ){
		vecs[i] = vecs[i]* L3GD20_SENSITIVITY_250DPS;
	}
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
	return len; 
}

void lcd_main_structure(void){
	//Estructura principal de la pantalla
	//Se separa para no cargarlo múltiples veces
	gfx_init(lcd_draw_pixel, 240, 320);
	gfx_fillScreen(LCD_WHITE);
	gfx_setTextSize(2);
	gfx_setCursor(15, 25);
	gfx_puts("Sismografo"); //Sin tilde, no lo acepta
	gfx_setTextSize(2);
	gfx_setCursor(15, 49);
	gfx_puts("X:");
	gfx_setCursor(15, 73);
	gfx_puts("Y:");
	gfx_setCursor(15, 97);
	gfx_puts("Z:");
	gfx_setCursor(15, 121);
	gfx_puts("V:");
	gfx_setCursor(15, 145);
	gfx_puts("USART:");
}

int main(void)
{
	char volt[10]; //Voltaje de la batería
	float temp;		//Voltaje de la batería 
	uint16_t input_adc3;	//Entrada de ADC
	int16_t vecs[3];  // Almacena información de los ejes
	char char_X[10]; 	//Valor de eje X
    char char_Y[10]; 	//Valor de eje Y
    char char_Z[10]; 	//Valor de eje Z
	char output[80];	//Salida por serial

//Se inicializa el sistema
	clock_setup();     // Se inicializa el reloj
	button_setup();
	gpio_setup();
	setup_spi();       //  Se inicializa SPI5
	usart_setup();
	adc_setup();
	console_setup(115200);  // Se inicializa la consola
	sdram_init();
	lcd_spi_init();

	while (1) {
		//Detectar si se presiona botón
		if (gpio_get(GPIOA, GPIO0)) {
            usart_enabled = !usart_enabled;  // Toggle bandera
            msleep(300);  // Debounce delay
        }
		// Leer XYZ
		read_xyz(vecs);  // Leer X, Y, Z
		sprintf(char_X, "%d", vecs[0]);
        sprintf(char_Y, "%d", vecs[1]);
        sprintf(char_Z, "%d", vecs[2]);
		//Leer ADC
		input_adc3 = read_adc_naiive(3);
		temp = input_adc3* 9.0f / 4095.0f;
		sprintf(volt, "%.2f", temp);
		//Activar alarma de batería
		if (temp < 7.5) {
			gpio_toggle(GPIOG, GPIO14);
			msleep(500);
		} else {
			gpio_clear(GPIOG, GPIO14);
		}

		//Impresión en pantalla
		lcd_main_structure();
		//Comunicación serial
		if (usart_enabled) {
            gpio_toggle(GPIOG, GPIO13);  // encender y apagar el LED
			gfx_setCursor(100, 145);
			gfx_puts("Activa");
			sprintf(output, "%s,%s,%s,%s\n", char_X, char_Y, char_Z, volt);
			console_puts(output);
			msleep(500);
        }else {
            gpio_clear(GPIOG, GPIO13);  // Asegurarse de que el LED esté apagado
			gfx_setCursor(100, 145);
			gfx_puts("Inactiva");
        }

		//Impresión en pantalla
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
		msleep(200);
	}

	return 0;
}
