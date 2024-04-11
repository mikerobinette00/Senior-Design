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

// various code snippets consulted and used from ece 362 lab work


#include "stm32f0xx.h"
#include <stdint.h>
#include <stdio.h>
#include "lcd.h"  // library provided by Niraj Menon for driving LCD display

void LCD_Setup();
void LCD_Clear(u16 Color);
void LCD_DrawChar(u16 x,u16 y,u16 fc, u16 bc, char num, u8 size, u8 mode);
void LCD_DrawString(u16 x,u16 y, u16 fc, u16 bg, const char *p, u8 size, u8 mode);
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 c);

uint8_t num_char = 0;
uint8_t col;

/* display block begin */
const int pixel_col = 240;
const int pixel_row = 320;

const int font_size = 16;  // currently only sizes 12 and 16 supported
const int num_digits = 5;

uint16_t cursor_pos_col = 0;
uint16_t cursor_pos_row = 0;

const int num_table_rows = 3;
const int num_table_cols = 4;

uint8_t row_inc = 0;
uint8_t col_inc = 0;

char pressed_key;
char keypresses[5] = {"00000"};

char *data_fields[] = {
		"Rated Motor Voltage", "", "", "00.00",
		"Rated Motor Max RPM", "", "", "00000",
		"Measured Motor RPM", "", "", "00000"
};

int motor_des_voltage = 0;
int motor_des_speed = 0;

int updated_value = 0;

int live_speed_reading = 0;

void init_display_fields(char *data_fields_arr[]);
void update_display_field(char *updated_string);
void draw_cursor();
void erase_cursor();
void process_keyPress(char key);
/* display block end */

void init_pins();
void drive_column(int c);
int read_rows();


/* debouncing functions begin */
void update_history(int c, int rows);
char get_keypress(void);
/* debouncing functions end */

/*
 * rays functions for pwm and dma
 */
void tim2_PWM(void);
void spi1_setup_dma(void);
void spi1_enable_dma(void);
/*
 * end of rays functions
 */

/* interrupts begin */
void init_spi1();


void setup_adc(void);
void TIM3_IRQHandler();
void init_tim3(void);

void setup_tim7();
void TIM7_IRQHandler();

void SysTick_Handler();
void init_systick();
void EXTI4_15_IRQHandler();
void init_exti();

/* interrupts end */

/*
 * placeholder global variable for dma
 */
int display[32];




void mysleep(void) {
    for(int n = 0; n < 1000; n++);
}

int main(void) {
	row_inc = pixel_row / num_table_rows;
	col_inc = pixel_col / num_table_cols;



    init_pins();
    setup_tim7();
    init_spi1();

    setup_adc();
    init_tim3();
    init_systick();
    init_exti();

    /*
     * rays functions
     */
    tim2_PWM();
    spi1_setup_dma();
    spi1_enable_dma();
    /*
     * end of rays functions
     */


    LCD_Setup();  // function from lcd.c
    LCD_Clear(0xFFFF);

    init_display_fields(data_fields);

    for(;;)
    {
    	pressed_key = get_keypress();
    	process_keyPress(pressed_key);
    	//process_keyPress(pressed_key);
    }
}


void init_display_fields(char *data_fields_arr[]) {
	/* requirements
	 * divide display into table
	 * input correct fields into spots of table
	 *
	 */

	for(int idx_row = 0; idx_row < num_table_rows; idx_row++) {
		for(int idx_col = 0; idx_col < num_table_cols; idx_col++) {
			cursor_pos_row = idx_row * row_inc;
			cursor_pos_col = idx_col * col_inc;
			update_display_field(data_fields_arr[num_table_cols * idx_row + idx_col]);
		}
	}
	cursor_pos_col = col_inc * (num_table_cols - 1);
	cursor_pos_row = 0;
}

/*
 * take in value and update only the necessary section of the display
 *
 * updates field at cursor position
 */
void update_display_field(char *updated_string) {
	LCD_DrawString(cursor_pos_col, cursor_pos_row, BLACK, WHITE, updated_string, font_size, 0);
}

void draw_cursor() {
	LCD_DrawLine(cursor_pos_col, cursor_pos_row + font_size + 1, cursor_pos_col + font_size / 2, cursor_pos_row + font_size + 1, BLACK);
}

