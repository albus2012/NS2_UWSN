################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../uw_mobility_pattern/uw_mobility_kinematic.o \
../uw_mobility_pattern/uw_mobility_pattern.o \
../uw_mobility_pattern/uw_mobility_rwp.o 

CC_SRCS += \
../uw_mobility_pattern/uw_mobility_kinematic.cc \
../uw_mobility_pattern/uw_mobility_pattern.cc \
../uw_mobility_pattern/uw_mobility_rwp.cc 

OBJS += \
./uw_mobility_pattern/uw_mobility_kinematic.o \
./uw_mobility_pattern/uw_mobility_pattern.o \
./uw_mobility_pattern/uw_mobility_rwp.o 

CC_DEPS += \
./uw_mobility_pattern/uw_mobility_kinematic.d \
./uw_mobility_pattern/uw_mobility_pattern.d \
./uw_mobility_pattern/uw_mobility_rwp.d 


# Each subdirectory must supply rules for building sources it contributes
uw_mobility_pattern/%.o: ../uw_mobility_pattern/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


