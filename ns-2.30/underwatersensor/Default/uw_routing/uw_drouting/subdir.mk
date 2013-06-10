################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../uw_routing/uw_drouting/uw_drouting.o \
../uw_routing/uw_drouting/uw_drouting_rtable.o 

CC_SRCS += \
../uw_routing/uw_drouting/uw_drouting.cc \
../uw_routing/uw_drouting/uw_drouting_rtable.cc 

OBJS += \
./uw_routing/uw_drouting/uw_drouting.o \
./uw_routing/uw_drouting/uw_drouting_rtable.o 

CC_DEPS += \
./uw_routing/uw_drouting/uw_drouting.d \
./uw_routing/uw_drouting/uw_drouting_rtable.d 


# Each subdirectory must supply rules for building sources it contributes
uw_routing/uw_drouting/%.o: ../uw_routing/uw_drouting/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