void erase_cursor() {
	LCD_DrawLine(cursor_pos_col, cursor_pos_row + font_size + 1, cursor_pos_col + font_size / 2, cursor_pos_row + font_size + 1, WHITE);
}



void process_keyPress(char key) {
	// allow user to enter up to 4 digits 00.00 (0-24V)
	// when user presses hashtag key, enter the number into the system
	/* PARAMS NEEDED:
	 * voltage
	 * speed
	 */
	/* A-D effects for now:
	 * A: left arrow
	 * B: right arrow
	 * C: start motor
	 * D: stop motor
	 */

//	NVIC_DisableIRQ(TIM2_IRQn);
//	NVIC_DisableIRQ(TIM7_IRQn);
//	NVIC_DisableIRQ(SysTick_IRQn);

	uint8_t far_left_pos = col_inc * (num_table_cols - 1);
	uint8_t far_right_pos = far_left_pos + (font_size / 2) * (num_digits - 1);

	uint8_t top_field_pos = 0;
	uint8_t bottom_field_pos = row_inc * (num_table_rows - 2);

	void process_num() {
		keypresses[(cursor_pos_col - far_left_pos) / (font_size / 2)] = key;
		LCD_DrawChar(cursor_pos_col, cursor_pos_row, WHITE, BLACK, key, font_size, 0);
		if(cursor_pos_col < far_right_pos) {
			erase_cursor();
			cursor_pos_col += font_size / 2;
			draw_cursor();
		}
	}

	switch(key) {
	case 'A':  // up arrow
		erase_cursor();
		if(cursor_pos_row > top_field_pos) {
			cursor_pos_row -= row_inc;
		}
		draw_cursor();
		break;
	case 'B':  // down arrow
		erase_cursor();
		if(cursor_pos_row < bottom_field_pos) {
			cursor_pos_row += row_inc;
		}
		draw_cursor();
		break;
	case 'C':  // left arrow
		erase_cursor();
		if(cursor_pos_col > far_left_pos) {
			cursor_pos_col -= font_size / 2;
		}
		draw_cursor();
		break;
	case 'D':  // right arrow
		erase_cursor();
		if(cursor_pos_col < far_right_pos) {
			cursor_pos_col += font_size / 2;
		}
		draw_cursor();
		break;
	case '#':  // enter value
		erase_cursor();
		cursor_pos_col = far_left_pos;
		draw_cursor();
		update_display_field(keypresses);

		int input_value = 0;
		int starting_power = 1;
		for(int i = 0; i < num_digits; i++) {
			input_value += (keypresses[i] - '0') * (10000 / starting_power);
			starting_power *= 10;
		}
		if((cursor_pos_row / row_inc) == 0) {
			motor_des_voltage = input_value;
		}
		else if((cursor_pos_row / row_inc) == 1) {
			motor_des_speed = input_value;
		}

		for(int i = 0; i < num_digits; i++) {
			keypresses[i] = '0';
		}
		break;
	case '0':
		process_num();
		break;
	case '1':
		process_num();
		break;
	case '2':
		process_num();
		break;
	case '3':
		process_num();
		break;
	case '4':
		process_num();
		break;
	case '5':
		process_num();
		break;
	case '6':
		process_num();
		break;
	case '7':
		process_num();
		break;
	case '8':
		process_num();
		break;
	case '9':  // record key presses
		process_num();
		break;
	default:
		break;

	}

//	NVIC_EnableIRQ(TIM2_IRQn);
//	NVIC_EnableIRQ(TIM7_IRQn);
//	NVIC_EnableIRQ(SysTick_IRQn);
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
	// PB8 SPI1_NSS (CS pin)
	// PB6 nRESET
	// PB7 DC
    // set pins pb8,6,7 as GPIO outputs
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
//    GPIOB->MODER &= ~0x30C30000;
//    GPIOB->MODER |= 0x10410000;
    GPIOB->MODER &= ~0x0003F000;
    GPIOB->MODER |= 0x00015000;

    // settings GPIOC pins for keypad
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC->MODER &= ~0x000fffff; //reset ports C0-C9 (0-3 and 8-9 will remain in this state as inputs.  4-7 will be set to outputs)
    GPIOC->MODER |= 0x00005500; // output open drain pc7-pc4

    GPIOC->OTYPER |= 0x000000F0; // open drain PC4-7 (for keypad debouncing)

    // pull up resisters PC0-3
    GPIOC->PUPDR &= ~0x000000ff;
    GPIOC->PUPDR |= 0x00000055;
}


