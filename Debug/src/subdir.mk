################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/lcd.c \
../src/main.c \
../src/support_functions.c \
../src/syscalls.c \
../src/system_stm32f0xx.c 

OBJS += \
./src/lcd.o \
./src/main.o \
./src/support_functions.o \
./src/syscalls.o \
./src/system_stm32f0xx.o 

C_DEPS += \
./src/lcd.d \
./src/main.d \
./src/support_functions.d \
./src/syscalls.d \
./src/system_stm32f0xx.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F0 -DSTM32F091RCTx -DDEBUG -DSTM32F091 -DUSE_STDPERIPH_DRIVER -I"C:/Users/mike/Desktop/Senior Design Code/Senior-Design/StdPeriph_Driver/inc" -I"C:/Users/mike/Desktop/Senior Design Code/Senior-Design/inc" -I"C:/Users/mike/Desktop/Senior Design Code/Senior-Design/CMSIS/device" -I"C:/Users/mike/Desktop/Senior Design Code/Senior-Design/CMSIS/core" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


