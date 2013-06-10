################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../uw_routing/uw_routing_buffer.o \
../uw_routing/vectorbasedforward.o \
../uw_routing/vectorbasedvoidavoidance.o 

CC_SRCS += \
../uw_routing/uw_routing_buffer.cc \
../uw_routing/uwflooding.cc \
../uw_routing/vectorbasedforward.cc \
../uw_routing/vectorbasedforward_hop_by_hop.cc \
../uw_routing/vectorbasedvoidavoidance.cc 

OBJS += \
./uw_routing/uw_routing_buffer.o \
./uw_routing/uwflooding.o \
./uw_routing/vectorbasedforward.o \
./uw_routing/vectorbasedforward_hop_by_hop.o \
./uw_routing/vectorbasedvoidavoidance.o 

CC_DEPS += \
./uw_routing/uw_routing_buffer.d \
./uw_routing/uwflooding.d \
./uw_routing/vectorbasedforward.d \
./uw_routing/vectorbasedforward_hop_by_hop.d \
./uw_routing/vectorbasedvoidavoidance.d 


# Each subdirectory must supply rules for building sources it contributes
uw_routing/%.o: ../uw_routing/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