//===========================================================================
// 4.4 SPI OLED Display
//===========================================================================
void init_spi1() {
    // PB3 SPI1_SCK
    // PB5 SPI1_MOSI
    // PB8 Generic output//CS NSS
	// PB6 Generic output//nRESET
	// PB7 Generic output//DC

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
    SPI1->CR1 |= SPI_CR1_BR_0;

    // set master mode
    SPI1->CR1 |= SPI_CR1_MSTR;

    // set word (data) size to 8-bit
    // set data bits to 0111
    SPI1->CR2 |= SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0;
    SPI1->CR2 &= ~(SPI_CR2_DS_3);

    // setting up output enable (SSOE) and setting up enable bit and enabling NSSP (NSSP)
    //SPI1->CR2 |= SPI_CR2_SSOE | SPI_CR2_NSSP;

    // configure "software slave management" and "internal slave select"
    SPI1->CR1 |= (SPI_CR1_SSM | SPI_CR1_SSI);

    // set the "FIFO reception threshold" bit in CR2 so that the SPI channel immediately releases a received 8-bit value
    SPI1->CR2 |= SPI_CR2_FRXTH;

    // enable SPI channel
    SPI1->CR1 |= SPI_CR1_SPE;  // enable spe
}



//============================================================================
// ADC conversion
//============================================================================
//=============================================================================
// Part 3: Analog-to-digital conversion for a volume level.
//=============================================================================
//uint32_t volume = 2400;


//============================================================================
// setup_adc()
//============================================================================
void setup_adc(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= 0x300;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CR2 |= RCC_CR2_HSI14ON;
    while(!(RCC->CR2 & RCC_CR2_HSI14RDY));
    ADC1->CR |= ADC_CR_ADEN;
    while(!(ADC1->ISR & ADC_ISR_ADRDY));

    ADC1->CHSELR = 0;
    ADC1->CHSELR |= 1 << 4;
}


// hz to rpm = hz * 60
/*
 * flip flop variable
 * when signal goes high -> set flipflop to 1
 * when signal goes low -> set flipflop to 0
 * have a counter that starts from 0 when flip flop goes high and ends when flipflop goes high again
 * each counter increment is 1/1000 s
 */
//============================================================================
// init_tim3()
//============================================================================
void init_tim3(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
//    // 10 hz sampling rate
//    TIM2->PSC = 47999;
//    TIM2->ARR = 99;
    // 1000 hz sampling rate
	TIM3->PSC = 4799;
	TIM3->ARR = 9;
    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] |= 0xffffffff;
}

//============================================================================
// Variables for boxcar averaging.
//============================================================================
#define BCSIZE 32
float bcsum = 0;
float boxcar[BCSIZE];
int bcn = 0;
int flipflop = 0;
float speed_counter = 0;


