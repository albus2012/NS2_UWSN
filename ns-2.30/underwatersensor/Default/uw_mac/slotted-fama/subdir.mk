################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../uw_mac/slotted-fama/sfama.o 

CC_SRCS += \
../uw_mac/slotted-fama/sfama.cc 

OBJS += \
./uw_mac/slotted-fama/sfama.o 

CC_DEPS += \
./uw_mac/slotted-fama/sfama.d 


# Each subdirectory must supply rules for building sources it contributes
uw_mac/slotted-fama/%.o: ../uw_mac/slotted-fama/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


