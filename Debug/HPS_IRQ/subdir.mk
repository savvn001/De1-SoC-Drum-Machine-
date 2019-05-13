################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../HPS_IRQ/HPS_IRQ.c 

C_DEPS += \
./HPS_IRQ/HPS_IRQ.d 

OBJS += \
./HPS_IRQ/HPS_IRQ.o 


# Each subdirectory must supply rules for building sources it contributes
HPS_IRQ/%.o: ../HPS_IRQ/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler 5'
	armcc --cpu=Cortex-A9.no_neon --apcs=/hardfp --arm --c99 --gnu -O2 -Otime --loop_optimization_level=2 -g --md --depend_format=unix_escaped --no_depend_system_headers --depend_dir="HPS_IRQ" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


