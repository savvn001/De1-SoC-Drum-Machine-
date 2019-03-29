################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../HPS_usleep/HPS_usleep.c 

C_DEPS += \
./HPS_usleep/HPS_usleep.d 

OBJS += \
./HPS_usleep/HPS_usleep.o 


# Each subdirectory must supply rules for building sources it contributes
HPS_usleep/%.o: ../HPS_usleep/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler 5'
	armcc --cpu=Cortex-A9 --apcs=/hardfp --arm --c99 --gnu -O0 -Otime -g --md --depend_format=unix_escaped --no_depend_system_headers --depend_dir="HPS_usleep" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


