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
void tim2_PWM(void);
void tim17_DMA(void);

void init_spi1();
void spi_cmd(unsigned int data);
void spi_data(unsigned int data);
void spi1_init_oled();
void spi1_display1(const char *string);
void spi1_display2(const char *string);
void spi1_setup_dma(void);
void spi1_enable_dma(void);

uint16_t ADC_array[32] = { 0x0000,0x0100,0x0200,0x0300,0x0400,0x0500,0x0600,0x0700};

void mysleep(void) {
    for(int n = 0; n < 1000; n++);
}

int main(void) {
    init_pins();
    setup_tim7();

    // SPI OLED direct drive
    #define SPI_OLED
    #if defined(SPI_OLED)
    init_spi1();
    spi1_init_oled();
    spi1_display1("Hello again,");
    spi1_display2(login);

    #endif

    // SPI
    //#define SPI_OLED_DMA
    #if defined(SPI_OLED_DMA)
    init_spi1();
    spi1_init_oled();
    spi1_setup_dma();
    spi1_enable_dma();
    #endif

	#define TIM2_PWM
	#if defined(TIM2_PWM)
	tim2_PWM();
	#endif

    for(;;);


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
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    GPIOB->MODER &= ~0x00ff0000;
    GPIOB->MODER |= 0x00550000; // output pb11-pb8

    GPIOB->MODER &= ~0x00000303; // input pb0 and pb4

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
    setn(8);
    col++;
    if(col >= 5) {
        col = 0;
    }
    drive_column(col);
}



//===========================================================================
// 4.4 SPI OLED Display
//===========================================================================
void init_spi1() {
    // PA5  SPI1_SCK
    // PA6  SPI1_MISO
    // PA7  SPI1_MOSI
    // PA15 SPI1_NSS (CS pin)

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    GPIOA->MODER &= ~0xC000FC00;
    GPIOA->MODER |= 0x8000A800;

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    GPIOA->AFR[0] &= ~0xFFF00000;
    GPIOA->AFR[1] &= ~0xF0000000;

    SPI1->CR1 &= ~SPI_CR1_SPE;  // disable spe
    SPI1->CR1 |= SPI_CR1_BR;

    SPI1->CR2 = SPI_CR2_DS_3 | SPI_CR2_DS_0;

    SPI1->CR1 |= SPI_CR1_MSTR;
    SPI1->CR2 |= SPI_CR2_SSOE | SPI_CR2_NSSP;
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    SPI1->CR1 |= SPI_CR1_SPE;  // enable spe

}

void spi_cmd(unsigned int data) {
    while(!(SPI1->SR & SPI_SR_TXE)) {}
    SPI1->DR = data;
}
void spi_data(unsigned int data) {
    spi_cmd(data | 0x200);
}
void spi1_init_oled() {
    nano_wait(1000000);
    spi_cmd(0x38);
    spi_cmd(0x08);
    spi_cmd(0x01);
    nano_wait(2000000);
    spi_cmd(0x06);
    spi_cmd(0x02);
    spi_cmd(0x0c);
}
void spi1_display1(const char *string) {
    spi_cmd(0x02);
    while(*string != '\0') {
        spi_data(*string);
        string++;
    }
}
void spi1_display2(const char *string) {
    spi_cmd(0xc0);
    while(*string != '\0') {
        spi_data(*string);
        string++;
    }
}



//===========================================================================
// Configure the proper DMA channel to be triggered by SPI1_TX.
// Set the SPI1 peripheral to trigger a DMA when the transmitter is empty.
//===========================================================================
void spi1_setup_dma(void) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3->CPAR = (uint32_t) (&(SPI1->DR));
    DMA1_Channel3->CMAR = (uint32_t) display;///////////////////////// address?
    DMA1_Channel3->CNDTR = 34;
    DMA1_Channel3->CCR |= DMA_CCR_DIR;
    DMA1_Channel3->CCR |= DMA_CCR_MINC;
    DMA1_Channel3->CCR &= ~DMA_CCR_PINC;
    DMA1_Channel3->CCR &= ~0x00000F00;  // clear Msize and psize
    DMA1_Channel3->CCR |= 0x00000500;  // 16 bits on msize and psize (0101)
    DMA1_Channel3->CCR |= DMA_CCR_CIRC;

}

//===========================================================================
// Enable the DMA channel triggered by SPI1_TX.
//===========================================================================
void spi1_enable_dma(void) {
    DMA1_Channel3->CCR |= DMA_CCR_EN;
}



void tim2_PWM(void) {
	RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
	//Pinout layout: PA1-PA3
	GPIOA -> MODER &= ~0x000000FC;
	GPIOA -> MODER |= 0x000000A8;

	//Come back to the AFR and what it does
	GPIOA -> AFR[0] &= ~0x0000FFF0;
	GPIOA -> AFR[0] |= 0x00002220;

	//Scaling the timer; Currently set to 100 kHz
	RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2 -> PSC = 47;
	TIM2 -> ARR = 9;

	TIM2 -> CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
	TIM2 -> CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1;
	TIM2 -> CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1;

	//Enable Output
	TIM2 -> CCER |= TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;

	//Enable TIM2 Counter
	TIM2 -> CR1 |= TIM_CR1_CEN;

	//Determines the duty cycle (Reloads at 9; CCR2 at 5 = ~50%)
	TIM2 -> CCR2 = 5;
	TIM2 -> CCR3 = 2.5;
	TIM2 -> CCR4 = 100;
}

//Timer for DMA currently set to 100kHz
void tim17_DMA(void) {
	RCC -> APB2ENR |= RCC_APB2ENR_TIM17EN;
	TIM17 -> PSC = 47;
	TIM17 -> ARR = 9;
	TIM17 -> DIER |= TIM_DIER_UDE; //UDE triggers DMA requests UIE triggers interrupts
	TIM17 -> CR1 |= TIM_CR1_CEN;
}

void dma_mem_to_perhipheral(void) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1 -> CCR &= ~DMA_CCR_EN; //Disabling for edits
    ADC1 -> CFGR1 |= ADC_CFGR1_DMAEN;
    DMA1_Channel1 -> CPAR = (uint32_t) (&(ADC1->DR)); //Address of the peripheral register
    DMA1_Channel1 -> CMAR = (uint32_t) (ADC_array); //Address of memory register
    DMA1_Channel1 -> CNDTR = 32; //Size of the array being stored
    DMA1_Channel1 -> CCR |= DMA_CCR_DIR; //Copy from memory to peripheral
    DMA1_Channel1 -> CCR |= DMA_CCR_MINC; //Incrementing every transfer
    DMA1_Channel1 -> CCR &= ~DMA_CCR_PINC; //Only used for memory to memory
    DMA1_Channel1 -> CCR &= ~0x00000F00;  // clear Msize and psize
    DMA1_Channel1 -> CCR |= 0x00000A00;  // 32 bits on msize and psize (1010)
    DMA1_Channel1 -> CCR |= DMA_CCR_CIRC; //Enabling circular mode
    DMA1_Channel1 -> CCR |= DMA_CCR_EN; //Enabling the DMA
}

//Timer for DMA
void init_tim17(void) {
    RCC -> APB2ENR |= RCC_APB2ENR_TIM15EN;
    TIM17 -> PSC = 4799;
    TIM17 -> ARR = 9;
    TIM17 -> DIER |= TIM_DIER_UDE;
    TIM17 -> CR1 |= TIM_CR1_CEN;
}

