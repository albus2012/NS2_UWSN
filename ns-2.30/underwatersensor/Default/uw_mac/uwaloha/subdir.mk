################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../uw_mac/uwaloha/uwaloha.o 

CC_SRCS += \
../uw_mac/uwaloha/uwaloha.cc 

OBJS += \
./uw_mac/uwaloha/uwaloha.o 

CC_DEPS += \
./uw_mac/uwaloha/uwaloha.d 


# Each subdirectory must supply rules for building sources it contributes
uw_mac/uwaloha/%.o: ../uw_mac/uwaloha/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


