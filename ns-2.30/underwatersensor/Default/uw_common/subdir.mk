################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../uw_common/underwatersensornode.o \
../uw_common/uw_hash_table.o \
../uw_common/uw_poi_traffic.o \
../uw_common/uw_sink.o \
../uw_common/uw_sink_vbva.o 

CC_SRCS += \
../uw_common/underwatersensornode.cc \
../uw_common/uw_hash_table.cc \
../uw_common/uw_poi_traffic.cc \
../uw_common/uw_sink.cc \
../uw_common/uw_sink_vbva.cc 

OBJS += \
./uw_common/underwatersensornode.o \
./uw_common/uw_hash_table.o \
./uw_common/uw_poi_traffic.o \
./uw_common/uw_sink.o \
./uw_common/uw_sink_vbva.o 

CC_DEPS += \
./uw_common/underwatersensornode.d \
./uw_common/uw_hash_table.d \
./uw_common/uw_poi_traffic.d \
./uw_common/uw_sink.d \
./uw_common/uw_sink_vbva.d 


# Each subdirectory must supply rules for building sources it contributes
uw_common/%.o: ../uw_common/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


