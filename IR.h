#include <string>
#include <map>
#include <stdio.h>
#include <vector>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <unistd.h>

using namespace llvm;

Value* getFromSymbolTable(const char *id);
void setDouble(const char *id, Value* value);
void printString(const char *str);
void printDouble(Value* value);
Value* performBinaryOperation(Value* lhs, Value* rhs, int op);
Value* createComparisonOperation(Value* lhs, Value* rhs, int op);
void handleIfStatement(Value* condition);
void endIfStatement();
void handleIfElseStatement(Value* condition);
void endIfThenBlock();
void endIfElseStatement();
void yyerror(const char *err);
static void initLLVM();
void printLLVMIR();
void addReturnInstr();
Value* createDoubleConstant(double val);

// handle for-loops
void startForLoop(Value* initVal, const char* counter, Value* endVal);
void endForLoop();

// handle functions
Function* defineFunction(const char* name);
void endFunctionDefinition(Value* returnValue);
Value* callFunction(const char* name, std::vector<Value*>& args);

// For managing if-else blocks
static BasicBlock *thenBlock = nullptr;
static BasicBlock *elseBlock = nullptr;
static BasicBlock *mergeBlock = nullptr;

// For managing for-loop blocks
static BasicBlock *loopHeaderBlock = nullptr;
static BasicBlock *loopBodyBlock = nullptr;
static BasicBlock *loopEndBlock = nullptr;
static Value *loopCounter = nullptr;
static Value *loopEndValue = nullptr;
static std::string loopCounterName;

// For managing functions
static std::map<std::string, Function*> FunctionTable;
static Function *currentFunction = nullptr;
static BasicBlock *currentEntryBlock = nullptr;
static bool inFunctionDefinition = false;

static std::map<std::string, Value *> SymbolTable;
static std::map<std::string, std::map<std::string, Value*>> FunctionSymbolTables;

static LLVMContext context;
static Module *module = nullptr;
static IRBuilder<> builder(context);
static Function *mainFunction = nullptr;

/**
* init LLVM
* Create main function (similar to C-main) that returns a int but takes no parameters.
*/
static void initLLVM() {
    module = new Module("ssc_program", context);
    
    // Returns an int and does not take any parameters.
    FunctionType *mainTy = FunctionType::get(builder.getInt32Ty(), false);
    
    // The main function definition.
    mainFunction = Function::Create(mainTy, Function::ExternalLinkage, "main", module);
    
    // Create entry basic block of the main function.
    BasicBlock *entry = BasicBlock::Create(context, "entry", mainFunction);
    
    // Tell builder that instruction to be added in this basic block.
    builder.SetInsertPoint(entry);
    currentFunction = mainFunction;
    currentEntryBlock = entry;
    
    // This ensures that empty functions have at least one instruction (debug)
    builder.CreateAlloca(builder.getDoubleTy(), nullptr, "dummy_alloca");
}

void addReturnInstr() {
    // Make sure we have a terminator instruction at the end of each function
    if (builder.GetInsertBlock() && !builder.GetInsertBlock()->getTerminator()) {
        // For main function, return 0
        if (currentFunction == mainFunction) {
            builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));
        } 
        // For other functions, return 0.0 as default
        else if (currentFunction->getReturnType()->isDoubleTy()) {
            builder.CreateRet(ConstantFP::get(context, APFloat(0.0)));
        }
    }
}

Value* createDoubleConstant(double val) {
    return ConstantFP::get(context, APFloat(val));
}

void printLLVMIR() {
    // Ensure all functions have terminators before printing
    for (auto &F : *module) {
        for (auto &BB : F) {
            if (!BB.getTerminator()) {
                builder.SetInsertPoint(&BB);
                if (F.getReturnType()->isDoubleTy()) {
                    builder.CreateRet(ConstantFP::get(context, APFloat(0.0)));
                } else {
                    builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));
                }
            }
        }
    }
    
    std::error_code EC;
    raw_fd_ostream out(STDOUT_FILENO, false);
    module->print(out, nullptr);
}

// Modified to ensure variables are properly allocated and tracked
Value* getFromSymbolTable(const char *id) {
    std::string name(id);
    std::string functionName = "";
    
    if (inFunctionDefinition && currentFunction != mainFunction) {
        functionName = currentFunction->getName().str();
    }
    
    // If we're in a function, check function-specific symbol table first
    if (!functionName.empty()) {
        if (FunctionSymbolTables[functionName].find(name) != FunctionSymbolTables[functionName].end()) {
            return FunctionSymbolTables[functionName][name];
        }
    }
    
    // Check global symbol table
    if (SymbolTable.find(name) != SymbolTable.end()) {
        return SymbolTable[name];
    } else {
        // Create a new allocation for the variable
        AllocaInst *alloca;
        
        if (inFunctionDefinition && currentFunction != mainFunction) {
            // Create allocation at the beginning of the function's entry block
            IRBuilder<> TmpB(&currentFunction->getEntryBlock(), 
                          currentFunction->getEntryBlock().begin());
            alloca = TmpB.CreateAlloca(builder.getDoubleTy(), nullptr, name);
            
            // Store in function symbol table
            FunctionSymbolTables[functionName][name] = alloca;
        } else {
            // Create allocation at the beginning of main function's entry block
            IRBuilder<> TmpB(&mainFunction->getEntryBlock(), 
                          mainFunction->getEntryBlock().begin());
            alloca = TmpB.CreateAlloca(builder.getDoubleTy(), nullptr, name);
            
            // Store in global symbol table
            SymbolTable[name] = alloca;
        }
        
        // Initialize with 0.0
        builder.CreateStore(createDoubleConstant(0.0), alloca);
        return alloca;
    }
}

