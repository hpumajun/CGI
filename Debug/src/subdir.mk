################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cgi.c \
../src/http.c \
../src/http_get.c \
../src/http_post.c \
../src/http_server.c 

OBJS += \
./src/cgi.o \
./src/http.o \
./src/http_get.o \
./src/http_post.o \
./src/http_server.o 

C_DEPS += \
./src/cgi.d \
./src/http.d \
./src/http_get.d \
./src/http_post.d \
./src/http_server.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


