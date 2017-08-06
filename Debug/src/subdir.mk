################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/TestXPlaneUDPClient.cpp \
../src/XPUtils.cpp \
../src/XPlaneBeaconListener.cpp \
../src/XPlaneUDPClient.cpp 

OBJS += \
./src/TestXPlaneUDPClient.o \
./src/XPUtils.o \
./src/XPlaneBeaconListener.o \
./src/XPlaneUDPClient.o 

CPP_DEPS += \
./src/TestXPlaneUDPClient.d \
./src/XPUtils.d \
./src/XPlaneBeaconListener.d \
./src/XPlaneUDPClient.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-gnueabihf-g++ -std=c++0x -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


