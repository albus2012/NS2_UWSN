################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../uw_mac/uwan-mac/uwan-mac-pkt.o \
../uw_mac/uwan-mac/uwan-mac.o 

CC_SRCS += \
../uw_mac/uwan-mac/uwan-mac-pkt.cc \
../uw_mac/uwan-mac/uwan-mac.cc 

OBJS += \
./uw_mac/uwan-mac/uwan-mac-pkt.o \
./uw_mac/uwan-mac/uwan-mac.o 

CC_DEPS += \
./uw_mac/uwan-mac/uwan-mac-pkt.d \
./uw_mac/uwan-mac/uwan-mac.d 


# Each subdirectory must supply rules for building sources it contributes
uw_mac/uwan-mac/%.o: ../uw_mac/uwan-mac/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


