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

keymap = "DCBA096308520741";

void initb();
void initc();
void setn(int32_t pin_num, int32_t val);
int32_t readpin(int32_t pin_num);
void buttons(void);
void keypad(void);
void setup_tim2(void);

char* keymap_arr = &keymap;
//extern uint8_t col;

void mysleep(void) {
    for(int n = 0; n < 1000; n++);
}

int main(void) {
    // Uncomment when most things are working
    //autotest();

    initb();
    initc();

    // uncomment one of the loops, below, when ready
//     while(1) {
//       buttons();
//     }

     while(1) {
       keypad();
     }
     //setup_tim2();

	/*#define TEST_TIMER2
	#ifdef TEST_TIMER2
		setup_tim2();
		for(;;) { }
	#endif*/

    //for(;;);
}

/**
 * @brief Init GPIO port B
 *        Pin 0: input
 *        Pin 4: input
 *        Pin 8-11: output
 *
 */
void initb() {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    GPIOB->MODER &= ~0x00ff0000;
    GPIOB->MODER |= 0x00550000; // output pb11-pb8

    GPIOB->MODER &= ~0x00000303; // input pb0 and pb4
}

/**
 * @brief Init GPIO port C
 *        Pin 0-3: inputs with internal pull down resistors
 *        Pin 4-7: outputs
 *
 */
void initc() {
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;

    GPIOC->MODER &= ~0x0000ffff; //reset ports 0-7 (0-3 will remain in this state as inputs.  4-7 will be set to outputs)
    GPIOC->MODER |= 0x00005500; // output pc7-pc4

    GPIOC->PUPDR &= ~0x000000ff;
    GPIOC->PUPDR |= 0x000000AA;
}

/**
 * @brief Set GPIO port B pin to some value
 *
 * @param pin_num: Pin number in GPIO B
 * @param val    : Pin value, if 0 then the
 *                 pin is set low, else set high
 */
void setn(int32_t pin_num, int32_t val) {

    if(val != 0) {
        //GPIOB->ODR |= 0x0001 << pin_num;
    	GPIOB->ODR |= val;
    }
    else {
        GPIOB->ODR &= ~(0x1 << pin_num);
    }
}



/**
 * @brief Read GPIO port B pin values
 *
 * @param pin_num   : Pin number in GPIO B to be read
 * @return int32_t  : 1: the pin is high; 0: the pin is low
 */
int32_t readpin(int32_t pin_num) {
    int32_t output = GPIOB->IDR & (0x1 << pin_num);

    return output >> pin_num;
}

/**
 * @brief Control LEDs with buttons
 *        Use PB0 value for PB8
 *        Use PB4 value for PB9
 *
 */
void buttons(void) {
    // Use the implemented subroutines
    // Read button input at PB0
    // Put the PB0 value as output for PB8
    // Read button input at PB4
    // Put the PB4 value as output for PB9hel
    setn(8, readpin(0));
    setn(9, readpin(4));
}

/**
 * @brief Control LEDs with keypad
 *
 */
void keypad(void) {
    for(int i = 1; i <= 4; i++) {
        GPIOC->ODR &= 0x0000;
        GPIOC->ODR |= 0x0001 << (i+3);
        mysleep();
        //setn(i+7, GPIOC->IDR & 0xF);// << (i-1));
        int rows = GPIOC->IDR & 0xF;
        if(rows == 0x8) {
        	GPIOB->ODR |= keymap_arr[(((4 - i) & 0b11)*4 + 3)];
		}
		else if(rows == 0x4) {
			GPIOB->ODR |= keymap_arr[(((4 - i) & 0b11)*4 + 2)];
		}
		else if(rows == 0x2) {
			GPIOB->ODR |= keymap_arr[(((4 - i) & 0b11)*4 + 1)];
		}
		else if(rows == 0x1) {
			GPIOB->ODR |= keymap_arr[(((4 - i) & 0b11)*4 + 0)];
		}
    }



    //(4-i)
}


void setup_tim2(void) {
	//Setting up timer
	//Currently using TIM2 because pins PA1 - PA3

	RCC -> AHBENR |= RCC_AHBENR_GPIOAEN; //Initialize pins
	GPIOA -> MODER &= ~0x000000FC;
	GPIOA -> MODER |= 0x000000A8;
	GPIOA -> AFR[0] &= ~0xFF000000;
	GPIOA -> AFR[1] &= ~0x000000FF;
	//Setting the clock down
	RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2 -> PSC = 47999; //Clock is now at 1 MHz
	TIM2 -> ARR = 999; //Sets clock to 1 hertz

	TIM2 -> CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
	TIM2 -> CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
	TIM2 -> CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1;
	TIM2 -> CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1;

	TIM2 -> CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;

	// TODO: Enable TIM3 counter
	TIM2->CR1 |= TIM_CR1_CEN;

	TIM2->CCR1 = 800;
	TIM2->CCR2 = 400;
	TIM2->CCR3 = 200;
	TIM2->CCR4 = 100;

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
//char rows_to_key(int rows) {
//    if(rows == 0x8) {
//        return keymap_arr[((col & 0b11)*4 + 3)];
//    }
//    else if(rows == 0x4) {
//        return keymap_arr[((col & 0b11)*4 + 2)];
//    }
//    else if(rows == 0x2) {
//        return keymap_arr[((col & 0b11)*4 + 1)];
//    }
//    else if(rows == 0x1) {
//        return keymap_arr[((col & 0b11)*4 + 0)];
//    }
//}


