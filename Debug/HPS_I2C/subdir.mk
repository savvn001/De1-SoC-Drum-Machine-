################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../HPS_I2C/HPS_I2C.c 

C_DEPS += \
./HPS_I2C/HPS_I2C.d 

OBJS += \
./HPS_I2C/HPS_I2C.o 


# Each subdirectory must supply rules for building sources it contributes
HPS_I2C/%.o: ../HPS_I2C/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler 5'
	armcc --cpu=Cortex-A9 --apcs=/hardfp --arm --c99 --gnu -O0 -Otime -g --md --depend_format=unix_escaped --no_depend_system_headers --depend_dir="HPS_I2C" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


