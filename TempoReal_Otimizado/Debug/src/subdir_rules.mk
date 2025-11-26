################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
src/%.obj: ../src/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C5500 Compiler'
	"F:/ccs/tools/compiler/c5500_4.4.1/bin/cl55" -v5502 --memory_model=large -g --include_path="C:/Users/Flip_/Documents/GitHub/ENGG54_LAB_3_PROJECT/TempoReal_Otimizado" --include_path="C:/Users/Flip_/Documents/GitHub/ENGG54_LAB_3_PROJECT/TempoReal_Otimizado/inc" --include_path="C:/Users/Flip_/Documents/GitHub/ENGG54_LAB_3_PROJECT/TempoReal_Otimizado/src" --include_path="C:/Users/FabioM/Desktop/Faculdade UFBA/Semestre 2025.2/Laboratorio 3/ezdsp5502_v1/include" --include_path="C:/Users/FabioM/Desktop/Faculdade UFBA/Semestre 2025.2/Laboratorio 3/ezdsp5502_v1/C55xxCSL/include" --include_path="F:/ccs/tools/compiler/c5500_4.4.1/include" --define=c5502 --display_error_number --diag_warning=225 --ptrdiff_size=32 --preproc_with_compile --preproc_dependency="src/$(basename $(<F)).d_raw" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

src/%.obj: ../src/%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C5500 Compiler'
	"F:/ccs/tools/compiler/c5500_4.4.1/bin/cl55" -v5502 --memory_model=large -g --include_path="C:/Users/Flip_/Documents/GitHub/ENGG54_LAB_3_PROJECT/TempoReal_Otimizado" --include_path="C:/Users/Flip_/Documents/GitHub/ENGG54_LAB_3_PROJECT/TempoReal_Otimizado/inc" --include_path="C:/Users/Flip_/Documents/GitHub/ENGG54_LAB_3_PROJECT/TempoReal_Otimizado/src" --include_path="C:/Users/FabioM/Desktop/Faculdade UFBA/Semestre 2025.2/Laboratorio 3/ezdsp5502_v1/include" --include_path="C:/Users/FabioM/Desktop/Faculdade UFBA/Semestre 2025.2/Laboratorio 3/ezdsp5502_v1/C55xxCSL/include" --include_path="F:/ccs/tools/compiler/c5500_4.4.1/include" --define=c5502 --display_error_number --diag_warning=225 --ptrdiff_size=32 --preproc_with_compile --preproc_dependency="src/$(basename $(<F)).d_raw" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


