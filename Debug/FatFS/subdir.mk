################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FatFS/diskio_cyclonev.c \
../FatFS/ff.c \
../FatFS/ffsystem.c \
../FatFS/ffunicode.c 

C_DEPS += \
./FatFS/diskio_cyclonev.d \
./FatFS/ff.d \
./FatFS/ffsystem.d \
./FatFS/ffunicode.d 

OBJS += \
./FatFS/diskio_cyclonev.o \
./FatFS/ff.o \
./FatFS/ffsystem.o \
./FatFS/ffunicode.o 


# Each subdirectory must supply rules for building sources it contributes
FatFS/%.o: ../FatFS/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler 5'
	armcc --cpu=Cortex-A9.no_neon --apcs=/hardfp --arm --c99 --gnu -O2 -Otime --loop_optimization_level=2 -g --md --depend_format=unix_escaped --no_depend_system_headers --depend_dir="FatFS" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