void setDouble(const char *id, Value* value) {
    Value *ptr = getFromSymbolTable(id);
    builder.CreateStore(value, ptr);
}

/**
* This is a general LLVM function to print a value in given format.
*/
void printfLLVM(const char *format, Value *inputValue) {
    // Check if printf function already exists
    Function *printfFunc = module->getFunction("printf");
    
    // If it does not exist then create it
    if(!printfFunc) {
        // The printf function returns integer
        // It takes variable number of parameters
        std::vector<Type*> args = {builder.getInt8Ty()->getPointerTo()};
        FunctionType *printfTy = FunctionType::get(builder.getInt32Ty(), args, true);
        printfFunc = Function::Create(printfTy, Function::ExternalLinkage, "printf", module);
    }
    
    // Create global string pointer for format
    Value *formatVal = builder.CreateGlobalStringPtr(format);
    
    // Create arguments vector
    std::vector<Value*> args = {formatVal, inputValue};
    
    // Call the printf function using Call LLVM instruction
    builder.CreateCall(printfFunc, args, "printfCall");
}

void printString(const char *str) {
    // Remove the quotes from the string literal
    std::string s(str);
    if (s.size() >= 2 && s[0] == '"' && s[s.size()-1] == '"') {
        s = s.substr(1, s.size()-2);
    }
    
    Value *strValue = builder.CreateGlobalStringPtr(s);
    printfLLVM("%s\n", strValue);
}

void printDouble(Value *value) {
    printfLLVM("%lf\n", value); 
}

Value* performBinaryOperation(Value* lhs, Value* rhs, int op) {
    if (!lhs || !rhs) {
        yyerror("Null operand in binary operation");
        return createDoubleConstant(0.0);
    }
    
    switch (op) {
        case '+': return builder.CreateFAdd(lhs, rhs, "fadd");
        case '-': return builder.CreateFSub(lhs, rhs, "fsub");
        case '*': return builder.CreateFMul(lhs, rhs, "fmul");
        case '/': return builder.CreateFDiv(lhs, rhs, "fdiv");
        default: yyerror("illegal binary operation"); return createDoubleConstant(0.0);
    }
}

/* 
 * Function to create comparison operations for conditions
 */
Value* createComparisonOperation(Value* lhs, Value* rhs, int op) {
    if (!lhs || !rhs) {
        yyerror("Null operand in comparison operation");
        return builder.getInt1(false);
    }
    
    switch (op) {
        case '>': return builder.CreateFCmpOGT(lhs, rhs, "fcmp_gt");
        case '<': return builder.CreateFCmpOLT(lhs, rhs, "fcmp_lt");
        case '=': return builder.CreateFCmpOEQ(lhs, rhs, "fcmp_eq");
        default: yyerror("illegal comparison operation"); return builder.getInt1(false);
    }
}

/*
 * Handle if statement without else
 */
void handleIfStatement(Value* condition) {
    if (!condition) {
        yyerror("Null condition in if statement");
        return;
    }
    
    // Create basic blocks for then and merge
    Function *func = builder.GetInsertBlock()->getParent();
    thenBlock = BasicBlock::Create(context, "then", func);
    mergeBlock = BasicBlock::Create(context, "ifcont");
    
    // Create conditional branch
    builder.CreateCondBr(condition, thenBlock, mergeBlock);
    
    // Emit then block
    builder.SetInsertPoint(thenBlock);
}

void endIfStatement() {
    // Create branch to merge block
    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        builder.CreateBr(mergeBlock);
    }
    
    // Add merge block to function
    Function *func = builder.GetInsertBlock()->getParent();
    func->getBasicBlockList().push_back(mergeBlock);
    builder.SetInsertPoint(mergeBlock);
}

/*
 * Handle if-else statement
 */
void handleIfElseStatement(Value* condition) {
    if (!condition) {
        yyerror("Null condition in if-else statement");
        return;
    }
    
    // Create basic blocks for then, else, and merge
    Function *func = builder.GetInsertBlock()->getParent();
    thenBlock = BasicBlock::Create(context, "then", func);
    elseBlock = BasicBlock::Create(context, "else");
    mergeBlock = BasicBlock::Create(context, "ifcont");
    
    // Create conditional branch
    builder.CreateCondBr(condition, thenBlock, elseBlock);
    
    // Emit then block
    builder.SetInsertPoint(thenBlock);
}

void endIfThenBlock() {
    // Create branch to merge block from then block
    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        builder.CreateBr(mergeBlock);
    }
    
    // Add else block to function and set insertion point
    Function *func = builder.GetInsertBlock()->getParent();
    func->getBasicBlockList().push_back(elseBlock);
    builder.SetInsertPoint(elseBlock);
}

