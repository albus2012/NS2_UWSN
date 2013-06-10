################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../uw_routing/static_routing/static_routing.o 

CC_SRCS += \
../uw_routing/static_routing/static_routing.cc 

OBJS += \
./uw_routing/static_routing/static_routing.o 

CC_DEPS += \
./uw_routing/static_routing/static_routing.d 


# Each subdirectory must supply rules for building sources it contributes
uw_routing/static_routing/%.o: ../uw_routing/static_routing/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


