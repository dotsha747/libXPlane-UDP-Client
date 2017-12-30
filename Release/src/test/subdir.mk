################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/test/TestBeaconListener.cpp \
../src/test/TestXPlaneUDPClient.cpp 

O_SRCS += \
../src/test/TestBeaconListener.o \
../src/test/TestXPlaneUDPClient.o 

OBJS += \
./src/test/TestBeaconListener.o \
./src/test/TestXPlaneUDPClient.o 

CPP_DEPS += \
./src/test/TestBeaconListener.d \
./src/test/TestXPlaneUDPClient.d 


# Each subdirectory must supply rules for building sources it contributes
src/test/%.o: ../src/test/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -std=c++0x -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


