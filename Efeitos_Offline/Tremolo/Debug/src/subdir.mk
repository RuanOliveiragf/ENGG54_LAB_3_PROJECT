################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/tremolo.c \
../src/tremoloTest.c 

OBJS += \
./src/tremolo.obj \
./src/tremoloTest.obj 

C_DEPS += \
./src/tremolo.pp \
./src/tremoloTest.pp 

C_DEPS__QUOTED += \
"src\tremolo.pp" \
"src\tremoloTest.pp" 

OBJS__QUOTED += \
"src\tremolo.obj" \
"src\tremoloTest.obj" 

C_SRCS__QUOTED += \
"../src/tremolo.c" \
"../src/tremoloTest.c" 


# Each subdirectory must supply rules for building sources it contributes
src/tremolo.obj: ../src/tremolo.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/tremolo.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/tremoloTest.obj: ../src/tremoloTest.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: C5500 Compiler'
	"C:/ti/ccsv5/tools/compiler/c5500/bin/cl55" -v5505 -g --include_path="C:/ti/ccsv5/tools/compiler/c5500/include" --include_path="..\inc" --diag_warning=225 --ptrdiff_size=16 --memory_model=large --preproc_with_compile --preproc_dependency="src/tremoloTest.pp" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


