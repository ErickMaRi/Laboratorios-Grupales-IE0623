#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "clock.h"
#include "console.h"

void setup_spi(void);
uint16_t read_reg(int reg);
int print_decimal(int num);
uint8_t read_xyz(int16_t vecs[3]);
void display_xyz(int16_t vecs[3]);

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

int print_decimal(int num)
{
	int		ndx = 0; //indice
	char	buf[10];
	int		len = 0;
	char	is_signed = 0; //Se usa para indicar negativos

	if (num < 0) {
        //Si el número es negativo  se indica en la bandera y se pasa a positivo
		is_signed++;
		num = 0 - num;
	}
	buf[ndx++] = '\000';
	do {
        //Convertir en caracteres
		buf[ndx++] = (num % 10) + '0';
		num = num / 10;
	} while (num != 0);
	ndx--;
	if (is_signed != 0) {
		//Imprimir negativo de ser necesario
		console_putc('-');
		len++;
	}
	while (buf[ndx] != '\000') {
        //Imprimir dígitos
		console_putc(buf[ndx--]);
		len++;
	}
	return len; /* number of characters printed */
}

uint16_t read_reg(int reg){
    uint16_t d1, d2;

    gpio_clear(GPIOC, GPIO1);  // Se pone  en bajo el pin CS
    d1 = 0x80 | (reg & 0x3f); // Mascara
    spi_send(SPI5, d1);        // Se indica cual registro se quiere leer
    d2 = spi_read(SPI5);       // Se lee primera parte
    d2 <<= 8;
    
    spi_send(SPI5, 0);         // Se envia un 0
    d2 |= spi_read(SPI5);      // Se lee segunda parte
    gpio_set(GPIOC, GPIO1);    // Se pone en alto el pin CS
    return d2;
}

uint8_t read_xyz(int16_t vecs[3]) {
    uint8_t buf[6];  // Buffer 
    int i;

    gpio_clear(GPIOC, GPIO1);  // Se pone  en bajo el pin CS
    spi_send(SPI5, 0xc0 | 0x28);  // Leer desde eje x
    (void) spi_read(SPI5);

    for (i = 0; i < 6; i++) {
        spi_send(SPI5, 0);  // Se envia un byte para recibir información
        buf[i] = spi_read(SPI5);  // Leer datos
    }
    gpio_set(GPIOC, GPIO1);  // Se pone en alto el pin CS

    // Pasar de 8 a 16 bits
    vecs[0] = (buf[1] << 8 | buf[0]);  // X
    vecs[1] = (buf[3] << 8 | buf[2]);  // Y
    vecs[2] = (buf[5] << 8 | buf[4]);  // Z
    return read_reg(0x27);  
}

void display_xyz(int16_t vecs[3]){
    console_puts("X: ");
    print_decimal(vecs[0]);
    console_puts(", Y: ");
    print_decimal(vecs[1]);
    console_puts(", Z: ");
    print_decimal(vecs[2]);
    console_puts("\n");
}

int main(void) {
    int16_t vecs[3];  // Almacena información de los ejes

    clock_setup();     // Se inicializa el reloj
    console_setup(115200);  // Se inicializa la consola
    setup_spi();       //  Se inicializa SPI5

    console_puts("Reading Gyroscope values...\n");

    while (1) {
        read_xyz(vecs);  // Leer X, Y, Z
        display_xyz(vecs); //Mostrar X, Y, Z en consola
        msleep(1000);  // Esperar antes de correr nuevamente
    }
}