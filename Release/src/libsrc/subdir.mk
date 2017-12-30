################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/libsrc/XPUtils.cpp \
../src/libsrc/XPlaneBeaconListener.cpp \
../src/libsrc/XPlaneUDPClient.cpp 

OBJS += \
./src/libsrc/XPUtils.o \
./src/libsrc/XPlaneBeaconListener.o \
./src/libsrc/XPlaneUDPClient.o 

CPP_DEPS += \
./src/libsrc/XPUtils.d \
./src/libsrc/XPlaneBeaconListener.d \
./src/libsrc/XPlaneUDPClient.d 


# Each subdirectory must supply rules for building sources it contributes
src/libsrc/%.o: ../src/libsrc/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -std=c++0x -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


