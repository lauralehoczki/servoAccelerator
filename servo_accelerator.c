/*
	Embedded Systems Final Project

	Bluetooth phone's accelerometer data to Discovery Board. Movement activates servo 
	motor. When speed is 0,	servo motor is also stationary.
	
	1. Create Android app that reads accelerometer's data
	2. Set up bluetooth connection between Discovery Board and phone
	3. Send app's accelerometer data to board
	4. Continuously read incoming signal
	5. If signal is 0, make servo stationary. If there is movement, rotate servo at a
		steady speed.
*/

#include "stm32f7xx_hal.h"              // Keil::Device:STM32Cube HAL:Common
#include "stm32f7xx.h"                  // Device header
#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include <stdio.h>
#include <stdlib.h>

/* USART Driver */
extern ARM_DRIVER_USART Driver_USART7;

int main(void)
{
	//Accelerometer outputs X, Y and Z acceleration
	//The ServoWalker app sends Z acceleration (1 byte signed number)
	signed char data[4];
	
	//Activate GPIOF, GPIOH
	RCC->AHB1ENR |= (1 << 5) | (1 << 7);
	
	//Activate TIM12 for Continuous Rotation Servo
	RCC->APB1ENR |= (1 << 6);

	//AF mode
	GPIOH->MODER |= (2 << 12);

	//AF9 for TIM12
	GPIOH->AFR[0] |= (9 << 24);

	TIM12->CCMR1 |= (6 << 4);
 
	//Connect to output
	TIM12->CCER |= (1 << 0);

	//Set prescaler and ARR values so servo is stationary by default
	//Clock frequency is 16MHz because LCD is not used
	TIM12->PSC = 5;
	TIM12->ARR = 57331;
	//I calculated CCR from the ~7% duty cycle
	TIM12->CCR1 = 4082;

	//Start counters
	TIM12->CR1 |= (1 << 0);
	
	Driver_USART7.Initialize(NULL);
	
    /*Power up the USART peripheral */
    Driver_USART7.PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 4800 Bits/sec */
    Driver_USART7.Control(ARM_USART_MODE_ASYNCHRONOUS |
                      ARM_USART_DATA_BITS_8 |
                      ARM_USART_PARITY_NONE |
                      ARM_USART_STOP_BITS_1 |
                      ARM_USART_FLOW_CONTROL_NONE, 9600);
                      
    /* Enable Receiver and Transmitter lines */
    Driver_USART7.Control (ARM_USART_CONTROL_TX, 1);
    Driver_USART7.Control (ARM_USART_CONTROL_RX, 1);

	//while(Driver_USART7.GetStatus().tx_busy==1);

	while(1)
	{
		//Get bluetooth data and change speed based on that
		Driver_USART7.Receive(&data, 1);
		while(Driver_USART7.GetRxCount()!=1);
		
		if(data[0] == 0){
			//Because of the limited range, the rescaler remains the same no matter the acceleration
			TIM12->ARR = 57331;
			TIM12->CCR1 = 4082;
		} else {
			//Saturate data at |13|
				if(data[0] < -13){
					data[0] = -13;
				} else if (data[0] > 13){
					data[0] = 13;
				}
			/* 
				Accelerometer data changes frequency; from slow but continuous to fast cont. motion
				Calculate speed from data
				Approaching 1.3 ms: faster clockwise motion, P = 5, ARR = 56799
				Approaching 1.7 ms: faster anti-clockwise motion, P = 5, ARR = 57864
				Multiplying by 40 approximately maps the values obtainable from the 
				accelerometer to the possible range of ARR. The range is 1065 values and data[0]
				can be between -13 and 13 approximately, so ~40 units correspond.
			*/
			TIM12->ARR = 57331+data[0]*40;
			TIM12->CCR1 = (57331+data[0]*40)-53249;
		}
	}
	return 0;
}
