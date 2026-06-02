#pragma once


#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

#include <memory>
#include <string>


#include "../KaleidoscopeJIT.h"
#include "../nsk_cpp.h"
#include "logging.h"



using namespace llvm;
using namespace orc;

extern std::unique_ptr<KaleidoscopeJIT> TheJIT;
extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<LLVMContext> GlobalContext;

extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<Module> GlobalModule;
extern std::unique_ptr<TargetMachine> CTM;


extern std::map<std::string, StructType*> struct_types;
extern std::unordered_map<std::string, int> struct_type_size;

extern std::unordered_map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
extern std::unordered_map<std::string, PrototypeAST*> PriorityProtos;
extern std::unordered_map<std::string, std::function<llvm::Type*(std::unique_ptr<LLVMContext>&)>> data_register_fn;
extern std::unordered_map<std::string, std::function<llvm::PointerType*(std::unique_ptr<LLVMContext>&)>> data_ptr_register_fn;
extern std::unordered_map<std::string, llvm::Type*> tuple_cache;

extern std::unordered_map<std::string, std::function<Value*(Parser_Struct, Function*, std::string, std::string, Data_Tree,
                                                       Value*, Value*,
                                                       std::vector<std::unique_ptr<ExprAST>>&,
                                                       std::vector<Value*>&)>> struct_create_fn;

extern std::unordered_map<std::string, std::function<Value*(Parser_Struct, Function*, std::string,
                                                       Data_Tree,
                                                       std::vector<Data_Tree>&,
                                                       Value*,
                                                       std::vector<std::unique_ptr<ExprAST>>&,
                                                       std::vector<Value*>&)>> llvm_callee;

extern std::unordered_map<std::string, std::function<Value*(Parser_Struct, Function*, 
                                                       Data_Tree, Data_Tree,
                                                       std::unique_ptr<ExprAST>&,
                                                       std::unique_ptr<ExprAST>&,
                                                       Value*,
                                                       Value*, Value*)>> llvm_data_ops;

extern std::unordered_map<std::string, std::function<Value*(Parser_Struct, Function*, 
                                                       Data_Tree, Data_Tree,
                                                       std::unique_ptr<ExprAST>&,
                                                       std::unique_ptr<ExprAST>&,
                                                       Value*, Value*,
                                                       Value*, Value*)>> llvm_store_idx;






inline Value *const_int8(int8_t val) {
    return ConstantInt::get(Type::getInt8Ty(*TheContext), val);
}
inline Value *const_int(int val) {
    return ConstantInt::get(Type::getInt32Ty(*TheContext), val);
}
inline Value *const_int16(int16_t val) {
    return ConstantInt::get(Type::getInt16Ty(*TheContext), val);
}
inline Value *const_uint16(uint16_t val) {
    return ConstantInt::get(Type::getInt16Ty(*TheContext), val);
}
inline Value *const_int64(int64_t val) {
    return ConstantInt::get(Type::getInt64Ty(*TheContext), val);
}
inline Value *const_float(float val) {
    return ConstantFP::get(*TheContext, APFloat(val));
}
inline Value *const_bool(bool val) {
    return ConstantInt::get(Type::getInt1Ty(*TheContext), val);
}



inline Function *getFunctionCheck(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name))
    return F;

  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();
  LogError(-1, "The function " + Name + " was not found.");
  // If no existing prototype exists, return null.
  return nullptr;
}

inline void call(std::string fn, const std::vector<Value *> &args) {
    if(!Shall_Exit)
        Builder->CreateCall(getFunctionCheck(fn), args);
}
inline Value *callret(std::string fn, const std::vector<Value *> &args) { 
    if(!Shall_Exit)
        return Builder->CreateCall(getFunctionCheck(fn), args);
    return const_float(0);
}



inline Value *global_str(std::string _string) {
    return Builder->CreateGlobalString(_string);
}




inline AllocaInst *int_alloca() {
    return Builder->CreateAlloca(Type::getInt32Ty(*TheContext), nullptr);
}
inline AllocaInst *float_alloca() {
    return Builder->CreateAlloca(Type::getFloatTy(*TheContext), nullptr);
}

inline void store_alloca(AllocaInst *alloca, Value *val) {
    Builder->CreateStore(val, alloca);
}

inline Value *load_int(AllocaInst *alloca) {
    Builder->CreateLoad(Type::getInt32Ty(*TheContext), alloca, "loaded");
}
inline Value *load_float(AllocaInst *alloca) {
    Builder->CreateLoad(Type::getFloatTy(*TheContext), alloca, "loaded");
}



inline void check_llvm_err(){
    std::string err;
    llvm::raw_string_ostream os(err);
    if (llvm::verifyModule(*TheModule, &os)) {
        errs() << "Module broken:\n" << os.str();
        TheModule->print(llvm::errs(), nullptr);
        // abort();
    }
}
