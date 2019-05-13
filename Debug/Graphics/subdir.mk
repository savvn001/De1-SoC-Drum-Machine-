################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Graphics/Graphics.c 

C_DEPS += \
./Graphics/Graphics.d 

OBJS += \
./Graphics/Graphics.o 


# Each subdirectory must supply rules for building sources it contributes
Graphics/%.o: ../Graphics/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler 5'
	armcc --cpu=Cortex-A9 --apcs=/hardfp --arm --c99 --gnu -O0 -Otime -g --md --depend_format=unix_escaped --no_depend_system_headers --depend_dir="Graphics" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


