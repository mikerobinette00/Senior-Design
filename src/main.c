/**
  ******************************************************************************
  * @file    main.c
  * @author  Michael Robinette and Raymond Azar
  * @version V1.0
  * @date    2/8/2024
  * @brief   Senior Design EMT Code
  * @brief   Purdue ECE 362 Class Notes and Lab Manuals used as Resource
  ******************************************************************************
*/

#include "stm32f0xx.h"
#include <stdint.h>
#include "lcd.h"  // library provided by Niraj Menon for driving LCD display
//#include "clock.c"

void LCD_Reset(void);
void LCD_Setup();
void LCD_Clear(u16 Color);

uint8_t col = 0;

// Be sure to change this to your login...
const char login[] = "robinetm";

char keymap_arr[] = "DCBA#9630852*741";

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

//===========================================================================
// This is the 34-entry buffer to be copied into SPI1.
// Each element is a 16-bit value that is either character data or a command.
// Element 0 is the command to set the cursor to the first position of line 1.
// The next 16 elements are 16 characters.
// Element 17 is the command to set the cursor to the first position of line 2.
//===========================================================================
uint16_t display[34] = {
        0x002, // Command to set the cursor at the first position line 1
        0x200+'E', 0x200+'C', 0x200+'E', 0x200+'3', 0x200+'6', + 0x200+'2', 0x200+' ', 0x200+'i',
        0x200+'s', 0x200+' ', 0x200+'t', 0x200+'h', + 0x200+'e', 0x200+' ', 0x200+' ', 0x200+' ',
        0x0c0, // Command to set the cursor at the first position line 2
        0x200+'c', 0x200+'l', 0x200+'a', 0x200+'s', 0x200+'s', + 0x200+' ', 0x200+'f', 0x200+'o',
        0x200+'r', 0x200+' ', 0x200+'y', 0x200+'o', + 0x200+'u', 0x200+'!', 0x200+' ', 0x200+' ',
};

void init_pins();
void setn(char val);

void drive_column(int c);
int read_rows();
char rows_to_key(int rows);
//void handle_key(char key);
void setup_tim7();
void TIM7_IRQHandler();

void init_spi1_slow();
//void spi_cmd(unsigned int data);
//void spi_data(unsigned int data);
//void spi1_init_oled();
//void spi1_display1(const char *string);
//void spi1_display2(const char *string);
//void spi1_setup_dma(void);
//void spi1_enable_dma(void);

void mysleep(void) {
    for(int n = 0; n < 1000; n++);
}

int main(void) {
    init_pins();
    setup_tim7();
    init_spi1_slow();
    LCD_Setup();  // function from lcd.c

    LCD_Reset(); // function from lcd.c
    LCD_Clear(RED); // function from lcd.c setting display to cyan

    //LCD_DrawLine(0,0,200,200, GREEN);



    for(;;);
    //	LCD_Clear(0x7FFF); // function from lcd.c setting display to cyan
    //}

}

/**
 * @brief Init GPIO port B
 *        Pin 0: input
 *        Pin 4: input
 *        Pin 8-11: output
 * @brief Init GPIO port C
 *        Pin 0-3: inputs with internal pull down resistors
 *        Pin 4-7: outputs
 *
 */
void init_pins() {

    // hijacked these pins for spi display due to instructions from niraj
    //GPIOB->MODER &= ~0x00ff0000;
    //GPIOB->MODER |= 0x00550000; // output pb11-pb8

    //GPIOB->MODER &= ~0x00000303; // input pb0 and pb4

	// PB8 SPI1_NSS (CS pin)
	// PB11 nRESET
	// PB14 DC
    // set pins pb8,11,14 as GPIO outputs
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
//    GPIOB->MODER &= ~0x30C30000;
//    GPIOC->MODER |= 0x10410000;

    // removing pb8 as generic output
    GPIOB->MODER &= ~0x30C00000;
	GPIOC->MODER |= 0x10400000;

    // initialize nss to high
    //GPIOB->ODR &= ~0x00000100;
    //GPIOB->ODR |= 0x00000100;

    // settings GPIOC pins for keypad
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC->MODER &= 0xffff0000; //reset ports C0-C7 (0-3 will remain in this state as inputs.  4-7 will be set to outputs)
    GPIOC->MODER |= 0x00005500; // output pc7-pc4

    GPIOC->PUPDR &= ~0x000000ff;
    GPIOC->PUPDR |= 0x000000AA;
}


