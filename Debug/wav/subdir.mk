################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../wav/wav.c 

C_DEPS += \
./wav/wav.d 

OBJS += \
./wav/wav.o 


# Each subdirectory must supply rules for building sources it contributes
wav/%.o: ../wav/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler 5'
	armcc --cpu=Cortex-A9 --apcs=/hardfp --arm --c99 --gnu -O0 -Otime -g --md --depend_format=unix_escaped --no_depend_system_headers --depend_dir="wav" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


