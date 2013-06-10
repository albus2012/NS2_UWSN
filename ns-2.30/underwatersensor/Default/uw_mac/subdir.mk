################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
O_SRCS += \
../uw_mac/rmac.o \
../uw_mac/tmac.o \
../uw_mac/underwaterchannel.o \
../uw_mac/underwatermac.o \
../uw_mac/underwaterphy.o \
../uw_mac/underwaterpropagation.o \
../uw_mac/uwbuffer.o 

CC_SRCS += \
../uw_mac/broadcastmac.cc \
../uw_mac/rmac.cc \
../uw_mac/tmac.cc \
../uw_mac/underwaterchannel.cc \
../uw_mac/underwatermac.cc \
../uw_mac/underwaterphy.cc \
../uw_mac/underwaterpropagation.cc \
../uw_mac/underwaterstaticmac.cc \
../uw_mac/uwbuffer.cc 

OBJS += \
./uw_mac/broadcastmac.o \
./uw_mac/rmac.o \
./uw_mac/tmac.o \
./uw_mac/underwaterchannel.o \
./uw_mac/underwatermac.o \
./uw_mac/underwaterphy.o \
./uw_mac/underwaterpropagation.o \
./uw_mac/underwaterstaticmac.o \
./uw_mac/uwbuffer.o 

CC_DEPS += \
./uw_mac/broadcastmac.d \
./uw_mac/rmac.d \
./uw_mac/tmac.d \
./uw_mac/underwaterchannel.d \
./uw_mac/underwatermac.d \
./uw_mac/underwaterphy.d \
./uw_mac/underwaterpropagation.d \
./uw_mac/underwaterstaticmac.d \
./uw_mac/uwbuffer.d 


# Each subdirectory must supply rules for building sources it contributes
uw_mac/%.o: ../uw_mac/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