/**
 * @brief Set GPIO port B pin to some value
 *
 * @param val    : Pin value, if 0 then the
 *                 pin is set low, else set high
 */
void setn(char val) {

    if(val != 0) {
    	GPIOB->ODR |= val << 8;
    }
    else {
        GPIOB->ODR &= ~(0xF << 8);
    }
}


/**
 * @brief Drive the column pins of the keypad
 *        First clear the keypad column output
 *        Then drive the column represented by `c`
 *
 * @param c
 */
void drive_column(int c) {
//    GPIOC->BRR = 0x000000F0;
//    GPIOC->BSRR |= 0x0010 << (c & 0b11);
	GPIOC->ODR &= 0x0000;
	GPIOC->ODR |= 0x0001 << (c+4);
}

/**
 * @brief Read the rows value of the keypad
 *
 * @return int
 */
int read_rows() {
    return GPIOC->IDR & 0xF;
}

/**
 * @brief Convert the pressed key to character
 *        Use the rows value and the current `col`
 *        being scanning to compute an offset into
 *        the character map array
 *
 * @param rows
 * @return char
 */
char rows_to_key(int rows) {
    if(rows == 0x8) {
        return keymap_arr[((col & 0b11)*4 + 3)];
    }
    else if(rows == 0x4) {
        return keymap_arr[((col & 0b11)*4 + 2)];
    }
    else if(rows == 0x2) {
        return keymap_arr[((col & 0b11)*4 + 1)];
    }
    else if(rows == 0x1) {
        return keymap_arr[((col & 0b11)*4 + 0)];
    }
    return '0';
}

/**
 * @brief Setup timer 7 as described in lab handout
 *
 */
void setup_tim7() {
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->PSC = 4799;
    TIM7->ARR = 9;
    TIM7->DIER |= TIM_DIER_UIE;
    TIM7->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 0xffffffff;
}

//-------------------------------
// Timer 7 ISR goes here
//-------------------------------
// TODO

void TIM7_IRQHandler(){
    TIM7->SR = ~TIM_SR_UIF;
    int rows = read_rows();
    char key = 0x00;
    if(rows != 0) {
        key = rows_to_key(rows);
    }
    setn(key);
    col++;
    if(col >= 5) {
        col = 0;
    }
    drive_column(col);
}



//===========================================================================
// 4.4 SPI OLED Display
//===========================================================================
void init_spi1_slow() {
    // PB3 SPI1_SCK
    // PB5 SPI1_MOSI
    // PB8 Generic output//SPI2_NSS (CS pin)
	// PB11 Generic output//nRESET
	// PB14 Generic output//DC

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // PB3, 5 as alternate functions (PB3: SCK) (PB5: MOSI)
    GPIOB->MODER &= ~0x00000CC0;
    GPIOB->MODER |= 0x00000880;

    // initialize sck to low
    //GPIOB->ODR &= ~0x00000008;

    // set AFR for pins PB3,5: (PB3:SCK:AF0) (PB5:MOSI:AF0)
    GPIOB->AFR[0] &= ~0x00F0F000;

    // disable spi channel
    SPI1->CR1 &= ~SPI_CR1_SPE;

    // set baud rate divisor to max value to make baud rate as low as possible?
    // set baud bits to 000
    SPI1->CR1 &= ~(SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2);

    // set master mode
    SPI1->CR1 |= SPI_CR1_MSTR;

    // set word (data) size to 8-bit
    // set data bits to 0111
    SPI1->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0;
    SPI1->CR2 &= ~(SPI_CR2_DS_3);

    // setting up output enable (SSOE) and setting up enable bit and enabling NSSP (NSSP)
    SPI1->CR2 |= SPI_CR2_SSOE | SPI_CR2_NSSP;

    // configure "software slave management" and "internal slave select"
    SPI1->CR1 |= (SPI_CR1_SSM | SPI_CR1_SSI);

    // set the "FIFO reception threshold" bit in CR2 so that the SPI channel immediately releases a received 8-bit value
    SPI1->CR2 |= SPI_CR2_FRXTH;

    // enable SPI channel
    SPI1->CR1 |= SPI_CR1_SPE;  // enable spe
}

