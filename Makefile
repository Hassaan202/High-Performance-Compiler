# LLVM and Clang paths
HOMEBREW_LLVM = /opt/homebrew/opt/llvm
CXX = $(HOMEBREW_LLVM)/bin/clang++
LLVM_CONFIG = $(HOMEBREW_LLVM)/bin/llvm-config
LLC = $(HOMEBREW_LLVM)/bin/llc
OPT = $(HOMEBREW_LLVM)/bin/opt

# Files
LEXER = ssc.l
PARSER = ssc.y
OPTIMIZER = IR.h
EXECUTABLE = output
TEST_FILE = input.ssc
IR_FILE = output.ll
OPT_IR_FILE = output_opt.ll
ASM_FILE = output.s
OBJ_FILE = output.o
BIN_FILE = ssc_compiler_ir

# Intermediate files
LEX_C = lex.yy.c
PARSER_C = ssc.tab.c
PARSER_H = ssc.tab.h

# Compiler flags
FLEX_INCLUDE = -I$(shell brew --prefix flex)/include
CPPFLAGS = -I$(HOMEBREW_LLVM)/include $(FLEX_INCLUDE) -fopenmp -Wno-deprecated
LDFLAGS = -L$(HOMEBREW_LLVM)/lib -lomp
LLVM_CXXFLAGS = -std=c++17 $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS = -lLLVM

# MLIR libraries
MLIR_LIBS = -lMLIR -lMLIRExecutionEngine -lMLIRLLVMDialect -lMLIRLLVMIRTransforms \
            -lMLIRLLVMToLLVMIRTranslation -lMLIRTargetLLVMIRExport \
            -lMLIRAffineToStandard -lMLIRSCFToControlFlow -lMLIRControlFlowToLLVM \
            -lMLIRFuncToLLVM -lMLIRAffineDialect -lMLIRSCFDialect \
            -lMLIROpenMPDialect -lMLIRFuncDialect -lMLIROpenMPToLLVM

# Default target
all: $(EXECUTABLE)

# Generate parser
$(PARSER_C) $(PARSER_H): $(PARSER)
	bison -d -o $(PARSER_C) $(PARSER)

# Generate lexer
$(LEX_C): $(LEXER)
	flex -o $(LEX_C) $(LEXER)

# Build the compiler
$(EXECUTABLE): $(PARSER_C) $(PARSER_H) $(LEX_C) $(OPTIMIZER)
	$(CXX) $(CPPFLAGS) $(LLVM_CXXFLAGS) \
	    -o $(EXECUTABLE) \
	    $(PARSER_C) $(LEX_C) \
	    $(LDFLAGS) $(LLVM_LDFLAGS) $(LLVM_LIBS) $(MLIR_LIBS)

# Generate LLVM IR
$(IR_FILE): $(EXECUTABLE) $(TEST_FILE)
	./$(EXECUTABLE) < $(TEST_FILE) > $(IR_FILE)

ir: $(IR_FILE)

# Optimize LLVM IR
$(OPT_IR_FILE): $(IR_FILE)
	$(OPT) -O2 $(IR_FILE) -S -o $(OPT_IR_FILE)

opt: $(OPT_IR_FILE)

# Compile to assembly
$(ASM_FILE): $(OPT_IR_FILE)
	$(LLC) -filetype=asm $(OPT_IR_FILE) -o $(ASM_FILE)

# Compile to object file
$(OBJ_FILE): $(ASM_FILE)
	clang -c $(ASM_FILE) -o $(OBJ_FILE)

# Link to executable
$(BIN_FILE): $(OBJ_FILE)
	clang++ -Xpreprocessor -fopenmp -L$(HOMEBREW_LLVM)/lib -lomp -o $(BIN_FILE) $(OBJ_FILE)

# Build and run
run: $(BIN_FILE)
	./$(BIN_FILE)

# Clean up
clean:
	rm -f $(LEX_C) $(PARSER_C) $(PARSER_H) $(EXECUTABLE) $(IR_FILE) \
	      $(OPT_IR_FILE) $(ASM_FILE) $(OBJ_FILE) $(BIN_FILE)

.PHONY: all ir opt run clean