void endIfElseStatement() {
    // Create branch to merge block from else block
    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        builder.CreateBr(mergeBlock);
    }
    
    // Add merge block to function
    Function *func = builder.GetInsertBlock()->getParent();
    func->getBasicBlockList().push_back(mergeBlock);
    builder.SetInsertPoint(mergeBlock);
}

/*
 * Start a for loop with initialization, condition, and increment
 */
 void startForLoop(Value* initVal, const char* counter, Value* endVal) {
    if (!initVal || !endVal) {
        yyerror("Null values in for loop");
        return;
    }
    
    Function *func = builder.GetInsertBlock()->getParent();
    
    // Store the loop counter name
    loopCounterName = std::string(counter);
    
    // Allocate and initialize loop counter
    Value* counterPtr = getFromSymbolTable(counter);
    builder.CreateStore(initVal, counterPtr);
    loopCounter = counterPtr;
    loopEndValue = endVal;
    
    // Create loop blocks
    loopHeaderBlock = BasicBlock::Create(context, "loop_header", func);
    loopBodyBlock = BasicBlock::Create(context, "loop_body", func);
    loopEndBlock = BasicBlock::Create(context, "loop_end");
    
    // Branch to loop header
    builder.CreateBr(loopHeaderBlock);
    
    // Set insert point to loop header
    builder.SetInsertPoint(loopHeaderBlock);
    
    // Load current counter value
    Value* currentVal = builder.CreateLoad(builder.getDoubleTy(), counterPtr, "current_val");
    
    // Check condition (counter < endVal)
    Value* condition = builder.CreateFCmpOLT(currentVal, endVal, "loop_cond");
    
    // Branch based on condition
    builder.CreateCondBr(condition, loopBodyBlock, loopEndBlock);
    
    // Set insert point to loop body
    builder.SetInsertPoint(loopBodyBlock);
}

void endForLoop() {
    if (!loopCounter) {
        yyerror("Null loop counter in endForLoop");
        return;
    }
    Value* counterPtr = loopCounter;
    Value* currentVal = builder.CreateLoad(builder.getDoubleTy(), counterPtr, "current_val");
    // printfLLVM("Counter before increment: %lf\n", currentVal); // Debug print
    Value* incrementedVal = builder.CreateFAdd(currentVal, createDoubleConstant(1.0), "incremented_val");
    builder.CreateStore(incrementedVal, counterPtr);
    // printfLLVM("Counter after increment: %lf\n", incrementedVal); // Debug print
    builder.CreateBr(loopHeaderBlock);
    Function *func = builder.GetInsertBlock()->getParent();
    func->getBasicBlockList().push_back(loopEndBlock);
    builder.SetInsertPoint(loopEndBlock);
}

/*
 * Define a new function in the SSC language
 */
Function* defineFunction(const char* name) {
    // Save current insertion point for later restoration
    Function *savedFunction = currentFunction;
    BasicBlock *savedBlock = builder.GetInsertBlock();
    
    // Create function type (returns double, takes no parameters for simplicity)
    FunctionType *funcType = FunctionType::get(builder.getDoubleTy(), false);
    
    // Create function
    std::string funcName(name);
    Function *func = Function::Create(funcType, Function::ExternalLinkage, funcName, module);
    
    // Add to function table
    FunctionTable[funcName] = func;
    
    // Create entry block
    BasicBlock *entry = BasicBlock::Create(context, "entry", func);
    
    // Set insert point to new function's entry block
    builder.SetInsertPoint(entry);
    currentFunction = func;
    inFunctionDefinition = true;
    currentEntryBlock = entry;
    
    // Create a new symbol table for this function
    FunctionSymbolTables[funcName] = std::map<std::string, Value*>();
    
    // Create a placeholder instruction to ensure the block is not empty
    builder.CreateAlloca(builder.getDoubleTy(), nullptr, "func_placeholder");
    
    return func;
}

void endFunctionDefinition(Value* returnValue) {
    // Skip if already in main or no function defined
    if (!inFunctionDefinition || !currentFunction || currentFunction == mainFunction) {
        return;
    }
    
    // Use provided return value or default to 0.0
    Value* retVal = returnValue ? returnValue : createDoubleConstant(0.0);
    
    // Create return instruction
    builder.CreateRet(retVal);
    
    // Reset flag
    inFunctionDefinition = false;
    
    // Set insert point back to main function
    builder.SetInsertPoint(&mainFunction->getEntryBlock());
    currentFunction = mainFunction;
}

/*
 * Call a defined function
 */
Value* callFunction(const char* name, std::vector<Value*>& args) {
    std::string funcName(name);
    
    // Get function from table
    if (FunctionTable.find(funcName) == FunctionTable.end()) {
        std::string errMsg = "Function '" + funcName + "' not defined";
        yyerror(errMsg.c_str());
        return createDoubleConstant(0.0);
    }
    
    // Get the function from the table
    Function *func = FunctionTable[funcName];
    
    // Call function
    return builder.CreateCall(func, args, "call_result");
}
