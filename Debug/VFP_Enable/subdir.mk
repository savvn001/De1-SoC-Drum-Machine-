################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../VFP_Enable/VFP_Enable.c 

C_DEPS += \
./VFP_Enable/VFP_Enable.d 

OBJS += \
./VFP_Enable/VFP_Enable.o 


# Each subdirectory must supply rules for building sources it contributes
VFP_Enable/%.o: ../VFP_Enable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler 5'
	armcc --cpu=Cortex-A9 --apcs=/hardfp --arm --c99 --gnu -O0 -Otime -g --md --depend_format=unix_escaped --no_depend_system_headers --depend_dir="VFP_Enable" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