//============================================================================
// Timer 3 ISR
//============================================================================
void TIM3_IRQHandler(){
    TIM3->SR = ~TIM_SR_UIF;
    ADC1->CR |= ADC_CR_ADSTART;
    while(!(ADC1->ISR & ADC_ISR_EOC));



//    bcsum -= boxcar[bcn];
//    bcsum += boxcar[bcn] = ADC1->DR;
//    bcn += 1;
//    if (bcn >= BCSIZE)
//        bcn = 0;
    //live_speed_reading = bcsum / BCSIZE; // 2.95 * live_speed_reading / 4096 -> conversion to voltage

    float live_speed_reading_voltage = 2.95 * (ADC1->DR) / 4096;
    speed_counter += 1;
    if(live_speed_reading_voltage > 0.3) {
    	if(flipflop == 0) {
    		// do speed calc

//			bcsum -= boxcar[bcn];
//			bcsum += boxcar[bcn] = (1.0 / (speed_counter/1000.0)) * 60.0;
//			bcn += 1;
//			if (bcn >= BCSIZE) {
//				live_speed_reading = bcsum / BCSIZE;
//				bcn = 0;
//			}

    		live_speed_reading = (1.0 / (speed_counter/1000.0)) * 60.0;


    		speed_counter = 0;
    	}
    	flipflop = 1;
    }
    else {
    	flipflop = 0;
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
    DMA1_Channel3->CMAR = (uint32_t) &display;///////////////////////// address?
    DMA1_Channel3->CNDTR = 34;
    DMA1_Channel3->CCR |= DMA_CCR_DIR;
    DMA1_Channel3->CCR |= DMA_CCR_MINC;
    DMA1_Channel3->CCR &= ~DMA_CCR_PINC;
    DMA1_Channel3->CCR &= ~0x00000F00;  // clear Msize and psize
    DMA1_Channel3->CCR |= 0x00000500;  // 16 bits on msize and psize (0101)
    DMA1_Channel3->CCR |= DMA_CCR_CIRC;

}


/*
 * TIM2 PWM and DMA
 */

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
    //TIM2 -> CCER |= TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;

    //Enable TIM2 Counter
    TIM2 -> CR1 |= TIM_CR1_CEN;

    //Determines the duty cycle (Reloads at 9; CCR2 at 5 = ~50%)
    TIM2 -> CCR2 = 5;
    TIM2 -> CCR3 = 2.5;
    TIM2 -> CCR4 = 100;
}



/*
 * INTERRUPTS EXIST BELOW THIS LINE
 */


/**
 * @brief Setup timer 7
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
// Timer 7 ISR
//-------------------------------

void TIM7_IRQHandler(){
    TIM7->SR = ~TIM_SR_UIF;
    // Remember to acknowledge the interrupt here!
    int rows = read_rows();
    update_history(col, rows);
    col = (col + 1) & 3;
    drive_column(col);
}



/**
 * @brief The ISR for the SysTick interrupt.
 *
 */
void SysTick_Handler() {
	char buffer[5];
	// motor voltage
	sprintf(buffer, "%d", motor_des_voltage);
	LCD_DrawString(0, 320-16*3, BLACK, WHITE, buffer, font_size, 0);
	// motor speed
	sprintf(buffer, "%d", motor_des_speed);
	LCD_DrawString(0, 320-16*2, BLACK, WHITE, buffer, font_size, 0);
	// adc reading for "motor speed"
	sprintf(buffer, "%3d", live_speed_reading);

	LCD_DrawString(0, 320-16*1, BLACK, WHITE, buffer, font_size, 0);
}

/**
 * @brief Enable the SysTick interrupt to occur every 1/16 seconds.
 *
 */
void init_systick() {
    SysTick->LOAD = 0x0005B8D7;
    SysTick->CTRL &= ~0x00000004;
    SysTick->CTRL |= 0x00000003;
}



// EXTI Interrupt handler for pins 4-15
// acknowledge interrupt on pins 8,9
void EXTI4_15_IRQHandler() {
    EXTI->PR |= EXTI_PR_PR8 | EXTI_PR_PR9;

    if(GPIOC->IDR & (0x1 << 8)) {  // start motor
        LCD_DrawString(0, 320-16*4, BLACK, WHITE, "MOTOR RUNNING", font_size, 0);
        //TIM2 -> CCER |= TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;  // start pwm signal coming out
    }
    else if(GPIOC->IDR & (0x1 << 9)) {  // stop motor
        LCD_DrawString(0, 320-16*4, BLACK, WHITE, "MOTOR STOPPED", font_size, 0);
        //TIM2 -> CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E);  // stop pwm signal coming out
    }
}

void init_exti() {
    // enable the clock
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;

    // enable the exti interrupt for pins 8-9
    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR3_EXTI8_PC | SYSCFG_EXTICR3_EXTI9_PC;
    EXTI->RTSR |= EXTI_RTSR_TR8 | EXTI_RTSR_TR9;
    EXTI->IMR |= EXTI_IMR_MR8 | EXTI_IMR_MR9;
    NVIC->ISER[0] |= 0x000000E0;
}