//void spi_cmd(unsigned int data) {
//    while(!(SPI1->SR & SPI_SR_TXE)) {}
//    SPI1->DR = data;
//}
//void spi_data(unsigned int data) {
//    spi_cmd(data | 0x200);
//}
//void spi1_init_oled() {
//    nano_wait(1000000);
//    spi_cmd(0x38);
//    spi_cmd(0x08);
//    spi_cmd(0x01);
//    nano_wait(2000000);
//    spi_cmd(0x06);
//    spi_cmd(0x02);
//    spi_cmd(0x0c);
//}
//void spi1_display1(const char *string) {
//    spi_cmd(0x02);
//    while(*string != '\0') {
//        spi_data(*string);
//        string++;
//    }
//}
//void spi1_display2(const char *string) {
//    spi_cmd(0xc0);
//    while(*string != '\0') {
//        spi_data(*string);
//        string++;
//    }
//}
//
//
//
////===========================================================================
//// Configure the proper DMA channel to be triggered by SPI1_TX.
//// Set the SPI1 peripheral to trigger a DMA when the transmitter is empty.
////===========================================================================
//void spi1_setup_dma(void) {
//    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
//    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
//    DMA1_Channel3->CPAR = (uint32_t) (&(SPI1->DR));
//    DMA1_Channel3->CMAR = (uint32_t) display;///////////////////////// address?
//    DMA1_Channel3->CNDTR = 34;
//    DMA1_Channel3->CCR |= DMA_CCR_DIR;
//    DMA1_Channel3->CCR |= DMA_CCR_MINC;
//    DMA1_Channel3->CCR &= ~DMA_CCR_PINC;
//    DMA1_Channel3->CCR &= ~0x00000F00;  // clear Msize and psize
//    DMA1_Channel3->CCR |= 0x00000500;  // 16 bits on msize and psize (0101)
//    DMA1_Channel3->CCR |= DMA_CCR_CIRC;
//
//}
//
////===========================================================================
//// Enable the DMA channel triggered by SPI1_TX.
////===========================================================================
//void spi1_enable_dma(void) {
//    DMA1_Channel3->CCR |= DMA_CCR_EN;
//}


void tim2_PWM(void) {
	RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
	//Pinout layout: PA1-PA3
	GPIOA -> MODER &= ~0x000000FC;
	GPIOA -> MODER |= 0x000000A8;

	//Come back to the AFR and what it does
	GPIOA -> AFR[0] &= ~0x0000FFF0;
	GPIOA -> AFR[0] |= 0x00002220;

	//Scaling the timer; Currently set to 1 Hz
	RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2 -> PSC = 47999;
	TIM2 -> ARR = 999;

	TIM2 -> CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
	TIM2 -> CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1;
	TIM2 -> CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1;

	//Enable Output
	TIM2 -> CCER |= TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;

	//Enable TIM2 Counter
	TIM2 -> CR1 |= TIM_CR1_CEN;

	//Determines the duty cycle
	TIM2 -> CCR2 = 400;
	TIM2 -> CCR3 = 200;
	TIM2 -> CCR4 = 100;
}

