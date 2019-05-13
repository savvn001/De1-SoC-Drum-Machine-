################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../DE1SoC_LT24/DE1SoC_LT24.c 

C_DEPS += \
./DE1SoC_LT24/DE1SoC_LT24.d 

OBJS += \
./DE1SoC_LT24/DE1SoC_LT24.o 


# Each subdirectory must supply rules for building sources it contributes
DE1SoC_LT24/%.o: ../DE1SoC_LT24/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler 5'
	armcc --cpu=Cortex-A9.no_neon --apcs=/hardfp --arm --c99 --gnu -O2 -Otime --loop_optimization_level=2 -g --md --depend_format=unix_escaped --no_depend_system_headers --depend_dir="DE1SoC_LT24" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


