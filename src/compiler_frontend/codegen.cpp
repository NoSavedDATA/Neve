#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

#include <string>
#include <map>
#include <unordered_map>
#include <vector>


#include <filesystem>
#include <fstream>


#include "../runtime/common/extension_functions.h"
#include "../runtime/data_types/include.h"
#include "../simd/include.h"
#include "../KaleidoscopeJIT.h"

#include "include.h"




using namespace llvm;
namespace fs = std::filesystem;


std::unordered_map<std::string, llvm::Type*> tuple_cache;
std::map<std::string, int> fn_stack_offset;
std::map<std::string, std::map<std::string, AllocaInst *>> function_allocas;
std::map<std::string, std::map<std::string, Value *>> function_values;
std::map<BasicBlock*, std::map<std::string, Value *>> block_values;
std::map<std::string, std::map<Value *, Value *>> function_vecs;
std::map<std::string, std::map<Value *, Value *>> function_vecs_size;
std::map<std::string, std::map<std::string, Value *>> function_pointers;
std::vector<std::string> prebuild_functions;
std::unordered_map<std::string, llvm::Type*> str_toTy;
std::unordered_map<std::string, Function *> async_fn;
std::string current_codegen_function;
std::unordered_map<std::string, std::function<Data_Tree(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&)>>function_return_overwrite;
std::unordered_map<std::string, std::function<Data_Tree(Parser_Struct, std::vector<std::unique_ptr<ExprAST>>&, std::unique_ptr<Nameable> &inner)>>method_return_overwrite;

std::vector<std::string> Global_Uniques;
std::unordered_map<std::string, int> Global_Uniques_Idx;


std::vector<Value *> thread_pointers;

PointerType *floatPtrTy, *int8PtrTy, *int1PtrTy;
llvm::Type *floatTy, *intTy, *int8Ty, *int16Ty, *int64Ty, *m256Ty, *boolTy, *voidTy;


Value *stack_top_value, *cur_self=nullptr;


llvm::Type *get_type_from_data(Data_Tree dt) {
  std::string type = dt.Type;
  llvm::Type *llvm_type;
  if (dt.is_array) {
    llvm_type = get_type_from_data(Data_Tree(dt.Type));
    int size = std::stoi(dt.Nested_Data[0].Type);
    llvm_type = ArrayType::get(llvm_type, size);
  }
  else if (str_toTy.count(type)>0)
      llvm_type = str_toTy[type];
  else if (type=="tuple") {
    std::string dt_str = dt.toString();
    if (tuple_cache.count(dt_str)>0)
        return tuple_cache[dt_str];
    std::vector<llvm::Type *> tupleTypes;
    for (auto dt : dt.Nested_Data)
        tupleTypes.push_back(get_type_from_data(dt));
    llvm_type = StructType::create(
        *TheContext,
        tupleTypes,
        dt_str
    );
    tuple_cache[dt_str] = llvm_type;
  }
  else if (type=="vec") {
    llvm::Type *inner_dt = get_type_from_data(Data_Tree(dt.Nested_Data[0].Type));
    int size = std::stoi(dt.Nested_Data[1].Type);
    llvm_type = VectorType::get(inner_dt, size, false);
  }
  else
    llvm_type = int8PtrTy;
  
  return llvm_type;
}




Value * VoidPtr_toValue(void *vec) {
  Value* LLVMValue = ConstantInt::get(int64Ty, reinterpret_cast<uint64_t>(vec));
  return Builder->CreateIntToPtr(LLVMValue, int8PtrTy);
}

inline void printTy(Value *v) {
    llvm::Type *ty = v->getType();
    ty->print(llvm::errs());
    llvm::errs() << "\n";
}

inline void bb_name(BasicBlock *bb) {
    errs() << "bb: " << bb->getName() << "\n";
}
inline void v_name(Value *v) {
    errs() << v->getName() << "\n";
}
inline void v_ir(Value *v) {
    v->print(errs());
    errs() << "\n";
}

Value *load_alloca(std::string name, std::string type, std::string from_function) {
    llvm::Type *load_type;
    if(type=="float")
      load_type = Type::getFloatTy(*TheContext);
    else if(type=="int")
      load_type = Type::getInt32Ty(*TheContext);
    else if(type=="bool")
      load_type = Type::getInt1Ty(*TheContext);
    else
      load_type = int8PtrTy;

    AllocaInst *alloca = function_allocas[from_function][name];

    return Builder->CreateLoad(load_type, alloca, name.c_str());
}

Value *float_llvm_hash(Value *float_value) {
    LLVMContext &C = Builder->getContext();
    // Bitcast float -> i32 (exact bit pattern)
    Value *bits = Builder->CreateBitCast(float_value, intTy, "float.bits");
    // bits ^= bits >> 16
    Value *x = bits;
    x = Builder->CreateXor(
        x,
        Builder->CreateLShr(x, const_int(16))
    );
    // bits *= 0x85ebca6b
    x = Builder->CreateMul(
        x,
        const_int(0x85ebca6bU)
    );
    // bits ^= bits >> 13
    x = Builder->CreateXor(
        x,
        Builder->CreateLShr(x, const_int(13))
    );
    // bits *= 0xc2b2ae35
    x = Builder->CreateMul(
        x,
        const_int(0xc2b2ae35U)
    );
    // bits ^= bits >> 16
    x = Builder->CreateXor(
        x,
        Builder->CreateLShr(x, const_int(16))
    );
    return x; // i32
}


Value *str_llvm_hash(Value *str_value, Function *F) {
    BasicBlock *startBB = BasicBlock::Create(*TheContext, "hash.start", F);
    BasicBlock *loopBB = BasicBlock::Create(*TheContext, "hash.loop", F);
    BasicBlock *exitBB = BasicBlock::Create(*TheContext, "hash.exit", F);
    BasicBlock *bodyBB = BasicBlock::Create(*TheContext, "hash.body", F);


    Builder->CreateBr(startBB);
    Builder->SetInsertPoint(startBB);

    Value *initHash = const_int(2166136261u); // FNV offset basis
    Value *initPtr  = str_value;


    Builder->CreateBr(loopBB);
    Builder->SetInsertPoint(loopBB);

    PHINode *phiHash = Builder->CreatePHI(intTy, 2);
    PHINode *phiPtr  = Builder->CreatePHI(int8PtrTy, 2);

    phiHash->addIncoming(initHash, startBB);
    phiPtr->addIncoming(initPtr,  startBB);

    Value *ch     = Builder->CreateLoad(int8Ty, phiPtr);
    Value *isZero = Builder->CreateICmpEQ(ch, Builder->getInt8(0));
    Builder->CreateCondBr(isZero, exitBB, bodyBB);

    /* ---- body ---- */
    Builder->SetInsertPoint(bodyBB);

    Value *ch32 = Builder->CreateZExt(ch, intTy);
    Value *h1   = Builder->CreateXor(phiHash, ch32);
    Value *h2   = Builder->CreateMul(h1, const_int(16777619u));
    Value *nextPtr = Builder->CreateGEP(int8Ty, phiPtr, const_int(1));

    phiHash->addIncoming(h2, bodyBB);
    phiPtr->addIncoming(nextPtr, bodyBB);


    Builder->CreateBr(loopBB);

    /* ---- exit ---- */
    Builder->SetInsertPoint(exitBB);
    return phiHash; // i32 == unsigned int
}

Value *str_view_llvm_hash(Value *str_value, Function *F) {
    BasicBlock *startBB = BasicBlock::Create(*TheContext, "hash.start", F);
    BasicBlock *loopBB  = BasicBlock::Create(*TheContext, "hash.loop", F);
    BasicBlock *bodyBB  = BasicBlock::Create(*TheContext, "hash.body", F);
    BasicBlock *exitBB  = BasicBlock::Create(*TheContext, "hash.exit", F);

    Builder->CreateBr(startBB);

    /* ---- start ---- */
    Builder->SetInsertPoint(startBB);

    // string_view.data
    Value *dataPtr = Builder->CreateExtractValue(str_value, {0});

    // string_view.size
    Value *sizeVal = Builder->CreateExtractValue(str_value, {1});

    Value *initHash = const_int(2166136261u);

    Builder->CreateBr(loopBB);

    /* ---- loop ---- */
    Builder->SetInsertPoint(loopBB);

    PHINode *phiHash = Builder->CreatePHI(intTy, 2);
    PHINode *phiPtr  = Builder->CreatePHI(int8PtrTy, 2);
    PHINode *phiIdx  = Builder->CreatePHI(intTy, 2);

    phiHash->addIncoming(initHash, startBB);
    phiPtr->addIncoming(dataPtr,  startBB);
    phiIdx->addIncoming(const_int(0), startBB);

    Value *done = Builder->CreateICmpEQ(phiIdx, sizeVal);
    Builder->CreateCondBr(done, exitBB, bodyBB);

    /* ---- body ---- */
    Builder->SetInsertPoint(bodyBB);

    Value *ch = Builder->CreateLoad(int8Ty, phiPtr);

    Value *ch32 = Builder->CreateZExt(ch, intTy);

    Value *h1 = Builder->CreateXor(phiHash, ch32);
    Value *h2 = Builder->CreateMul(h1, const_int(16777619u));

    Value *nextPtr = Builder->CreateGEP(int8Ty, phiPtr, const_int(1));
    Value *nextIdx = Builder->CreateAdd(phiIdx, const_int(1));

    phiHash->addIncoming(h2, bodyBB);
    phiPtr->addIncoming(nextPtr, bodyBB);
    phiIdx->addIncoming(nextIdx, bodyBB);

    Builder->CreateBr(loopBB);

    /* ---- exit ---- */
    Builder->SetInsertPoint(exitBB);

    return phiHash;
}



Function *getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = CurModule->getFunction(Name))
    return F;

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();

  // auto FP = FunctionUserProtos.find(Name);
  // if (FP != FunctionUserProtos.end())
  //   return FP->second->codegen();
  LogError(-1, "The function " + Name + " was not found.");

  // If no existing prototype exists, return null.
  return nullptr;
}



Value *ClassExprAST::codegen(Value *scope_struct) {

  return const_float(0);
}

Value *IndexExprAST::codegen(Value *scope_struct) {
  
  return const_float(0);
}


Value *CalcArrayIdx(Value *scope_struct, Value *vec, std::unique_ptr<ExprAST> &idx) {
    if (!idx)
        return Builder->CreateLoad(intTy, Builder->CreateStructGEP(struct_types["array"], vec, 0));

    if (auto *stmt = dynamic_cast<UnaryExprAST*>(idx.get())) {
        if (stmt->Opcode=='-') {
            Value *idx = stmt->Operand->codegen(scope_struct);  
            Value *arr_size = Builder->CreateLoad(intTy,
                                        Builder->CreateStructGEP(struct_types["array"], vec, 0));
            return Builder->CreateSub(arr_size, idx);
        }
    }
    return idx->codegen(scope_struct);
}

Value *Idx_Calc_Codegen(std::string type, Value *vec, std::unique_ptr<IndexExprAST> &idxs, Value *scope_struct) {
  auto &indices = idxs->Idxs;
  auto &first = indices[0].start; 
  if (!has_slice(indices)) {
    if (type=="array")
        return CalcArrayIdx(scope_struct, vec, first);
    
    if (first->GetDataTree().Type=="str")
        idxs->idx_slice_or_query = "query";
    return first->codegen(scope_struct); 
  }
  return const_int(0);

  // if (!idxs->IsSlice) {
  //   std::string fn = type+"_CalculateIdx";
  //   Function *F = CurModule->getFunction(fn);
  //   if (F)
  //     return callret(fn, idxs_values);
  // } else {
  //   for (int i=0; i<idxs->size(); i++)
  //     idxs_values.push_back(idxs->Second_Idxs[i]->codegen(scope_struct));
  //   std::string fn = type+"_CalculateSliceIdx";
  //   Function *F = CurModule->getFunction(fn);
  //   if (F)
  //     return callret(fn, idxs_values);
  //   else
  //     return callret("__sliced_idx__", idxs_values);
  // }
}


/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          StringRef VarName, Type *alloca_type) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                   TheFunction->getEntryBlock().begin());

  AllocaInst *alloca = TmpB.CreateAlloca(alloca_type, nullptr, VarName);

  return alloca;
}


void StoreAlloca(Function *TheFunction, std::string fn_name, std::string name, Value *val, Data_Tree dt) {
    AllocaInst *alloca;
    if (function_allocas[fn_name].count(name)==0) {
        alloca = CreateEntryBlockAlloca(TheFunction, name, get_type_from_data(dt));
        function_allocas[fn_name][name] = alloca;
    }
    else
        alloca = function_allocas[fn_name][name];
    Builder->CreateStore(val, alloca);
}
Value *LoadAlloca(std::string fn_name, std::string name, Data_Tree dt) {
    return Builder->CreateLoad(get_type_from_data(dt), function_allocas[fn_name][name]);
}
void StoreVal(Function *TheFunction, std::string fn_name, std::string name, Value *val, Data_Tree dt) {
    function_values[fn_name][name] = val;
    // AllocaInst *alloca = CreateEntryBlockAlloca(TheFunction, name, get_type_from_data(dt));
    // Builder->CreateStore(val, alloca);
    // function_allocas[fn_name][name] = alloca;
}
Value *LoadVal(std::string fn_name, std::string name, Data_Tree dt) {
    return function_values[fn_name][name];
    // return Builder->CreateLoad(get_type_from_data(dt), function_allocas[fn_name][name]);
}

Value *NumberExprAST::codegen(Value *scope_struct) {
  if (!ShallCodegen)
    return const_float(0.0f);


  return const_float(Val);
}

Value *IntExprAST::codegen(Value *scope_struct) {
  // if (!ShallCodegen)
  //   return const_int64(0);
  // return const_int64(Val);
  if (!ShallCodegen)
    return const_int(0);
  return const_int(Val);
}


Value *ConstExprAST::codegen(Value *scope_struct) {

  if (str=="float_t")
    return const_int(3);
  if (str=="char_t")
    return const_int(9);
  if (str=="i8_t")
    return const_int(6);
  if (str=="i16_t")
    return const_int(7);
  if (str=="int_t")
    return const_int(2);
  if (str=="i64_t")
    return const_int(8);
  if (str=="bool_t")
    return const_int(4);

  if (str=="str_t")
    return const_int(100);
  if (str=="array_t")
    return const_int(102);


  return const_int(0);
}


Value *BoolExprAST::codegen(Value *scope_struct) {
  if (!ShallCodegen)
    return const_bool(false); 
  return const_bool(Val);
}


Value *StringExprAST::codegen(Value *scope_struct) {
  if (not ShallCodegen)
    return const_float(0.0f);
  SetName(Val);
  Value *ptr = global_str(Val);
  Value *view_val = UndefValue::get(struct_types["DT_str"]);
  view_val = Builder->CreateInsertValue(view_val, ptr, {0});
  view_val = Builder->CreateInsertValue(view_val, const_int(Val.size()), {1});
  return view_val;
}

Value *CopyStrVal(Value *scope_struct, Value *DT_str_Val) {
    Value *str  = Builder->CreateExtractValue(DT_str_Val, {0});
    Value *size = Builder->CreateExtractValue(DT_str_Val, {1});

    Value *alloc_size = Builder->CreateAdd(size, const_int(1));

    // Value *str_copy = callret("allocate_pool", {scope_struct, alloc_size,
    //                             const_uint16(data_name_to_type()["charp"])});
    Value *str_copy = callret("malloc", {alloc_size});
    call("memcpy", {str_copy, str, size});

    // add \0
    Value *end_ptr = Builder->CreateGEP(int8Ty, str_copy, size);
    Builder->CreateStore(const_int8(0), end_ptr);

    return str_copy;
}


Value *CharExprAST::codegen(Value *scope_struct) {
  if (not ShallCodegen)
    return const_float(0.0f);
  return const_int8(Val);
}


Value *NullPtrExprAST::codegen(Value *scope_struct) { 
  return ConstantPointerNull::get(
            cast<PointerType>(int8PtrTy)
        );
}
Value *VarExprAST::codegen(Value *scope_struct) {
  return const_float(0);
}


Constant *make_lut(uint8_t table[16]) {
    std::vector<Constant*> vals;
    for (int i = 0; i < 32; i++) {
        vals.push_back(
            ConstantInt::get(
                Type::getInt8Ty(*TheContext),
                table[i & 15]
            )
        );
    }
    return ConstantVector::get(vals);
}
Value *LutLoExprAST::codegen(Value *scope_struct) {
    uint8_t lo_table[16] = {
        0,1,0,0,
        0,0,0,0,
        0,0,1,1,
        1,0,1,1
    };
    return make_lut(lo_table);
}
Value *LutHiExprAST::codegen(Value *scope_struct) {
    uint8_t hi_table[16] = {
        0,0,1,1,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0
    };
    return make_lut(hi_table);
}



llvm::Type *get_type_from_str(std::string type) {
  llvm::Type *llvm_type;
  if (str_toTy.count(type)>0)
      llvm_type = str_toTy[type];
  else
    llvm_type = int8PtrTy;
  return llvm_type;
}

bool is_type_array(Data_Tree dt) {
    return dt.Type=="charv";
}



StructType *tupleTy_of(const std::vector<llvm::Type*> &types) {
    return StructType::create(
        *TheContext,
        types,
        "tuple"
    ); 
}

bool Check_Is_Compatible_Data_Type(Data_Tree LType, Data_Tree RType, Parser_Struct parser_struct) {
  int differences = LType.Compare(RType);
  if (differences>0)
  {
    LogErrorS(parser_struct.line, "Tried to attribute data of different types");
    std::cout << "Left type:\n   ";
    LType.Print();
    std::cout << "\nRight type:\n   ";
    RType.Print();
    std::cout << "\n\n";
    return false;
  }
  return true;
}


inline bool Check_ArgsV_Count(const std::string &Callee, const std::vector<Value *> &ArgsV,
                              Parser_Struct parser_struct, int backend_args=1) {  
  Function *CalleeF;
  CalleeF = getFunction(Callee);
  
  if (!CalleeF)
  {
    std::string _error = "The referenced function "+ Callee +" was not yet declared.";
    LogErrorV(parser_struct.line, _error);
    return false;
  }

  if ((CalleeF->arg_size()) != ArgsV.size() && !in_str(Callee, vararg_methods))
  {
    std::string _error = "Incorrect parameters used on function " + Callee + " call.\n\t    Expected " +  std::to_string(CalleeF->arg_size()-backend_args) + " arguments, got " + std::to_string(ArgsV.size()-backend_args);
    LogErrorV(parser_struct.line, _error);
    return false;
  }

  return true;
}

inline bool Check_Required_Args_Count(const std::string &Callee, int sent_args,
                              Parser_Struct parser_struct, int backend_args=1) {  
  if(Function_Required_Arg_Count.count(Callee)==0||in_str(Callee, vararg_methods))
    return true;

  if (sent_args<Function_Required_Arg_Count[Callee]) {
      // LogErrorS(parser_struct.line, "Sent " + std::to_string(sent_args) + " into function " + Callee + ", but the function requires at least "+\
      //                               std::to_string(Function_Required_Arg_Count[Callee]) + " arguments.");
      return false;
  }

  return true;
}

// void Cache_Array(Parser_Struct parser_struct, Value *var) {
//     Value *vec_gep = Builder->CreateStructGEP(struct_types["array"], var, 3);
//     function_vecs[parser_struct.function_name][var] = vec_gep;
// }

inline Value *Load_Array(std::string &function_name, Value *var) {
    Value *vec_gep;
    // if (function_vecs[function_name].count(var)==0) { // handles self.array
    //     vec_gep = Builder->CreateStructGEP(struct_types["array"], var, 3);
    //     function_vecs[function_name][var] = vec_gep;
    // } else
    //     vec_gep = function_vecs[function_name][var];
        vec_gep = Builder->CreateStructGEP(struct_types["array"], var, 3);
    return Builder->CreateLoad(int8PtrTy, vec_gep);
}

inline void Check_Is_Array_Inbounds(Parser_Struct &parser_struct, Value *var, Value *idx) {
    // return;
    Value *vec_gep = Builder->CreateStructGEP(struct_types["array"], var, 0);
    Value *size = Builder->CreateLoad(intTy, vec_gep);
    // Value *size = const_int(1000000);

    Value *in_bounds = Builder->CreateICmpSLT(idx, size);

    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    BasicBlock *okBB = BasicBlock::Create(*TheContext, "idx.ok", TheFunction);
    BasicBlock *bad_sizeBB = BasicBlock::Create(*TheContext, "idx.bad_size", TheFunction);

    Builder->CreateCondBr(in_bounds, okBB, bad_sizeBB);

    Builder->SetInsertPoint(bad_sizeBB);
    call("array_bad_idx", {const_int(parser_struct.line), idx, size});
    Builder->CreateUnreachable();

    Builder->SetInsertPoint(okBB);
}



inline Value *swap_scope_obj(Value *scope_struct, Value *obj) {
    StructType *st = struct_types["scope_struct"];
    Value *obj_gep = Builder->CreateStructGEP(st, scope_struct, 4);
    Value *previous_obj = Builder->CreateLoad(int8PtrTy, obj_gep);
    Builder->CreateStore(obj, obj_gep);
    return previous_obj;
}

inline void set_scope_obj(Value *scope_struct, Value *obj) {
    StructType *st = struct_types["scope_struct"];
    Value *obj_gep = Builder->CreateStructGEP(st, scope_struct, 4);
    Builder->CreateStore(obj, obj_gep);
    cur_self = nullptr;
}

inline Value *get_scope_obj(Value *scope_struct) {
    // if (!cur_self) {
        StructType *st = struct_types["scope_struct"]; 
        Value *obj_gep = Builder->CreateStructGEP(st, scope_struct, 4);
        cur_self = Builder->CreateLoad(int8PtrTy, obj_gep);
        // Value *obj_gep = Builder->CreateStructGEP(st, scope_struct, 4);
    // }
    return cur_self;
    // return Builder->CreateLoad(int8PtrTy, cur_self);
}

void check_scope_struct_sweep(Function *TheFunction, Value *scope_struct, const Parser_Struct &parser_struct) {
  return;
  
  Value *GC_ptr = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 5, "GC_ptr_element");
  Value *GC = Builder->CreateLoad(struct_types["GC"]->getPointerTo(), GC_ptr, "GC");

  Value *GC_allocations_ptr = Builder->CreateStructGEP(struct_types["GC"], GC, 0, "GC_allocations_ptr");
  Value *GC_allocations = Builder->CreateLoad(intTy, GC_allocations_ptr, "GC_allocations");

  Value *GC_size_alloc_ptr = Builder->CreateStructGEP(struct_types["GC"], GC, 1, "GC_allocations_ptr");
  Value *GC_size_alloc = Builder->CreateLoad(Type::getInt64Ty(*TheContext), GC_size_alloc_ptr, "GC_allocations");

  // // Compare GC_allocations > 1000
  // Value *cmp_alloc = Builder->CreateICmpSGT(
  //     GC_allocations,
  //     ConstantInt::get(Type::getInt32Ty(*TheContext), 1000),
  //     "cmp_alloc"
  // );
  // Compare GC_size_alloc > 10000

  Value *cmp_size = Builder->CreateICmpUGT(  // unsigned since uint64_t
      GC_size_alloc,
      ConstantInt::get(Type::getInt64Ty(*TheContext), sweep_after_alloc),
      "cmp_size"
  );
  // Value *sweep_cond = Builder->CreateOr(cmp_alloc, cmp_size);
  Value *sweep_cond = cmp_size;

  BasicBlock *SweepThenBB = BasicBlock::Create(*TheContext, "sweep_then", TheFunction);
  BasicBlock *SweepContinueBB = BasicBlock::Create(*TheContext, "sweep_continue", TheFunction);

  Builder->CreateCondBr(sweep_cond, SweepThenBB, SweepContinueBB);

  Builder->SetInsertPoint(SweepThenBB);

  Set_Stack_Top(scope_struct, parser_struct.function_name);
  call("scope_struct_Sweep", {scope_struct});
  Builder->CreateBr(SweepContinueBB);

  Builder->SetInsertPoint(SweepContinueBB);
}


void MakeWriteBarrier(Parser_Struct parser_struct, Function *TheFunction, Value *scope_struct,
                        Value *src, Value *ptr, Value *ref,
                        std::string type) {
    Value *gc_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 5);
    Value *gc = Builder->CreateLoad(int8PtrTy, gc_gep);
    Value *marking = Builder->CreateLoad(boolTy,
                                Builder->CreateStructGEP(struct_types["GC"], gc, 3));

    BasicBlock *MarkingBB = BasicBlock::Create(*TheContext, "array.marking", TheFunction);
    BasicBlock *StandardBB = BasicBlock::Create(*TheContext, "array.standard", TheFunction);
    BasicBlock *PostBB = BasicBlock::Create(*TheContext, "array.standard", TheFunction);
    
    Builder->CreateCondBr(marking, MarkingBB, StandardBB);
    Builder->SetInsertPoint(MarkingBB);
    if (type=="str") {
        Value *str_v = Builder->CreateExtractValue(ref, {0});
        Value *size = Builder->CreateExtractValue(ref, {1});
        call("GC_write_barrier_str", {const_int16(data_name_to_type()[type]), gc, src, ptr, str_v, size});
    }
    else
        call("GC_write_barrier_obj", {const_int16(data_name_to_type()[type]), gc, src, ptr, ref});
    Builder->CreateBr(PostBB);

    Builder->SetInsertPoint(StandardBB);
    Builder->CreateStore(ref, ptr);
    Builder->CreateBr(PostBB);

    Builder->SetInsertPoint(PostBB);
}


Value *UnkVarExprAST::codegen(Value *scope_struct) {
  if (not ShallCodegen)
    return const_float(0.0f);

  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  // Register all variables and emit their initializer.
  for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
    const std::string &VarName = VarNames[i].first; 
    ExprAST *Init = VarNames[i].second.get();

    if (dynamic_cast<NullPtrExprAST*>(Init))
      continue; 

    bool is_self = GetSelf();
    bool is_attr = GetIsAttribute();

    Value *initial_value = Init->codegen(scope_struct);

      
    Data_Tree init_dt = data_typeVars[parser_struct.function_name][VarName]; 
    Type = init_dt.Type;


    if(Init->GetIsMsg()) {
      Value *void_ptr = Constant::getNullValue(int8PtrTy);
      initial_value = callret(Type+"_channel_message", {scope_struct, void_ptr, initial_value});
    }




    if((in_vec(Type, primary_data_tokens)||Type=="layout")&&!(is_self||is_attr)) { 
      StoreVal(TheFunction, parser_struct.function_name, VarName, initial_value, init_dt);
      continue;
    }


 
    


    if(is_self) {
      int object_ptr_offset = ClassVariables[parser_struct.class_name][VarName]; 
  
      Value *obj = get_scope_obj(scope_struct);
      call("object_ptr_Attribute_object", {obj, const_int(object_ptr_offset), initial_value});

    } else if (is_attr) {
      LogErrorS(parser_struct.line, "Creating attribute in a data expression is not supported.");
    }
    else {
      Value *unpacked_val = initial_value;
      if (Type=="str") {
            unpacked_val = Builder->CreateExtractValue(initial_value, {0});
      }
      StoreVal(TheFunction, parser_struct.function_name, VarName, initial_value, init_dt);
      Allocate_On_Pointer_Stack(scope_struct, parser_struct.function_name, VarName, data_type, unpacked_val); 
    }
      

  }


  return const_float(0.0f);
}




Value *ListExprAST::codegen(Value *scope_struct) {
}
Value *DictExprAST::codegen(Value *scope_struct) {
}

Value *TupleExprAST::codegen(Value *scope_struct) {
  if (not ShallCodegen)
    return ConstantFP::get(*TheContext, APFloat(0.0f));
  Function *TheFunction = Builder->GetInsertBlock()->getParent();


  // Register all variables and emit their initializer.
  for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
    const std::string &VarName = VarNames[i].first; 
    ExprAST *Init = VarNames[i].second.get();
        
    
    Value *initial_value = Init->codegen(scope_struct);
    data_type.Print();
    
    StoreVal(TheFunction, parser_struct.function_name, VarName, initial_value, data_type);
  }

  return ConstantFP::get(*TheContext, APFloat(0.0));
}

Value *Malloc_LLVM_Struct(Value *scope_struct, std::string &struct_name, std::string &type) {
    StructType *st = struct_types[struct_name];
    DataLayout DL(CurModule);
    uint64_t size = DL.getTypeAllocSize(st);


    // call malloc
    // Value *rawPtr = callret("malloc", {sizeV});
    //
    Value *rawPtr = callret("allocate_void", {scope_struct, const_int(size), global_str(type)});

    // cast to DT_vec*
    Value *ptr = Builder->CreateBitCast(rawPtr, PointerType::getUnqual(st));
    return ptr;
}

Value *Load_Stack_Top(std::string fn) {
    Value *stack_top_value = function_values[fn]["QQ_stack_top"];
    Value *st = Builder->CreateAdd(stack_top_value, const_int(fn_stack_offset[fn]));
    return st;
}
void Set_Stack_Top(Value *scope_struct, std::string fn) {
    Value *stack_top_value_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 3);
    Builder->CreateStore(Load_Stack_Top(fn), stack_top_value_gep);
}


void Allocate_On_Pointer_Stack(Value *scope_struct, std::string function_name,
        std::string var_name, Data_Tree dt, Value *val) {

    Value *stack_top_value = Load_Stack_Top(function_name);
    function_pointers[function_name][var_name] = stack_top_value;

    Value *stack_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 2);

    Value *void_ptr_gep = Builder->CreateGEP(ArrayType::get(int8PtrTy, ContextStackSize), stack_gep, { const_int(0), stack_top_value });
    
    Builder->CreateStore(stack_top_value,
                         Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 3));

    Builder->CreateStore(val, void_ptr_gep);
    // std::cout << function_name <<  " - " << var_name << ": " << fn_stack_offset[function_name] << "\n";
    fn_stack_offset[function_name]++;
}

void Allocate_On_Pointer_Stack_no_metadata(Value *scope_struct, std::string function_name, Value *val) {
    Value *stack_top_value = Load_Stack_Top(function_name);
    // Prevents the function_pointer from being used inside another function (invalid Value *).
    
    // pointer to [N x i8*]
    Value *stack_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 2);

    // element pointer: &pointers_stack[i]
    // {0, i} first index the array object, then the element
    Value *void_ptr_gep = Builder->CreateGEP(ArrayType::get(int8PtrTy, ContextStackSize), stack_gep, { const_int(0), stack_top_value });
    Builder->CreateStore(val, void_ptr_gep);
    
    Builder->CreateStore(stack_top_value,
                         Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 3));

    fn_stack_offset[function_name]++;
}

Value *Load_Pointer_Stack(Value *scope_struct, std::string function_name, std::string var_name) {
    Value *stack_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 2);
    Value *stack_idx = function_pointers[function_name][var_name];

    Value *void_ptr_gep = Builder->CreateGEP(ArrayType::get(int8PtrTy, ContextStackSize), stack_gep, { const_int(0), stack_idx });
    return Builder->CreateLoad(int8PtrTy, void_ptr_gep);
}

void Set_Pointer_Stack(Value *scope_struct, std::string function_name, std::string var_name, Value *val) {
    // p2t("SET " + var_name);
    // call("print_void_ptr", {val});
    if (function_pointers[function_name].count(var_name)==0) {
        Allocate_On_Pointer_Stack(scope_struct, function_name, var_name, Data_Tree("aux"), val);
        return;
    }
    Value *stack_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 2);
    Value *stack_idx = function_pointers[function_name][var_name];

    Value *void_ptr_gep = Builder->CreateGEP(ArrayType::get(int8PtrTy, ContextStackSize), stack_gep, { const_int(0), stack_idx });
    Builder->CreateStore(val, void_ptr_gep);
}


inline std::vector<Value *> Codegen_Argument_List(Parser_Struct parser_struct,
                                                  std::vector<Value *> ArgsV,
                                                  std::vector<std::unique_ptr<ExprAST>> &Args,
                                                  std::vector<Data_Tree> &ArgTypes,
                                                  Value *scope_struct, std::string fn_name,
                                                  bool is_nsk_fn, int arg_offset=1) {
  // -- Required Arguments -- //
  unsigned i, e;
  for (i = 0, e = Args.size(); i != e; ++i) {
    if (dynamic_cast<PositionalArgExprAST*>(Args[i].get()))
        break;
     
    Value *arg = Args[i]->codegen(scope_struct);
 

    Data_Tree data_type = Args[i]->GetDataTree();
    std::string type = data_type.Type;

    ArgTypes.push_back(data_type);

    
    int tgt_arg = i + arg_offset;
    Data_Tree expected_data_type = Function_Arg_DataTypes[fn_name][Function_Arg_Names[fn_name][tgt_arg]];
    std::string expected_type = expected_data_type.Type;



    // casts   
    if(type=="int"&&expected_type=="float")
      arg = Builder->CreateSIToFP(arg, floatTy, "lfp");
    if (type!=expected_type&&in_vec(type, int_types)&&in_vec(expected_type, int_types))
        arg = Builder->CreateIntCast(arg, get_type_from_data(expected_data_type), /*isSigned=*/true);

    if (!is_nsk_fn&&data_type.is_array) {
        llvm::Type *Ty = get_type_from_data(data_type);
        arg = Builder->CreateGEP(Ty, arg, \
                                 {const_int(0), const_int(0)});
    }



    std::string copy_fn = type+"_CopyArg";
    Function *F = CurModule->getFunction(copy_fn);
    if (F&&!is_nsk_fn) {
        std::cout << "Copy of " << type << "\n";
        Value *copied_value = callret(copy_fn,
                        {scope_struct,
                        arg,
                        global_str("-")});   
      arg = copied_value;
    }

    ArgsV.push_back(arg);

    if (!is_nsk_fn && !in_vec(type, primary_data_tokens)) {
        // If it creates a new memory address for a high-level fn, store address on the stack.
        bool does_op_create_memory = \
            (F||dynamic_cast<UnaryExprAST*>(Args[i].get())\
              ||dynamic_cast<NameableCall*>(Args[i].get()));
        if (auto *stmt = dynamic_cast<BinaryExprAST*>(Args[i].get()))
            does_op_create_memory = (does_op_create_memory||stmt->Op!=tok_offby);

        if (does_op_create_memory) {
            Allocate_On_Pointer_Stack_no_metadata(scope_struct, parser_struct.function_name, arg);
            Set_Stack_Top(scope_struct, parser_struct.function_name);
        }
    }


    if (!ArgsV.back()) {
      LogErrorS(parser_struct.line, "Failed to codegen argument of function " + fn_name);
      return {};
    }

  }

  i = i + arg_offset-1;
  if (gpu_fn.count(fn_name)>0)
      i++;

  // -- Add Default Arguments -- //
  if (Function_Arg_Count.count(fn_name)>0&&!in_vec(fn_name, vararg_methods)) {
      int arg_count = Function_Arg_Count[fn_name];

      int c=i+1;
      
      std::vector<std::string> fn_args_name = Function_Arg_Names[fn_name];
      for (; i<Args.size(); ++i, ++c) { // Positional Arguments
          auto PosArg = dynamic_cast<PositionalArgExprAST*>(Args[i].get());
          if(!PosArg) {
            LogErrorS(parser_struct.line, "Standard argument followed by positional argument.");
            return std::move(ArgsV);
          }



          std::string arg_name = PosArg->ArgName;

        
          auto it = std::find(fn_args_name.begin(), fn_args_name.end(), arg_name);
          int arg_idx = it-fn_args_name.begin();

          for (; c<arg_idx; ++c) {
              std::string arg_name = Function_Arg_Names[fn_name][c];
              Value *arg_default = ArgsInit[fn_name][arg_name]->codegen(scope_struct);
              ArgsV.push_back(arg_default);
          }

        
          ArgsV.push_back(Args[i]->codegen(scope_struct));
      }

      for (; i<arg_count; ++i) {
          std::string arg_name = fn_args_name[i+1];
          Value *arg_default = ArgsInit[fn_name][arg_name]->codegen(scope_struct);
          ArgsV.push_back(arg_default);
      }
  }


  if (fn_name=="list_append") {
    std::string appended_type = UnmangleVec(Args[Args.size()-1]->GetDataTree());
    ArgsV.push_back(global_str(appended_type));
  }

  return std::move(ArgsV);
}








Value *DataExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return const_float(0.0f);

    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    // Register all variables and emit their initializer.
    for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
        const std::string &VarName = VarNames[i].first; 
        ExprAST *Init = VarNames[i].second.get();


        bool is_self = GetSelf();
        bool is_attr = GetIsAttribute();



        Value *initial_value = Init->codegen(scope_struct);
        Data_Tree init_dt = Init->GetDataTree();
        std::string init_type = init_dt.Type;




        if(Init->GetIsMsg()) {
            Value *void_ptr = Constant::getNullValue(int8PtrTy);
            if (!in_vec(Type, primary_data_tokens)&&Type!="str")
                initial_value = callret("void_channel_message", {scope_struct, void_ptr, initial_value});
            else
                initial_value = callret(Type+"_channel_message", {scope_struct, void_ptr, initial_value});
        }



        if (data_type.is_array && !(is_self||is_attr)) { 
            if (auto *null_stmt = dynamic_cast<NullPtrExprAST*>(VarNames[i].second.get())) {
                AllocaInst *alloca = CreateEntryBlockAlloca(TheFunction, VarName, \
                                                get_type_from_data(data_type));
                function_allocas[parser_struct.function_name][VarName] = alloca;
                continue;
            } 
        }


        if(in_vec(Type, primary_data_tokens) && !data_type.is_buffer && !(is_self||is_attr)) { 
            if (Type=="bool"&&init_type=="any")
                initial_value = callret("to_bool", {scope_struct, initial_value}); 
            if (Type=="int"&&init_type=="any")
                initial_value = callret("to_int", {scope_struct, initial_value}); 
            if (Type=="float"&&init_type=="any")
                initial_value = callret("to_float", {scope_struct, initial_value});

            if (Type=="float"&&init_dt.Type=="int")
                initial_value = Builder->CreateSIToFP(initial_value, floatTy, "int_to_float");
            if (Type!=init_dt.Type&&in_vec(Type, int_types)&&in_vec(init_dt.Type, int_types))
                initial_value = Builder->CreateIntCast(initial_value,
                                        get_type_from_data(data_type), true);
            StoreVal(TheFunction, parser_struct.function_name, VarName, initial_value, init_dt);
            continue;
        }




        if(DtHasCreateFn) {
            if (auto *null_stmt = dynamic_cast<NullPtrExprAST*>(VarNames[i].second.get())) {
                if(Check_Required_Args_Count(create_fn, Notes.size(), parser_struct)) {

                    std::vector<Value *> ArgsV = {scope_struct};

                    if (create_fn=="array_Create" || create_fn=="map_Create" || create_fn=="channel_Create") {
                        if (create_fn=="array_Create")
                            ArgsV.push_back(const_int16(data_name_to_type()[data_type.Nested_Data[0].Type]));
                        else if (create_fn=="channel_Create") {
                            ArgsV.push_back(const_int16(data_name_to_type()[data_type.Nested_Data[0].Type]));
                            ArgsV.push_back(const_int(std::stoi(data_type.Nested_Data[1].Type)));
                        }
                        else {
                            Data_Tree *dt = new Data_Tree(data_type.Type);
                            dt->Nested_Data.push_back(data_type.Nested_Data[0]);
                            if(create_fn=="map_Create")
                                dt->Nested_Data.push_back(data_type.Nested_Data[1]);
                            ArgsV.push_back(VoidPtr_toValue(dt));
                        }
                        
                    }
                    else {
                        std::vector<Data_Tree> ArgTypes;
                        ArgsV = Codegen_Argument_List(parser_struct, std::move(ArgsV), Notes, ArgTypes,
                                                      scope_struct, create_fn, true, 1);
                        if(in_vec(create_fn, vararg_methods))
                            ArgsV.push_back(const_int(TERMINATE_VARARG));

                    }

                    if(struct_create_fn.count(dt_type)>0) {
                        initial_value = (struct_type_size.count(Type)>0) ? callret("allocate_pool", {scope_struct,
                                    const_int(struct_type_size[Type]),
                                    const_int16(data_name_to_type()[Type])}) : nullptr;
                        std::vector<Value*> ArgsV_slice(ArgsV.begin()+1, ArgsV.end()); // skip ctx
                        initial_value = struct_create_fn[dt_type](parser_struct, TheFunction,
                                VarName, Type, data_type,
                                scope_struct, 
                                initial_value,
                                Notes,
                                ArgsV_slice);
                    }
                    else
                        initial_value = callret(create_fn, ArgsV);
                    
                }
            }
        }


        if(is_self) {
            int object_ptr_offset = ClassVariables[parser_struct.class_name][VarName]; 
            Value *obj = get_scope_obj(scope_struct);
            call("object_ptr_Attribute_object", {obj, const_int(object_ptr_offset), initial_value});
        } else if (is_attr) {
            LogErrorS(parser_struct.line, "Creating attribute in a data expression is not supported.");
        }
        else { 
            Value *unpacked_val = initial_value;
            if (Type=="str")
                unpacked_val = Builder->CreateExtractValue(initial_value, {0});

            if (parser_struct.gpu==0)
                Allocate_On_Pointer_Stack(scope_struct, parser_struct.function_name, VarName, data_type, unpacked_val); 

            StoreVal(TheFunction, parser_struct.function_name, VarName, initial_value, init_dt);
        }
    }
    return const_float(0.0f);
}



Value *LibImportExprAST::codegen(Value *scope_struct) {
    // Library import is made before codegen

    return const_float(0.0f);
}



Value *GCSafePointExprAST::codegen(Value *scope_struct) {
    if (parser_struct.gpu>0)
        return const_float(0.0f);
    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    Set_Stack_Top(scope_struct, parser_struct.function_name);
    check_scope_struct_sweep(TheFunction, scope_struct, parser_struct);
    return const_float(0.0f);
}



Value *IfExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return const_float(0.0f);


    Value *CondV = Cond->codegen(scope_struct);
    if (!CondV)
        return nullptr;


    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    BasicBlock *ThenPostBB, *ElsePostBB;
    BasicBlock *ThenBB  = BasicBlock::Create(*TheContext, "if_then", TheFunction);
    BasicBlock *ElseBB  = BasicBlock::Create(*TheContext, "if_else");
    BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "if_cont");

    std::string cond_type = Cond->GetDataTree().Type;

    // handle pointer type
    if (cond_type!="bool" && !in_vec(cond_type, primary_data_tokens)&&!in_vec(cond_type, compound_tokens))
        CondV = Builder->CreateICmpNE(CondV,
                        ConstantPointerNull::get(
                            cast<PointerType>(int8PtrTy)
                        ));

    Builder->CreateCondBr(CondV, ThenBB, ElseBB);
    Builder->SetInsertPoint(ThenBB);

    auto old_allocas = function_allocas[parser_struct.function_name];
    auto old_values = function_values[parser_struct.function_name];


    Value *ThenV;
    for (auto &then_body : Then)
        ThenV = then_body->codegen(scope_struct);
    ThenPostBB = Builder->GetInsertBlock();
    block_values[ThenPostBB] = function_values[parser_struct.function_name];


    bool ThenTerminated = Builder->GetInsertBlock()->getTerminator() != nullptr;
    if (!ThenV)
        return nullptr;
    if (!ThenTerminated) {
        ThenPostBB = Builder->GetInsertBlock();
        block_values[ThenPostBB] = function_values[parser_struct.function_name];
        Builder->CreateBr(MergeBB);
    }

    // Emit else block.
    TheFunction->insert(TheFunction->end(), ElseBB);
    Builder->SetInsertPoint(ElseBB);

    function_values[parser_struct.function_name] = old_values;
    function_allocas[parser_struct.function_name] = old_allocas;

    Value *ElseV;
    for (auto &else_body : Else)
        ElseV = else_body->codegen(scope_struct);
    ElsePostBB = Builder->GetInsertBlock();

    if(Else.size()>0)
        block_values[ElsePostBB] = function_values[parser_struct.function_name];
    else {
        ElseV = const_int(0);
        block_values[ElsePostBB] = old_values;
    }

    bool ElseTerminated = Builder->GetInsertBlock()->getTerminator() != nullptr;
    if (!ElseV)
        return nullptr;
    if (!ElseTerminated)
        Builder->CreateBr(MergeBB);


    if (ThenTerminated && ElseTerminated)
        return nullptr;

    // Emit merge block.
    TheFunction->insert(TheFunction->end(), MergeBB);
    Builder->SetInsertPoint(MergeBB);

    auto then_values = block_values[ThenPostBB];
    auto else_values = block_values[ElsePostBB];
    for (auto &[name, value] : old_values) {
        if (!ThenTerminated&&!ElseTerminated) {
            if (then_values[name]!=value || else_values[name]!=value) {
                PHINode *phi = Builder->CreatePHI(value->getType(), 2);
                phi->addIncoming(then_values[name], ThenPostBB);
                phi->addIncoming(else_values[name], ElsePostBB);
                function_values[parser_struct.function_name][name] = phi;
            }
        } else if (!ThenTerminated)
            function_values[parser_struct.function_name][name] = then_values[name];
        else if (!ElseTerminated)
            function_values[parser_struct.function_name][name] = else_values[name];
        else {}
    }
    return const_float(0.0f);
}


Value *IfExprAST::codegen_from_loop(Value *scope_struct,
        BasicBlock *LoopBB, BasicBlock *IncBB, BasicBlock *LoopAfter,
        std::map<std::string, Value*> &break_values_snapshot,
        std::vector<BasicBlock *> &BreakBB, std::vector<BasicBlock *> &ContinueBB) {
    if (not ShallCodegen)
        return const_float(0.0f);


    Value *CondV = Cond->codegen(scope_struct);
    if (!CondV)
        return nullptr;



    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.

    BasicBlock *ThenBB  = BasicBlock::Create(*TheContext, "if.then", TheFunction);
    BasicBlock *ElseBB  = BasicBlock::Create(*TheContext, "if.else");
    BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "if.after");
    BasicBlock *ThenPostBB, *ElsePostBB;


    Builder->CreateCondBr(CondV, ThenBB, ElseBB);
    Builder->SetInsertPoint(ThenBB);

    auto old_values = function_values[parser_struct.function_name];


    Value *ThenV;
    for (auto &then_body : Then) {
        if (auto *if_stmt = dynamic_cast<IfExprAST*>(then_body.get()))
            if_stmt->codegen_from_loop(scope_struct,
                    ThenBB, IncBB, LoopAfter, break_values_snapshot, BreakBB, ContinueBB);
        else if (auto *break_stmt = dynamic_cast<BreakExprAST*>(then_body.get())) {
            break_values_snapshot = function_values[parser_struct.function_name];
            auto bb = Builder->GetInsertBlock();
            block_values[bb] = function_values[parser_struct.function_name];
            BreakBB.push_back(bb);
            Builder->CreateBr(LoopAfter);
        }
        else if (auto *stmt = dynamic_cast<ContinueExprAST*>(then_body.get())) {
            auto bb = Builder->GetInsertBlock();
            block_values[bb] = function_values[parser_struct.function_name];
            ContinueBB.push_back(bb);
            Builder->CreateBr(IncBB);
        }
        else
            ThenV = then_body->codegen(scope_struct);
    }
    ThenPostBB = Builder->GetInsertBlock();
    block_values[ThenPostBB] = function_values[parser_struct.function_name];


    bool ThenTerminated = Builder->GetInsertBlock()->getTerminator() != nullptr;
    if (!ThenV)
        return nullptr;
    if (!ThenTerminated)
        Builder->CreateBr(MergeBB);

    // Emit else block.
    TheFunction->insert(TheFunction->end(), ElseBB);
    Builder->SetInsertPoint(ElseBB);


    function_values[parser_struct.function_name] = old_values;
    Value *ElseV;
    for (auto &else_body : Else)
        ElseV = else_body->codegen(scope_struct);
    ElsePostBB = Builder->GetInsertBlock();

    if(Else.size()>0)
        block_values[ElsePostBB] = function_values[parser_struct.function_name];
    else {
        ElseV = const_int(0);
        block_values[ElsePostBB] = old_values;
    }

    bool ElseTerminated = Builder->GetInsertBlock()->getTerminator() != nullptr;
    if (!ElseV)
        return nullptr;
    if (!ElseTerminated)
        Builder->CreateBr(MergeBB);


    if (ThenTerminated && ElseTerminated)
        return nullptr;

    // Emit merge block.
    TheFunction->insert(TheFunction->end(), MergeBB);
    Builder->SetInsertPoint(MergeBB);


    auto then_values = block_values[ThenPostBB];
    auto else_values = block_values[ElsePostBB];
    for (auto &[name, value] : old_values) {
        if (!ThenTerminated&&!ElseTerminated) {
            if (then_values[name]!=value || else_values[name]!=value) {
                PHINode *phi = Builder->CreatePHI(value->getType(), 2);
                phi->addIncoming(then_values[name], ThenPostBB);
                phi->addIncoming(else_values[name], ElsePostBB);
                function_values[parser_struct.function_name][name] = phi;
            }
        } else if (!ThenTerminated)
            function_values[parser_struct.function_name][name] = then_values[name];
        else if (!ElseTerminated)
            function_values[parser_struct.function_name][name] = else_values[name];
        else {}
    }

    return const_float(0.0f);
}







// Output for-loop as:
//   var = alloca float
//   ...
//   start = startexpr
//   store start -> var
//   goto loop
// loop:
//   ...
//   bodyexpr
//   ...
// loopend:
//   step = stepexpr
//   endcond = endexpr
//
//   curvar = load var
//   nextvar = curvar + step
//   store nextvar -> var
//   br endcond, loop, endloop
// outloop:



Value *BreakExprAST::codegen(Value *scope_struct) { 
    p2t("break");
    return const_int(0);
}
Value *ContinueExprAST::codegen(Value *scope_struct) { 
    return const_int(0);
}


void SetBreakPHIS(Parser_Struct parser_struct, std::vector<std::string> &assigned_vars, 
            std::map<std::string, Value*> &old_values,
            std::map<std::string, PHINode*> &function_phi_values,
            std::vector<BasicBlock *> &BreakBB, BasicBlock *LoopPredecessor) {
    for (auto &bb : BreakBB) {
        auto break_values_snapshot = block_values[bb];
        for (const auto &pair : break_values_snapshot) {
            std::string name = pair.first;
            bool existed_already = in_vec(pair.first, assigned_vars);

            if (existed_already) {
                bool was_created = old_values.count(name)==0;
                if (was_created)
                    continue;
                PHINode *phi_val = Builder->CreatePHI(pair.second->getType(), 2, name.c_str());

                phi_val->addIncoming(function_phi_values[name], LoopPredecessor);
                phi_val->addIncoming(pair.second, bb);

                function_values[parser_struct.function_name][name] = phi_val;
            }
        }
    }
}

void Codegen_Loop_Body(Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> Body,
        BasicBlock *LoopBB, BasicBlock *IncBB, BasicBlock *AfterBB,
        std::map<std::string, Value*> &break_values_snapshot,
        std::vector<BasicBlock *> &BreakBB, std::vector<BasicBlock *> &ContinueBB) {  
    // Handle break stmt
    
    for (auto &body : Body) {
        if (auto *if_stmt = dynamic_cast<IfExprAST*>(body.get()))
            if_stmt->codegen_from_loop(scope_struct, LoopBB, IncBB, AfterBB, break_values_snapshot, BreakBB, ContinueBB);
        else
            body->codegen(scope_struct);
    }
}

void Get_Recursive_Assign_Statements(const std::vector<std::unique_ptr<ExprAST>> &stmt, std::vector<std::string> &assigned_vars) {

    for (auto &body : stmt) {
        if (auto *if_stmt = dynamic_cast<IfExprAST*>(body.get())) {
            Get_Recursive_Assign_Statements(if_stmt->Then, assigned_vars);
            Get_Recursive_Assign_Statements(if_stmt->Else, assigned_vars);
        }
        if (auto *while_stmt = dynamic_cast<WhileExprAST*>(body.get()))
            Get_Recursive_Assign_Statements(while_stmt->Body, assigned_vars);
        if (auto *while_stmt = dynamic_cast<ForExprAST*>(body.get()))
            Get_Recursive_Assign_Statements(while_stmt->Body, assigned_vars);
        if (auto *while_stmt = dynamic_cast<ForEachExprAST*>(body.get()))
            Get_Recursive_Assign_Statements(while_stmt->Body, assigned_vars);
        
        if (auto *bin_stmt = dynamic_cast<BinaryExprAST*>(body.get())) {
            char op = bin_stmt->Op;
            bool is_sugar = bin_stmt->is_store_sugar;
            if (op=='='||op==tok_arrow||is_sugar) {
                if (auto *nameable_stmt = dynamic_cast<Nameable*>(bin_stmt->LHS.get())) {
                    if(typeid(*nameable_stmt)==typeid(Nameable)) {
                        if (nameable_stmt->Depth==1) {
                            const std::string &var_name = nameable_stmt->GetName();
                            if (!in_vec(var_name, assigned_vars))
                                assigned_vars.push_back(var_name);
                        }
                    }
                }
            }
        }
    }

}


Value *ForExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return const_float(0);

    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    check_scope_struct_sweep(TheFunction, scope_struct, parser_struct);

    Data_Tree start_dt = Start->GetDataTree();
    std::string start_type = start_dt.Type;
    llvm::Type *llvm_type = get_type_from_data(start_type);


    Value *StartVal = Start->codegen(scope_struct);
    if (!StartVal)
        return nullptr;


    // Make the new basic block for the loop header, inserting after current
    // block.
    BasicBlock *CondBB = BasicBlock::Create(*TheContext, "for.cond", TheFunction);
    BasicBlock *LoopBB  = BasicBlock::Create(*TheContext, "for.loop");
    BasicBlock *IncBB  = BasicBlock::Create(*TheContext, "for.inc");
    BasicBlock *AfterBB  = BasicBlock::Create(*TheContext, "for.after");
    BasicBlock *PreheaderBB = Builder->GetInsertBlock();

    auto old_allocas = function_allocas[parser_struct.function_name];
    block_values[PreheaderBB] = function_values[parser_struct.function_name];

    Builder->CreateBr(CondBB);
    Builder->SetInsertPoint(CondBB);


    std::vector<std::string> assigned_vars, changed_vars;
    Get_Recursive_Assign_Statements(Body, assigned_vars);

    // Possible phi for each value
    auto old_function_values = function_values[parser_struct.function_name];
    std::map<std::string, PHINode*> function_phi_values;
    for (const auto &name : assigned_vars) {
        if (name!=VarName && old_function_values.count(name)>0) {
            changed_vars.push_back(name);
            Value *val = old_function_values[name];
            PHINode *phi_val = Builder->CreatePHI(val->getType(), 2, name.c_str());
            phi_val->addIncoming(val, PreheaderBB);
            function_phi_values[name] = phi_val;
            function_values[parser_struct.function_name][name] = phi_val;
        }
    }

    // Control var phi
    PHINode *LoopVar = Builder->CreatePHI(StartVal->getType(), 2, VarName.c_str());
    LoopVar->addIncoming(StartVal, PreheaderBB);
    function_values[parser_struct.function_name][VarName] = LoopVar;




    // Emit the step value.
    Value *StepVal = nullptr;
    if (Step) {
        StepVal = Step->codegen(scope_struct);
        if (!StepVal)
            return nullptr;
    } 



    // Compute the end condition.
    Value *EndCond = End->codegen(scope_struct);
    if (!EndCond)
        return nullptr;


    // conditional goto branch
    Builder->CreateCondBr(EndCond, LoopBB, AfterBB);


    // codegen body and increment
    TheFunction->insert(TheFunction->end(), LoopBB);
    Builder->SetInsertPoint(LoopBB);

    int j=0;
    std::map<std::string, Value*> break_values_snapshot;
    std::vector<BasicBlock *> BreakBB, ContinueBB;

    Codegen_Loop_Body(scope_struct, std::move(Body), LoopBB, IncBB, AfterBB, break_values_snapshot, BreakBB, ContinueBB);
    block_values[Builder->GetInsertBlock()] = function_values[parser_struct.function_name];

    Builder->CreateBr(IncBB);
    TheFunction->insert(TheFunction->end(), IncBB);
    Builder->SetInsertPoint(IncBB);


    BasicBlock *CurBB = Builder->GetInsertBlock(); // Catch branching scenarios
    Value *NextVal;
    if (start_type=="int")
        NextVal = Builder->CreateAdd(LoopVar, StepVal, "nextvar"); // Increment  
    if (start_type=="float")
        NextVal = Builder->CreateFAdd(LoopVar, StepVal, "nextvar"); // Increment 

    LoopVar->addIncoming(NextVal, CurBB);
    for (auto &name : changed_vars) {
        Value *val = function_values[parser_struct.function_name][name];
        function_phi_values[name]->addIncoming(val, CurBB);
        function_values[parser_struct.function_name][name] = function_phi_values[name];
    }
    Builder->CreateBr(CondBB);





    TheFunction->insert(TheFunction->end(), AfterBB);
    Builder->SetInsertPoint(AfterBB);

    SetBreakPHIS(parser_struct, assigned_vars, old_function_values, function_phi_values,
                 BreakBB, CondBB);

    function_allocas[parser_struct.function_name] = old_allocas;
    // verifyFunction(*TheFunction);
    // CurModule->print(llvm::errs(), nullptr);

    return Constant::getNullValue(Type::getInt32Ty(*TheContext));
}





Value *ForEachExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return ConstantFP::get(*TheContext, APFloat(0.0f));

    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    check_scope_struct_sweep(TheFunction, scope_struct, parser_struct);


    Value *_zero = const_int(0);
    Value *CurIdx = const_int(0);


    function_values[parser_struct.function_name][VarName] = CurIdx;


    Value *vec = Vec->codegen(scope_struct);

    StructType *st = struct_types["array"];
    Value *vec_size_gep = Builder->CreateStructGEP(st, vec, 0);
    Value *VecSize = Builder->CreateLoad(intTy, vec_size_gep);



    // Make the new basic block for the loop header, inserting after current
    // block.
    BasicBlock *CondBB = BasicBlock::Create(*TheContext, "foreach.cond", TheFunction);
    BasicBlock *IncBB  = BasicBlock::Create(*TheContext, "foreach.inc");
    BasicBlock *LoopBB  = BasicBlock::Create(*TheContext, "foreach.loop");
    BasicBlock *AfterBB  = BasicBlock::Create(*TheContext, "foreach.after");
    BasicBlock *PreheaderBB = Builder->GetInsertBlock();


    block_values[PreheaderBB] = function_values[parser_struct.function_name];

    // Insert an explicit fall through from the current block to the LoopBB.
    Builder->CreateBr(CondBB); 
    Builder->SetInsertPoint(CondBB);


    // --- PHI Nodes --- //
    std::vector<std::string> assigned_vars, changed_vars;
    Get_Recursive_Assign_Statements(Body, assigned_vars);
    // Possible phi for each value
    auto old_function_values = function_values[parser_struct.function_name];
    std::map<std::string, PHINode*> function_phi_values;
    for (const auto &name : assigned_vars) {
        if (name!=VarName && old_function_values.count(name)>0) {
            changed_vars.push_back(name);
            Value *val = old_function_values[name];
            PHINode *phi_val = Builder->CreatePHI(val->getType(), 2, name.c_str());
            phi_val->addIncoming(val, PreheaderBB);
            function_phi_values[name] = phi_val;
            function_values[parser_struct.function_name][name] = phi_val;
        }
    }
    // Control var phi
    PHINode *LoopVar = Builder->CreatePHI(CurIdx->getType(), 2, VarName.c_str());
    LoopVar->addIncoming(CurIdx, PreheaderBB);
    function_values[parser_struct.function_name][VarName] = LoopVar;



    // Emit the body of the loop.  This, like any other expr, can change the
    // current BB.  Note that we ignore the value computed by the body, but don't
    // allow an error.

    Value *StepVal = const_int(1);


    // Compute the end condition.
    Value *EndCond = Builder->CreateICmpNE(
            LoopVar, VecSize, "loopcond");


    // conditional goto branch
    Builder->CreateCondBr(EndCond, LoopBB, AfterBB);


    // codegen body and increment
    TheFunction->insert(TheFunction->end(), LoopBB);
    Builder->SetInsertPoint(LoopBB);




    std::string vec_type = UnmangleVec(data_type);
    Value *vec_value;
    if (vec_type=="array") {
        StructType *st = struct_types["array"];
        llvm::Type *elem_type = get_type_from_str(Type); 

        Value *elem_size_gep = Builder->CreateStructGEP(st, vec, 2); 
        Value *elem_size = Builder->CreateLoad(intTy, elem_size_gep);

        Value *vec_gep = Builder->CreateStructGEP(st, vec, 3);
        Value *array = Builder->CreateLoad(int8PtrTy, vec_gep);

        Value *element = Builder->CreateGEP(Type::getInt8Ty(*TheContext), array, Builder->CreateMul(LoopVar, elem_size));
        vec_value = Builder->CreateLoad(elem_type, element, "elem");  
    } else
        vec_value = callret(vec_type+"_Idx", {scope_struct, vec, LoopVar});
    if((vec_type=="list"||vec_type=="tuple")&&(Type=="float"||Type=="int"))
        vec_value = callret("to_"+Type, {scope_struct, vec_value});
    function_values[parser_struct.function_name][VarName] = vec_value;

    std::map<std::string, Value*> break_values_snapshot;
    std::vector<BasicBlock *> BreakBB, ContinueBB;
    Codegen_Loop_Body(scope_struct, std::move(Body), LoopBB, IncBB, AfterBB, break_values_snapshot, BreakBB, ContinueBB);
    block_values[Builder->GetInsertBlock()] = function_values[parser_struct.function_name];


    Builder->CreateBr(IncBB);
    TheFunction->insert(TheFunction->end(), IncBB);
    Builder->SetInsertPoint(IncBB);

    BasicBlock *CurBB = Builder->GetInsertBlock(); // Catch branching scenarios
    Value *NextVal = Builder->CreateAdd(LoopVar, StepVal, "nextvar"); // Increment  

    LoopVar->addIncoming(NextVal, CurBB);
    for (auto &name : changed_vars) {
        function_phi_values[name]->addIncoming(function_values[parser_struct.function_name][name], CurBB);
        function_values[parser_struct.function_name][name] = function_phi_values[name];
    }


    Builder->CreateBr(CondBB);



    // when the loop body is done, return the insertion point to outside the for loop
    TheFunction->insert(TheFunction->end(), AfterBB);
    Builder->SetInsertPoint(AfterBB);

    SetBreakPHIS(parser_struct, assigned_vars, old_function_values, function_phi_values, BreakBB, CondBB);

    return const_float(0.0f);
}


Value *WhileExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return const_float(0.0f);

    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    check_scope_struct_sweep(TheFunction, scope_struct, parser_struct);

    // Create blocks for loop condition, loop body, and after loop
    BasicBlock *CondBB = BasicBlock::Create(*TheContext, "while.cond", TheFunction);
    BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "while.loop", TheFunction);
    BasicBlock *AfterBB = BasicBlock::Create(*TheContext, "while.after_bb", TheFunction);

    BasicBlock *PreheaderBB = Builder->GetInsertBlock();

    block_values[PreheaderBB] = function_values[parser_struct.function_name];

    // Jump to the condition block
    Builder->CreateBr(CondBB);
    Builder->SetInsertPoint(CondBB);


    std::vector<std::string> assigned_vars, changed_vars;
    Get_Recursive_Assign_Statements(Body, assigned_vars);

    // Possible phi for each value
    auto old_function_values = function_values[parser_struct.function_name];
    std::map<std::string, PHINode*> function_phi_values;
    for (const auto &name : assigned_vars) {
        if (old_function_values.count(name)>0) {
            changed_vars.push_back(name);
            Value *val = old_function_values[name];
            PHINode *phi_val = Builder->CreatePHI(val->getType(), 0, name.c_str());
            phi_val->addIncoming(val, PreheaderBB);
            function_phi_values[name] = phi_val;
            function_values[parser_struct.function_name][name] = phi_val;
        }
    }


    Value* condVal = Cond->codegen(scope_struct);
    if (!condVal)
        return nullptr;

    Builder->CreateCondBr(condVal, LoopBB, AfterBB);

    Builder->SetInsertPoint(LoopBB);

    std::map<std::string, Value*> break_values_snapshot;
    std::vector<BasicBlock *> BreakBB, ContinueBB;
    Codegen_Loop_Body(scope_struct, std::move(Body),
            LoopBB, CondBB, AfterBB,
            break_values_snapshot,
            BreakBB, ContinueBB);
    BasicBlock *CurBB = Builder->GetInsertBlock(); // handles branching
    block_values[CurBB] = function_values[parser_struct.function_name];

    for (auto &name : changed_vars) {
        PHINode *phi = function_phi_values[name];
        Value *val = function_values[parser_struct.function_name][name]; 
        phi->addIncoming(val, CurBB);
        for (auto &bb : ContinueBB)
            phi->addIncoming(block_values[bb][name], bb);
        function_values[parser_struct.function_name][name] = phi;
    }

    Builder->CreateBr(CondBB);


    Builder->SetInsertPoint(AfterBB);
    // if (ContinueBB.size()>0)
// TheFunction->print(llvm::errs());

    SetBreakPHIS(parser_struct, assigned_vars,
            old_function_values, function_phi_values, BreakBB, CondBB);

    return Constant::getNullValue(Type::getFloatTy(*TheContext));
}










bool seen_var_attr = false;








bool CheckIs_CastInt_to_FloatChannel(std::string Operation, Data_Tree LTree) {
    if(Operation!="channel_int_message")
        return false;

    if(LTree.Type=="channel" && LTree.Nested_Data.size()>0) {
        if(LTree.Nested_Data[0].Type=="float")
            return true;
    }

    return false;
}



bool CheckIsSenderChannel(std::string Elements, Parser_Struct parser_struct, std::string LName) {

    if(begins_with(Elements, "channel")) {
        if(ChannelDirections[parser_struct.function_name].count(LName)>0) {
            if(((int)ChannelDirections[parser_struct.function_name][LName])==(int)ch_sender) {
                LogErrorS(parser_struct.line, "1 Tried to attribute data to a sender only channel." + parser_struct.function_name + "/" + LName + ": " + std::to_string(ChannelDirections[parser_struct.function_name].count(LName)) + "; " + std::to_string(ChannelDirections[parser_struct.function_name][LName]) );
                return false;
            }
        }
        else {
            if(ChannelDirections[parser_struct.class_name].count(LName)>0) {
                if((int)ChannelDirections[parser_struct.class_name][LName]==(int)ch_sender) {
                    LogErrorS(parser_struct.line, "2 Tried to attribute data to a sender only channel. " + parser_struct.class_name + "/" + LName + ": " + std::to_string(ChannelDirections[parser_struct.class_name].count(LName)));
                    return false;
                }
            }
        }
    }
    return true;
}


Value *strview_to_strcmp(Value *ctx, Value *a, Value *b) {
    Value *str_a = Builder->CreateExtractValue(a, {0});

    Value *size_a = Builder->CreateExtractValue(a, {1});
    Value *size_b = callret("strlen", {b});
    return callret("str_eq", {ctx, str_a, b, size_a, size_b});
}

Value *strviewcmp(Value *ctx, Value *a, Value *b) {
    Value *str_a = Builder->CreateExtractValue(a, {0});
    Value *str_b = Builder->CreateExtractValue(b, {0});

    Value *size_a = Builder->CreateExtractValue(a, {1});
    Value *size_b = Builder->CreateExtractValue(b, {1});
    return callret("str_eq", {ctx, str_a, str_b, size_a, size_b});
}











void BinaryStore(Parser_Struct parser_struct, Value *scope_struct, int Op, std::string Operation, std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS,
                 Value *R, Data_Tree L_dt, Data_Tree R_dt) {
    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    std::string LType = L_dt.Type;

    Value *Val;
    bool from_channel_op=false;
    if (Op==tok_arrow) { 
        bool is_high_lvl_obj = ClassSize.count(LType)>0;
        if(is_high_lvl_obj)
            Operation = "void_channel_message";
        Val = callret(Operation, {scope_struct, LHS->codegen(scope_struct), R});

        if (!in_str(LType, primary_data_tokens) && !is_high_lvl_obj) {
            std::string copy_fn = LType + "_Copy";

            Function *F = CurModule->getFunction(copy_fn);
            if (!F) {
                LogErrorV(parser_struct.line, "Tried to use channel operation for " + \
                        LType + ", but this data type has no Copy implementation.");
                return;
            }
            Val = callret(copy_fn, {scope_struct, Val});
        }

        from_channel_op = true;
    }
    else
        Val = R;




    if (!Val) {
        seen_var_attr=false;
        return; 
    }



    if (LHS->GetIsList()) {
        VariableListExprAST *VarList = static_cast<VariableListExprAST *>(LHS.get());

        // std::cout << "\n";

        for (int i=0; i<VarList->ExprList.size(); ++i) {
            Nameable *LHSE = static_cast<Nameable *>(VarList->ExprList[i].get()); 

            std::string Lname = LHSE->Name;
            Data_Tree ldt = LHSE->GetDataTree();
            std::string list_LType = ldt.Type;

            // ldt.Print();
            // printTy(R);


            std::string store_trigger = list_LType + "_StoreTrigger";
            // std::cout << "got " <<  list_LType << "\n";
            // std::cout << "idx " <<  i << "\n";

            Value *Val_indexed = Builder->CreateExtractValue(Val, {static_cast<unsigned>(i)});

            // Function *F = CurModule->getFunction(copy_fn);
            // if (F)
            //   Val_indexed = callret(copy_fn, {scope_struct, Val_indexed});

            Function *F = CurModule->getFunction(store_trigger);
            if (F) {
                Value *old_val = LoadVal(parser_struct.function_name, Lname, ldt);
                call(store_trigger, {scope_struct, old_val, Val_indexed});
            }


            StoreVal(TheFunction, parser_struct.function_name, Lname, Val_indexed, ldt);

            if (!in_vec(list_LType, primary_data_tokens))
                Set_Pointer_Stack(scope_struct, parser_struct.function_name, Lname, Val_indexed);
        }
        return; 
    }

    std::string RType = R_dt.Type;
    if (LType=="bool"&&RType=="any")
        Val = callret("to_bool", {scope_struct, Val}); 
    if (LType=="int"&&RType=="any")
        Val = callret("to_int", {scope_struct, Val}); 
    if (LType=="float"&&RType=="any")
        Val = callret("to_float", {scope_struct, Val});





    Value *Lvar_name; 

    if(auto *LHSV = dynamic_cast<NameableIdx *>(LHS.get())) {

        Value *vec_ptr = LHSV->Inner->codegen(scope_struct);
        Data_Tree dt = LHSV->GetDataTree(true);
        std::string type = UnmangleVec(dt);

        Value *idx = Idx_Calc_Codegen(type, vec_ptr, LHSV->Idx, scope_struct); //StoreIdx


        if(type=="map") {
            Data_Tree map_dt = dt, key_dt = map_dt.Nested_Data[0], val_dt = map_dt.Nested_Data[1];
            std::string key_type = key_dt.Type;
            std::string value_type = val_dt.Type;

            Value *query = idx;

            StructType *st = struct_types["map"];
            StructType *st_node = struct_types["map_node"];

            if (R_dt.Type=="int"&&key_type=="float")
                query = Builder->CreateSIToFP(query, floatTy);
            if (key_type=="str")
                query = CopyStrVal(scope_struct, query);


            Value *nullPtr = ConstantPointerNull::get(
                        cast<PointerType>(int8PtrTy)
                    );


            Value *new_node_ptr = callret("malloc", {const_int(24)});

            // key
            Value *new_node_key_gep = Builder->CreateStructGEP(st_node, new_node_ptr, 0);
            if (key_type=="int") {
                Value *int_ptr = callret("malloc", {const_int(4)});
                Builder->CreateStore(query, int_ptr);
                Builder->CreateStore(int_ptr, new_node_key_gep);
            } else if (key_type=="i64") {
                Value *int_ptr = callret("malloc", {const_int(8)});
                Builder->CreateStore(query, int_ptr);
                Builder->CreateStore(int_ptr, new_node_key_gep);
            } else if (key_type=="float") {
                Value *float_ptr = callret("malloc", {const_int(4)});
                Builder->CreateStore(query, float_ptr);
                Builder->CreateStore(float_ptr, new_node_key_gep);
            } else
                Builder->CreateStore(query, new_node_key_gep); 

            // value
            Value *new_node_value_gep = Builder->CreateStructGEP(st_node, new_node_ptr, 1);
            if (value_type=="int") {
                Value *int_ptr = callret("malloc", {const_int(4)});
                Builder->CreateStore(Val, int_ptr);
                Builder->CreateStore(int_ptr, new_node_value_gep);
            } else if (value_type=="i64") {
                Value *int_ptr = callret("malloc", {const_int(8)});
                Builder->CreateStore(Val, int_ptr);
                Builder->CreateStore(int_ptr, new_node_value_gep);
            } else if (value_type=="float") {
                Value *float_ptr = callret("malloc", {const_int(4)});
                Builder->CreateStore(Val, float_ptr);
                Builder->CreateStore(float_ptr, new_node_value_gep);
            } else if (value_type=="str")
                Builder->CreateStore(CopyStrVal(scope_struct, Val), new_node_value_gep);
            else
                Builder->CreateStore(Val, new_node_value_gep);

            Value *new_node_next_gep = Builder->CreateStructGEP(st_node, new_node_ptr, 2);
            Builder->CreateStore(nullPtr, new_node_next_gep);

            // Load map attributes 
            Value *size_gep = Builder->CreateStructGEP(st, vec_ptr, 0);
            Value *map_size = Builder->CreateLoad(intTy, size_gep);


            Value *expand_at_gep = Builder->CreateStructGEP(st, vec_ptr, 2);
            Value *map_expand_at = Builder->CreateLoad(intTy, expand_at_gep);



            // Check for expansion
            Function *TheFunction = Builder->GetInsertBlock()->getParent();
            BasicBlock *MapInsertBB = BasicBlock::Create(*TheContext, "map.insert.bb", TheFunction);
            BasicBlock *MapExpandBB = BasicBlock::Create(*TheContext, "map.expand.bb", TheFunction);


            Builder->CreateCondBr(
                        Builder->CreateICmpSGE(Builder->CreateAdd(map_size,const_int(1)), map_expand_at),
                        MapExpandBB, MapInsertBB);

            Builder->SetInsertPoint(MapExpandBB);
            call("map_expand", {scope_struct, vec_ptr});
            Builder->CreateBr(MapInsertBB);


            Builder->SetInsertPoint(MapInsertBB);

            Value *capacity_gep = Builder->CreateStructGEP(st, vec_ptr, 1);
            Value *map_capacity = Builder->CreateLoad(intTy, capacity_gep);

            Value *nodes_gep = Builder->CreateStructGEP(st, vec_ptr, 5);
            Value *nodes = Builder->CreateLoad(int8PtrTy->getPointerTo(), nodes_gep);




            BasicBlock *CheckFirstKeyBB = BasicBlock::Create(*TheContext, "map.check_first_key.bb", TheFunction);
            BasicBlock *FromFirstKeyBB = BasicBlock::Create(*TheContext, "map.from_first_key.bb", TheFunction);
            BasicBlock *FromKeyBB = BasicBlock::Create(*TheContext, "map.from_first_key.bb", TheFunction);
            BasicBlock *PtrChaseBB = BasicBlock::Create(*TheContext, "map.pointer_chase.bb", TheFunction);
            BasicBlock *PtrChaseCheckKeyBB = BasicBlock::Create(*TheContext, "map.pointer_chase_check_key.bb", TheFunction);
            BasicBlock *FromPtrChaseBB = BasicBlock::Create(*TheContext, "map.new_from_pointer_chase.bb", TheFunction);
            BasicBlock *FromNullBB = BasicBlock::Create(*TheContext, "map.from_null.bb", TheFunction);
            BasicBlock *AfterBB = BasicBlock::Create(*TheContext, "map.after.bb", TheFunction);


            // Check if bucket is nullptr
            Value *query_hash;
            if (key_type=="str")
                query_hash = str_llvm_hash(query, TheFunction);        
            if (key_type=="float")
                query_hash = float_llvm_hash(query);
            if (key_type=="int")
                query_hash = query;
            if (key_type=="array")
                query_hash = callret("hash_array_"+key_dt.Nested_Data[0].Type, {scope_struct, query});
            if (in_vec(key_type,int_types) && key_type!="int")
                query_hash = Builder->CreateIntCast(query, intTy, true);
            Value *hash_pos = Builder->CreateURem(query_hash, map_capacity);


            Value *node_gep = Builder->CreateGEP(int8PtrTy->getPointerTo(), nodes, hash_pos);
            Value *node = Builder->CreateLoad(int8PtrTy, node_gep);

            Value *IsNull = Builder->CreateICmpEQ(node, nullPtr);

            Builder->CreateCondBr(IsNull, FromNullBB, CheckFirstKeyBB);

            // From nullptr
            Builder->SetInsertPoint(FromNullBB);
            // Builder->CreateStore(new_node_ptr, node_gep); 
            call("map_node_set_bucket", {scope_struct, vec_ptr, new_node_ptr, hash_pos});
            Builder->CreateBr(AfterBB);

            // Check first key
            Builder->SetInsertPoint(CheckFirstKeyBB);
            Value *key_gep = Builder->CreateStructGEP(st_node, node, 0);
            Value *keyCond, *key;
            if (key_type=="int"||key_type=="i64") {
                llvm::Type *ty = get_type_from_data(Data_Tree(key_type));
                Value *key_void_ptr = Builder->CreateLoad(int8PtrTy, key_gep);
                Value *key_int_ptr = Builder->CreateBitCast(key_void_ptr, ty->getPointerTo());
                key = Builder->CreateLoad(ty, key_int_ptr);
                keyCond = Builder->CreateICmpEQ(key, query);
            } else if (key_type=="float") {
                Value *key_void_ptr = Builder->CreateLoad(int8PtrTy, key_gep);
                Value *key_float_ptr = Builder->CreateBitCast(key_void_ptr, floatTy->getPointerTo());
                key = Builder->CreateLoad(floatTy, key_float_ptr);
                keyCond = Builder->CreateFCmpUEQ(key, query);
            } else if (key_type=="array") {
                key = Builder->CreateLoad(int8PtrTy, key_gep);
                keyCond = callret("array_eq_"+key_dt.Nested_Data[0].Type, {scope_struct, query, key});
            } else {
                key = Builder->CreateLoad(int8PtrTy, key_gep);
                // keyCond = strviewcmp(scope_struct, key, query);
                keyCond = callret("strcmp", {key, query});
                keyCond = Builder->CreateICmpEQ(keyCond, const_int(0));
            }
            Builder->CreateCondBr(keyCond, FromFirstKeyBB, PtrChaseBB);


            // Key overwrite
            Builder->SetInsertPoint(FromFirstKeyBB);
            call("map_node_reclaim", {scope_struct, vec_ptr, node});
            // Value *next_node_of_first_gep = Builder->CreateStructGEP(st_node, node, 2);
            // Value *next_node_of_first = Builder->CreateLoad(int8PtrTy, next_node_of_first_gep);
            // Builder->CreateStore(next_node_of_first, new_node_next_gep);
            // Builder->CreateStore(new_node_ptr, node_gep);
            call("map_node_overwrite_bucket", {scope_struct, new_node_ptr, vec_ptr, hash_pos});
            Builder->CreateBr(AfterBB);


            // Pointer Chase
            Builder->SetInsertPoint(PtrChaseBB);
            PHINode *map_phi_node = Builder->CreatePHI(int8PtrTy, 2);
            map_phi_node->addIncoming(node, CheckFirstKeyBB);

            Value *next_node_gep = Builder->CreateStructGEP(st_node, map_phi_node, 2);
            Value *next_node = Builder->CreateLoad(int8PtrTy, next_node_gep);
            map_phi_node->addIncoming(next_node, PtrChaseCheckKeyBB);

            Value *IsNextNull = Builder->CreateICmpEQ(next_node, nullPtr);

            Builder->CreateCondBr(IsNextNull, FromPtrChaseBB, PtrChaseCheckKeyBB);

            Builder->SetInsertPoint(PtrChaseCheckKeyBB);
            key_gep = Builder->CreateStructGEP(st_node, next_node, 0);
            if (key_type=="int"||key_type=="i64") {
                llvm::Type *ty = get_type_from_data(Data_Tree(key_type));
                Value *key_void_ptr = Builder->CreateLoad(int8PtrTy, key_gep);
                Value *key_int_ptr = Builder->CreateBitCast(key_void_ptr, ty->getPointerTo());
                key = Builder->CreateLoad(ty, key_int_ptr);
                keyCond = Builder->CreateICmpEQ(key, query);
            } else if (key_type=="float") {
                Value *key_void_ptr = Builder->CreateLoad(int8PtrTy, key_gep);
                Value *key_float_ptr = Builder->CreateBitCast(key_void_ptr, floatTy->getPointerTo());
                key = Builder->CreateLoad(floatTy, key_float_ptr);
                keyCond = Builder->CreateFCmpUEQ(key, query);
            } else if (key_type=="array") {
                key = Builder->CreateLoad(int8PtrTy, key_gep);
                keyCond = callret("array_eq_"+key_dt.Nested_Data[0].Type, {scope_struct, query, key});
            } else {
                key = Builder->CreateLoad(int8PtrTy, key_gep);
                keyCond = callret("strcmp", {key, query});
                keyCond = Builder->CreateICmpEQ(keyCond, const_int(0));
            }
            Builder->CreateCondBr(keyCond, FromKeyBB, PtrChaseBB);


            // Pointer Chase Overwrite
            Builder->SetInsertPoint(FromKeyBB);
            call("map_node_reclaim", {scope_struct, vec_ptr, next_node});
            call("map_node_overwrite", {scope_struct, vec_ptr, new_node_ptr, map_phi_node, next_node});
            // Value *next_next_node_gep = Builder->CreateStructGEP(st_node, next_node, 2);
            // Value *next_next_node = Builder->CreateLoad(int8PtrTy, next_next_node_gep);
            // Builder->CreateStore(next_next_node, new_node_next_gep);
            // Builder->CreateStore(new_node_ptr, next_node_gep);
            Builder->CreateBr(AfterBB);


            // New pointer from Pointer Chase
            Builder->SetInsertPoint(FromPtrChaseBB);
            // Builder->CreateStore(new_node_ptr, next_node_gep); 
            call("map_node_set_next", {scope_struct, vec_ptr, new_node_ptr, map_phi_node});
            Builder->CreateBr(AfterBB);


            // After
            Builder->SetInsertPoint(AfterBB);


            return;
        }










        if (type=="array") {
            StructType *st = struct_types["array"];
            std::string elem_type = dt.Nested_Data[0].Type;

            Check_Is_Array_Inbounds(parser_struct, vec_ptr, idx);

            if (elem_type=="float"&&RType=="int")
                Val = Builder->CreateSIToFP(Val, floatTy);

            Value *vec = Load_Array(parser_struct.function_name, vec_ptr);

            llvm::Type *idxTy;
            if (elem_type=="int")
                idxTy = intTy;
            else if (elem_type=="float") 
                idxTy = floatTy;
            else if (elem_type=="bool") 
                idxTy = boolTy;
            else if (elem_type=="str") 
                idxTy = struct_types["DT_str"];
            else 
                idxTy = int8PtrTy;


            Value *element = Builder->CreateGEP(idxTy, vec, idx);
            Builder->CreateStore(Val, element);
        } else if (llvm_store_idx.count(type)>0)
            llvm_store_idx[type](parser_struct, Builder->GetInsertBlock()->getParent(),
                                            L_dt, R_dt, LHS, RHS, scope_struct, vec_ptr, idx, Val);
        else if (L_dt.is_array) {// char[8]
            L_dt = LHS->GetDataTree(true);
            llvm::Type *Ty = get_type_from_data(L_dt);

            Value *gep = Builder->CreateGEP(Ty, vec_ptr, {const_int(0), idx});

            Builder->CreateStore(R, gep);
        } else if (L_dt.is_buffer) {// float[]
            L_dt = LHS->GetDataTree(true);
            llvm::Type *Ty = get_type_from_data(L_dt);
            
            Value *gep = Builder->CreateGEP(Ty, vec_ptr, idx);
            Builder->CreateStore(R, gep);
        } else 
            call(type+"_Store_Idx", {vec_ptr, idx, Val, scope_struct});

        return;
    } 






    std::string Lname = LHS->GetName();

    if(auto *LHSV = dynamic_cast<Nameable *>(LHS.get())) {
        if (LHSV->Depth==1) { // is_alloca
            std::string store_trigger = LType + "_StoreTrigger";
            std::string copy_fn = LType + "_Copy";

            // Copy data types that support copying (i.e, function <DT>_Copy exists)
            if(auto Rvar = dynamic_cast<Nameable *>(RHS.get())) // if it is leaf
            {
                Function *F = CurModule->getFunction(copy_fn);
                if (!from_channel_op && F)
                    Val = callret(copy_fn, {scope_struct, Val});
            }

            // Store trigger behavior for supported types (i.e, function <DT>_StoreTrigger exists)
            Function *F = CurModule->getFunction(store_trigger);

            if (F && function_values[parser_struct.function_name].count(Lname)>0) {
                Value *old_val = LoadVal(parser_struct.function_name, Lname, L_dt);
                Val = callret(store_trigger, {scope_struct, old_val, Val});
            }

            if(!in_str(LType, primary_data_tokens)) {
                Value *unpacked_val = Val;
                if (RType=="str")
                    unpacked_val = Builder->CreateExtractValue(Val, {0});
                Set_Pointer_Stack(scope_struct, parser_struct.function_name, Lname, unpacked_val);
            }

            StoreVal(TheFunction, parser_struct.function_name, Lname, Val, L_dt);

        } else { // self|attr 
            Value *src = LHSV->Inner->codegen(scope_struct);
            LHSV->Load_Last=false;
            Value *obj_ptr = LHSV->codegen(scope_struct);
            
            if (!in_vec(LType, primary_data_tokens) && LType!="charv") {
                // p2t("Store " + LHSV->Inner->GetDataTree().Type + " -- " + LType);
                MakeWriteBarrier(parser_struct, Builder->GetInsertBlock()->getParent(), scope_struct,
                                    src, obj_ptr, Val, LType);

            } else
                Builder->CreateStore(Val, obj_ptr);

        }
    } else
        LogErrorC(-1, "Variable " + Lname + " could not be attributed");
    


    seen_var_attr=false;
}




Value *BinaryExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return ConstantFP::get(*TheContext, APFloat(0.0f));


    Value *R = RHS->codegen(scope_struct);
    if (cast_R_to=="int_to_float")
        R = Builder->CreateSIToFP(R, floatTy, "lfp");

    if (cast_R_to=="to_int")
        R = Builder->CreateIntCast(R, intTy, true);
    if (cast_R_to=="to_i8")
        R = Builder->CreateIntCast(R, int8Ty, true);
    if (cast_R_to=="to_i16")
        R = Builder->CreateIntCast(R, int16Ty, true);
    if (cast_R_to=="to_i64")
        R = Builder->CreateIntCast(R, int64Ty, true);


    if (Op == '=' || (Op==tok_arrow&&!begins_with(Elements, "channel"))) {
        BinaryStore(parser_struct, scope_struct, Op, Operation, LHS, RHS, R, L_dt, R_dt);
        return const_int(0);
    }





    // --- Handle operations --- //
    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    Value *L = LHS->codegen(scope_struct);  
    Value *ret;
    if (!L || !R)
        return nullptr;


    if (cast_L_to=="int_to_float")
        L = Builder->CreateSIToFP(L, Type::getFloatTy(*TheContext), "lfp");
    if (cast_L_to=="i64_to_int")
        L = Builder->CreateIntCast(L, intTy, true);

    if (Operation=="tensor_int_div") {
        Operation = "tensor_float_div";
        R = Builder->CreateSIToFP(R, Type::getFloatTy(*TheContext), "lfp");
    }

    if (CheckIs_CastInt_to_FloatChannel(Operation, LHS->GetDataTree())) {
        Operation = "channel_float_message";
        R = Builder->CreateSIToFP(R, Type::getFloatTy(*TheContext), "lfp");
    }



    if (Elements=="bool_bool") {
        switch (Op) {
            case tok_equal:
                ret = Builder->CreateICmpEQ(L, R, "booleq");
                break;
            case tok_and:
                ret = Builder->CreateAnd(L, R, "booland");
                break;
            case tok_or:
                ret = Builder->CreateOr(L, R, "boolor");
                break;
        }
    }
    else if (Elements=="charv_i64"||Elements=="charv_int") {
        if(Elements=="charv_int")
            R = Builder->CreateSExtOrTrunc(R, int64Ty);

        if(Op=='+') {
            Value *view_val = UndefValue::get(struct_types["DT_str"]);
            view_val = Builder->CreateInsertValue(view_val, Builder->CreateInBoundsGEP(int8Ty, L, R), {0});
            view_val = Builder->CreateInsertValue(view_val, const_int(0), {1});
            ret = view_val;
        } else
            LogErrorC(parser_struct.line, "unkown charv op " + Operation);

    }
    else if (Elements=="buffer_i8_i64"||Elements=="buffer_i8_int") {
        if(Op==tok_offby) {
            if(auto *LHSV = dynamic_cast<Nameable *>(LHS.get())) {

                llvm::Type *lTy = get_type_from_data(L_dt);
                Value *loaded = Builder->CreateLoad(lTy, L);
                
                Value *buffer_size = const_int(std::stoi(L_dt.Nested_Data[0].Type)); 
                Value *size = Builder->CreateSub(buffer_size, R);

                Value *ptr = Builder->CreateGEP(int8Ty, L, R);
                // Value *ptr = Builder->CreateGEP(lTy, L, {const_int(0), R});


                Value *view_val = UndefValue::get(struct_types["DT_str"]);
                view_val = Builder->CreateInsertValue(view_val, ptr, {0});
                view_val = Builder->CreateInsertValue(view_val, \
                            size, {1});
                
                ret = view_val;
            }
        } else
            LogErrorC(parser_struct.line, "Unkown buffer op " + Operation);

    }
    else if (Elements=="buffer_float_i64"||Elements=="buffer_float_int") {
        if(Op==tok_offby) {
            if(auto *LHSV = dynamic_cast<Nameable *>(LHS.get())) {
                if (L_dt.is_buffer) {
                    llvm::Type *lTy = get_type_from_data(L_dt);
                    Value *ptr = Builder->CreateGEP(lTy, L, R);
                    ret = ptr;
                } else {
                    llvm::Type *lTy = get_type_from_data(L_dt);
                    Value *ptr = Builder->CreateGEP(lTy, L, {const_int(0), R});
                    ret = ptr;
                }
            }
        } else
            LogErrorC(parser_struct.line, "unkown buffer op " + Operation);
    }
    else if (Elements=="str_str") {
        switch (Op) {
            case tok_equal: {
                Value *L_ptr = Builder->CreateExtractValue(L, {0});
                Value *L_size = Builder->CreateExtractValue(L, {1});
                Value *R_ptr = Builder->CreateExtractValue(R, {0});
                Value *R_size = Builder->CreateExtractValue(R, {1});
                ret = callret("str_eq", {scope_struct, L_ptr, R_ptr, L_size, R_size});
                break;
                }
            case '+': {
                // Value *L_ptr = Builder->CreateExtractValue(L, {0});
                // Value *L_size = Builder->CreateExtractValue(L, {1});
                // Value *R_ptr = Builder->CreateExtractValue(R, {0});
                // Value *R_size = Builder->CreateExtractValue(R, {1});
                ret = callret("str_str_add", {scope_struct, L, R});
                break;
                }
            default:
                break;
        }   
    }
    else if (Elements=="str_int") {
        switch (Op) {
            case tok_offby: {
                Value *str_ptr = Builder->CreateExtractValue(L, {0});
                Value *old_size = Builder->CreateExtractValue(L, {1});

                Value *offset_val = Builder->CreateGEP(int8Ty, str_ptr, R);
                Value *new_size = Builder->CreateSub(old_size, R);
                
                Value *view_val = UndefValue::get(struct_types["DT_str"]);
                view_val = Builder->CreateInsertValue(view_val, offset_val, {0});
                view_val = Builder->CreateInsertValue(view_val, new_size, {1});
                ret = view_val;
                break;
                }
            default:
                break;
        }
    }
    else if (Elements=="vec_vec") {
        switch (Op) {
            case '+':
                ret = simd_add(LHS, RHS, L, R);
                break;
            case '*':
                ret = simd_mult(LHS, RHS, L, R);
                break;
            case tok_equal:
                ret = simd_equal(LHS, RHS, L, R);
                break;
            case tok_diff:
                ret = simd_dif(LHS, RHS, L, R);
                break;
            case tok_and:
                ret = simd_and(LHS, RHS, L, R);
                break;
            case tok_or:
                ret = simd_or(LHS, RHS, L, R);
                break;
            case '<':
                ret = simd_minor(LHS, RHS, L, R);
                break;
            case '>':
                ret = simd_higher(LHS, RHS, L, R);
                break;
            case tok_minor_eq:
                ret = simd_minoreq(LHS, RHS, L, R);
                break;
            case tok_higher_eq:
                ret = simd_highereq(LHS, RHS, L, R);
                break;
            default:
                LogError(parser_struct.line, "Unimplemented vec op " + Operation);
                std::exit(0);
        }
    }
    else if (Elements=="vec_int") {
        switch (Op) {
            case tok_lshift:
                ret = simd_lshift(LHS, RHS, L, R);
                break;
            case tok_rshift:
                ret = simd_rshift(LHS, RHS, L, R);
                break;
            case tok_and:
                ret = simd_and(LHS, RHS, L, R);
                break;
            case tok_or:
                ret = simd_or(LHS, RHS, L, R);
                break;
            default:
                LogError(parser_struct.line, "Unimplemented vec op " + Operation);
                std::exit(0);
        }
    }
    else if (Elements=="float_float") {
        switch (Op) {
            case '+':
                ret = Builder->CreateFAdd(L, R, "addtmp");
                break;
            case ':':
                ret = L;
                break;
            case tok_space:
                ret = R;
                break;
            case '-':
                ret = Builder->CreateFSub(L, R, "subtmp");
                break;
            case '*':
                ret = Builder->CreateFMul(L, R, "multmp");
                break;
            case '/':
                ret = Builder->CreateFDiv(L, R, "divtmp");
                break;
            case '%':
                ret = Builder->CreateFRem(L, R, "remtmp");
                break;
            case tok_int_div:
                ret = LogErrorV(parser_struct.line, "GOTCHA");
                break;
            case '<':
                ret = Builder->CreateFCmpULT(L, R, "cmptmp");
                break;
            case '>':
                ret = Builder->CreateFCmpULT(R, L, "cmptmp");
                break;
            case tok_equal:
                ret = Builder->CreateFCmpUEQ(L, R, "cmptmp");
                break;
            case tok_diff:
                ret = Builder->CreateFCmpUNE(L, R, "cmptmp");
                break;
            case tok_minor_eq:
                ret = Builder->CreateFCmpULE(L, R, "cmptmp");  // less or equal
                break;
            case tok_higher_eq:
                ret = Builder->CreateFCmpUGE(L, R, "cmptmp");  // greater or equal
                break;
            default:
                LogError(parser_struct.line, "Unimplemented float op " + Operation);
                std::exit(0);
        }

    } else if (in_vec(Elements, {"int_int", "i64_i64", "i8_i8", "i16_i16"})) {
        switch (Op) {
            case '+':
                ret = Builder->CreateAdd(L, R, "addtmp");
                break;
            case ':':
                ret = L;
                break;
            case tok_space:
                ret = R;
                break;
            case '-':
                ret = Builder->CreateSub(L, R, "subtmp");
                break;
            case '*':
                ret = Builder->CreateMul(L, R, "multmp");
                break;
            case '/':
                {
                    llvm::Value* L_float = Builder->CreateSIToFP(L, floatTy, "lfp");
                    llvm::Value* R_float = Builder->CreateSIToFP(R, floatTy, "rfp");
                    ret = Builder->CreateFDiv(L_float, R_float, "divtmp");
                    break;
                }
            case '%':
                ret = Builder->CreateSRem(L, R, "remtmp");  // Signed remainder
                break;
            case tok_int_div:
                ret = Builder->CreateSDiv(L, R, "divtmp");  // Signed division
                break;
            case '<':
                ret = Builder->CreateICmpSLT(L, R, "cmptmp");
                break;
            case '>':
                ret = Builder->CreateICmpSGT(L, R, "cmptmp");
                break;
            case tok_equal:
                ret = Builder->CreateICmpEQ(L, R, "cmptmp");
                break;
            case tok_diff:
                ret = Builder->CreateICmpNE(L, R, "cmptmp");
                break;
            case tok_minor_eq:
                ret = Builder->CreateICmpSLE(L, R, "cmptmp");
                break;
            case tok_higher_eq:
                ret = Builder->CreateICmpSGE(L, R, "cmptmp");
                break;
            case '|':
                ret = Builder->CreateOr(L, R);
                break;
            case tok_lshift:
                ret = Builder->CreateShl(L, R);
                break;
            case tok_rshift:
                ret = Builder->CreateLShr(L, R);
                break;
            default:
                LogError(parser_struct.line, "Unimplemented int op " + Operation);
                std::exit(0);
        }
    } 
    else if (llvm_data_ops.count(Operation)>0)
        ret = llvm_data_ops[Operation](parser_struct, Builder->GetInsertBlock()->getParent(),
                                        L_dt, R_dt, LHS, RHS, scope_struct, L, R);
    else
        ret = callret(Operation, {scope_struct, L, R}); 



    if (is_store_sugar) {
        BinaryStore(parser_struct, scope_struct, '=', Operation, LHS, LHS, ret, L_dt, L_dt);
        return const_int(0);
    }


    return ret;






    // If it wasn't a builtin binary operator, it must be a user defined one. Emit
    // a call to it.
    Function *F = getFunction(std::string("binary") + Op);
    assert(F && "Operator not found.");

    Value *Ops[] = {L, R};
    return Builder->CreateCall(F, Ops, "binop");
}

































Value *UnaryExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return const_float(0);
    Value *OperandV = Operand->codegen(scope_struct);
    if (!OperandV)
        return nullptr;


    std::string operand_type = Operand->GetDataTree().Type;
    if (Opcode=='-') {
        if (Operand->GetDataTree().Type=="int")
            return Builder->CreateMul(const_int(-1), OperandV, "multmp");
    }


    if (Opcode==tok_not||Opcode=='!') {
        if(operand_type!="bool") {
            if (!in_vec(operand_type, primary_data_tokens)&&!in_vec(operand_type, compound_tokens)) {
                Value *nullPtr = ConstantPointerNull::get(
                        cast<PointerType>(int8PtrTy)
                        );
                return OperandV = Builder->CreateICmpEQ(OperandV, nullPtr);
            }
            else
                LogErrorS(parser_struct.line, "Cannot use not with type: " + operand_type);
        } 
        return Builder->CreateNot(OperandV, "logicalnot");
    }

    if (Opcode==';')
        return OperandV;


    Function *F = getFunction(std::string("unary") + std::to_string(Opcode));
    if (!F) {
        auto err = LogErrorV(parser_struct.line,"Unknown unary operator.");
        std::cout << "" << Opcode << "/" << ReverseToken(Opcode) << ".\n";
        return err;
    }

    return Builder->CreateCall(F, OperandV, "unop");
}


Value *ChannelExprAST::codegen(Value *scope_struct) {
    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    int buffer_size = stoi(data_type.Nested_Data[1].Type);

    p2t("create channel expr");
    call("print_int", {const_int(buffer_size)});
    Value *initial_value = callret("channel_Create", {scope_struct,
                                const_int16(data_name_to_type()[data_type.Nested_Data[0].Type]),
                                const_int(buffer_size)});


    StoreVal(TheFunction, parser_struct.function_name, Name, initial_value, Data_Tree("channel"));
    Allocate_On_Pointer_Stack(scope_struct, parser_struct.function_name, Name, Data_Tree("channel"), initial_value);

    return const_float(0);
}


Value *AsyncFnPriorExprAST::codegen(Value *scope_struct) {
    int fnIndex = 1;
    while (CurModule->getFunction("__async_" + std::to_string(fnIndex)))
        fnIndex++;


    BasicBlock *CurrentBB = Builder->GetInsertBlock();


    // Create function for this async function
    llvm::Type *int8PtrTy = Type::getInt8Ty(*TheContext)->getPointerTo();

    FunctionType *asyncFunTy = FunctionType::get(
            int8PtrTy,
            {int8PtrTy},
            false);

    std::string functionName = "__async_" + std::to_string(fnIndex);

    Function *asyncFun =
        Function::Create(asyncFunTy,
                Function::ExternalLinkage,
                functionName,
                TheModule.get());

    // emit EntryBB value
    BasicBlock *BB = BasicBlock::Create(*TheContext, "async_bb", asyncFun);
    Builder->SetInsertPoint(BB);

    Value *V = const_float(0.0);

    Builder->CreateRet(Constant::getNullValue(int8PtrTy));  

    Builder->SetInsertPoint(CurrentBB);

    return const_float(0.0);
}












Function *codegenAsyncFunction(std::vector<std::unique_ptr<ExprAST>> asyncBody, \
        Value *scope_struct, \
        Parser_Struct parser_struct, std::string async_suffix,\
        Value *AsyncsCount) {


    // find existing unique function name (_async_1, _async_2, _async_3 etc)
    int fnIndex = 1;
    while (TheModule->getFunction("__async_" + std::to_string(fnIndex)))
        fnIndex++;


    // Create function for this async function
    llvm::Type *int8PtrTy = Type::getInt8Ty(*TheContext)->getPointerTo();

    FunctionType *asyncFunTy = FunctionType::get(
            int8PtrTy,
            {int8PtrTy},
            false);

    std::string functionName = "__async_" + std::to_string(fnIndex);
    Function *asyncFun =
        Function::Create(asyncFunTy,
                Function::ExternalLinkage,
                functionName,
                TheModule.get());


    std::vector<std::string> previous_scope_value_types;
    std::vector<std::string> previous_scope_value_names;
    for (auto &pair : function_values[parser_struct.function_name]) {
        if(pair.first=="QQ_stack_top")
            continue;

        std::string type;
        if (Object_toClass[parser_struct.function_name].count(pair.first)>0)
            type = "void";
        else
        {   
            type = UnmangleVec(data_typeVars[parser_struct.function_name][pair.first]);
            if(!in_str(type, primary_data_tokens))
                type="void";
        }

        call("dive_"+type, {global_str(functionName), pair.second, global_str(pair.first)});

        previous_scope_value_types.push_back(type);
        previous_scope_value_names.push_back(pair.first);
    }

    int uniques_size = Global_Uniques_Idx.size(); 

    //Dive scope_struct
    call("scope_struct_Save_for_Async", {scope_struct, global_str(functionName)}); 

    // emit EntryBB value
    BasicBlock *BB = BasicBlock::Create(*TheContext, "async_bb", asyncFun);
    Builder->SetInsertPoint(BB);

    // Recover scope_struct Value * on the new function
    Value *scope_struct_copy = callret("scope_struct_Load_for_Async", {const_int(uniques_size), global_str(functionName)}); 




    // define body of function
    Value *V = const_float(0.0);

    Value *scope_struct_typed = Builder->CreateBitCast(
            scope_struct_copy, 
            scope_struct->getType()  // the original struct type
            );



    std::string async_scope = parser_struct.function_name + async_suffix;
    for(int i=0; i<previous_scope_value_names.size(); ++i) {
        std::string type = previous_scope_value_types[i];
        std::string var_name = previous_scope_value_names[i];

        Value *v = callret("emerge_"+type, {global_str(functionName), global_str(var_name)});

        llvm::Type *llvm_type = get_type_from_str(type);
        function_values[async_scope][var_name] = v;
    }
    function_values[async_scope]["QQ_stack_top"] = const_int(uniques_size);




    block_values[BB] = function_values[parser_struct.function_name];
    for (auto &body : asyncBody)
        V = body->codegen(scope_struct_typed);


    Value *stack_top_value_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct_typed, 3);
    Builder->CreateStore(const_int(uniques_size), stack_top_value_gep); // for gc reclaim
    call("scope_struct_Delete", {scope_struct_typed});


    if (V) { 
        Builder->CreateRet(Constant::getNullValue(int8PtrTy));  


        std::string functionError;
        llvm::raw_string_ostream functionErrorStream(functionError);

        if (verifyFunction(*asyncFun, &functionErrorStream)) {
            functionErrorStream.flush();
            llvm::errs() << "codegen Async Function: Function verification failed:\n" << functionError << "\n";
        } 

        verifyModule(*TheModule);
        // TheModule->print(llvm::errs(), nullptr);
        return asyncFun;
    }

    std::cout << "ERASING ASYNC FROM PARENT" << "\n";
    asyncFun->eraseFromParent();

    return nullptr;
}



Value *SpawnExprAST::codegen(Value *scope_struct) {

    BasicBlock *CurrentBB = Builder->GetInsertBlock();
    Function *asyncFun = codegenAsyncFunction(std::move(Body), scope_struct, parser_struct, "_spawn", const_int(0));
    Builder->SetInsertPoint(CurrentBB);
    block_values[CurrentBB] = function_values[parser_struct.function_name];

    PointerType *pthreadTy = Type::getInt8Ty(*TheContext)->getPointerTo();
    Value *pthreadPtr = Builder->CreateAlloca(pthreadTy, nullptr);


    Value *voidPtrNull = Constant::getNullValue(Type::getInt8Ty(*TheContext)->getPointerTo());


    call("pthread_create_aux",
            {pthreadPtr,
            voidPtrNull,
            asyncFun,
            voidPtrNull}
        );

    return const_float(0);
}












Value *AsyncExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return ConstantFP::get(*TheContext, APFloat(0.0f));
    // Create/Spawn Threads

    // Value *barrier = callret("get_barrier", {const_int(1)});

    BasicBlock *CurrentBB = Builder->GetInsertBlock();
    Function *asyncFun = codegenAsyncFunction(std::move(Body), scope_struct, parser_struct, "_async", const_int(0));
    Builder->SetInsertPoint(CurrentBB);
    block_values[CurrentBB] = function_values[parser_struct.function_name];

    Function *pthread_create = TheModule->getFunction("pthread_create_aux");


    PointerType *pthreadTy = Type::getInt8Ty(*TheContext)->getPointerTo();
    Value *pthreadPtr = Builder->CreateAlloca(pthreadTy, nullptr);



    Value *voidPtrNull = Constant::getNullValue(
            Type::getInt8Ty(*TheContext)->getPointerTo());


    Builder->CreateCall(pthread_create,
            {pthreadPtr,
            voidPtrNull,
            asyncFun,
            voidPtrNull}
            );

    p2t("AsyncExpr Created join call");


    thread_pointers.push_back(pthreadPtr);

    return pthreadPtr;
}

Value *AsyncsExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return ConstantFP::get(*TheContext, APFloat(0.0f));

    // Create/Spawn Threads

    Value *count = Count->codegen(scope_struct);
    call("scope_struct_Store_Asyncs_Count", {scope_struct, count});
    // Value *barrier = callret("get_barrier", {Count});

    BasicBlock *CurrentBB = Builder->GetInsertBlock();

    Function *asyncFun = codegenAsyncFunction(std::move(Body), scope_struct, parser_struct, "_asyncs", count);

    Builder->SetInsertPoint(CurrentBB);
    block_values[CurrentBB] = function_values[parser_struct.function_name];



    Function *TheFunction = Builder->GetInsertBlock()->getParent();


    Value *st_thread_ptrs = UndefValue::get(struct_types["DT_thread_pointers"]);
    Value *thread_ptrs = callret("malloc",
                            {Builder->CreateMul(const_int(8), count)});
    thread_ptrs =
        Builder->CreateBitCast(
            thread_ptrs,
            int64Ty->getPointerTo());
    st_thread_ptrs = Builder->CreateInsertValue(st_thread_ptrs, thread_ptrs, {0});
    st_thread_ptrs = Builder->CreateInsertValue(st_thread_ptrs, count, {1});

    BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "asyncs.spawn_loop", TheFunction);
    BasicBlock *AfterBB = BasicBlock::Create(*TheContext, "asyncs.after", TheFunction);

    Builder->CreateBr(LoopBB);

    Builder->SetInsertPoint(LoopBB);

    PHINode *idx = Builder->CreatePHI(intTy, 2);
    idx->addIncoming(const_int(0), CurrentBB);

    Value *pthreadPtr = Builder->CreateAlloca(int8PtrTy, nullptr);
    Value *voidPtrNull = Constant::getNullValue(int8PtrTy);
    call("pthread_create_aux",
            {pthreadPtr,
            voidPtrNull,
            asyncFun,
            voidPtrNull}
        );
    
    Value *handle = Builder->CreateLoad(int64Ty, pthreadPtr);
    Value *thread_ptr_gep = Builder->CreateGEP(int64Ty, thread_ptrs, idx);
    Builder->CreateStore(handle, thread_ptr_gep);

    Value *next_idx = Builder->CreateAdd(idx, const_int(1));
    idx->addIncoming(next_idx, LoopBB);

    Value *cond = Builder->CreateICmpSLT(next_idx, count);
    Builder->CreateCondBr(cond, LoopBB, AfterBB);


    Builder->SetInsertPoint(AfterBB);
    return st_thread_ptrs;
}


Value *IncThreadIdExprAST::codegen(Value *scope_struct) {
    call("scope_struct_Increment_Thread", {scope_struct});
    return ConstantFP::get(*TheContext, APFloat(0.0f));
}




Value *SplitParallelExprAST::codegen(Value *scope_struct) {

    Value *inner_vec = Inner_Vec->codegen(scope_struct);
    std::string type = UnmangleVec(Inner_Vec->GetDataTree());
    SetType(type);


    std::string split_fn = type + "_Split_Parallel";

    return callret(split_fn, {scope_struct, inner_vec});
}

Value *SplitStridedParallelExprAST::codegen(Value *scope_struct) {

    Value *inner_vec = Inner_Vec->codegen(scope_struct);
    std::string type = UnmangleVec(Inner_Vec->GetDataTree());
    SetType(type);

    std::string split_fn = type + "_Split_Strided_Parallel";

    return callret(split_fn, {scope_struct, inner_vec});
}


Value *FinishExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return const_float(0.0f);

    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    std::vector<Value *> thread_ids; 
    for (const auto &body : Bodies) {
        if (auto *stmt = dynamic_cast<AsyncsExprAST*>(body.get()))
            thread_ids.push_back(body->codegen(scope_struct));
        else
            body->codegen(scope_struct);
    }

    int i=0;
    for (Value *st_thread_ptrs : thread_ids) {
        Value *ptrs = Builder->CreateExtractValue(st_thread_ptrs, {0});
        Value *count = Builder->CreateExtractValue(st_thread_ptrs, {1});
        
        BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "finish.spawn_loop"+std::to_string(i), TheFunction);
        BasicBlock *AfterBB = BasicBlock::Create(*TheContext, "finish.after"+std::to_string(i++), TheFunction);

        BasicBlock *CurBB = Builder->GetInsertBlock(); // Catch branching scenarios
        Builder->CreateBr(LoopBB);
        Builder->SetInsertPoint(LoopBB);

        PHINode *idx = Builder->CreatePHI(intTy, 2);
        idx->addIncoming(const_int(0), CurBB);

        Value *ptr_gep = Builder->CreateGEP(int64Ty, ptrs, idx);
        Value *pthread_ptr = Builder->CreateLoad(int64Ty, ptr_gep);
        
        call("pthread_join_aux", {pthread_ptr});

        Value *next_idx = Builder->CreateAdd(idx, const_int(1));
        idx->addIncoming(next_idx, LoopBB);

        Value *cond = Builder->CreateICmpSLT(next_idx, count);
        Builder->CreateCondBr(cond, LoopBB, AfterBB);

        Builder->SetInsertPoint(AfterBB);
        call("free", {ptrs});
    }


    // for (Value *pthreadPtr : thread_pointers) {
    //     Value *pthread = Builder->CreateLoad(int8PtrTy, pthreadPtr);
    //     call("pthread_join_aux", {pthread});
    // }
    // thread_pointers.clear();


    call("scope_struct_Reset_Threads", {scope_struct});
    return const_float(0);
}


Value *LockExprAST::codegen(Value *scope_struct){
    call("LockMutex", {Builder->CreateGlobalString(Name)});

    for (auto &body : Bodies)
        body->codegen(scope_struct);

    call("UnlockMutex", {Builder->CreateGlobalString(Name)});

    return ConstantFP::get(*TheContext, APFloat(0.0f));
}



void SetUniques(Value *scope_struct) {

    int idx = 0;
    for (auto class_name : Global_Uniques) {
        Value *ptr = callret("allocate_pool", {scope_struct, const_int(ClassSize[class_name]),
                                const_int16(data_name_to_type()[class_name])});
        std::string function_name = "__anon_expr";
        Allocate_On_Pointer_Stack(scope_struct, function_name, class_name, Data_Tree(class_name), ptr);
        StructType *st = struct_types["class_"+class_name]; 
        for (auto attr : ClassAttrsName[class_name]) {
          Data_Tree dt = data_typeVars[class_name][attr];
          std::string type = dt.Type;
          std::string create_fn = type+"_Create";
          if (in_vec(type, compound_tokens)) {
            int attr_idx = ClassAttrs[class_name][attr];
            std::vector<Value *> ArgsDT_Create = {scope_struct};
            if (create_fn=="array_Create")
                ArgsDT_Create.push_back(const_int16(data_name_to_type()[dt.Nested_Data[0].Type]));
            else if (type=="channel") { 
                ArgsDT_Create.push_back(const_int16(data_name_to_type()[dt.Nested_Data[0].Type]));
                ArgsDT_Create.push_back(const_int(stoi(dt.Nested_Data[1].Type)));
            } else { //map
                Data_Tree *dt_ptr = new Data_Tree(type);
                dt_ptr->Nested_Data.push_back(dt.Nested_Data[0]);
                if(type=="map")
                    dt_ptr->Nested_Data.push_back(dt.Nested_Data[1]);
                ArgsDT_Create.push_back(VoidPtr_toValue(dt_ptr));
            }
            Value *compound = callret(create_fn, ArgsDT_Create);
            Value *compound_gep = Builder->CreateStructGEP(st, ptr, attr_idx);
            Builder->CreateStore(compound, compound_gep);
          }
        }
        Value *previous_obj = get_scope_obj(scope_struct);
        set_scope_obj(scope_struct, ptr);
        call(class_name+"___init__", {scope_struct});
        set_scope_obj(scope_struct, previous_obj);
        Global_Uniques_Idx[class_name] = idx++;
    }

}

Value *MainExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return const_float(0);

    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    std::string functionName = TheFunction->getName().str();

    SetUniques(scope_struct);
    for (auto &prebuild_fn : prebuild_functions)
        call(prebuild_fn, {scope_struct});


    for (const auto &body : Bodies) {
        body->codegen(scope_struct);
        if (!body)
            return const_float(1);
    }

    call("scope_struct_Join_GC", {scope_struct});

    return const_float(0);
}


Value *ExitCheckExprAST::codegen(Value *scope_struct) {
    return const_float(0.0f);
}



Value *VariableListExprAST::codegen(Value *scope_struct) {
    std::cout << "Variable list expr" << ".\n";
    return const_float(0.0f);
}



Value *RetExprAST::codegen(Value *scope_struct) {
    if (!ShallCodegen) {
        Value *ret = const_float(0.0f);
        Builder->CreateRet(ret);
        return ret;
    }


    if(Vars.size()==1) { 
        Value *ret = Vars[0]->codegen(scope_struct);

        if (returning_type.Type=="int" && return_expected_type.Type=="float")
            ret = Builder->CreateSIToFP(ret, floatTy, "lfp");

        Builder->CreateRet(ret);

        return ret;
    }


    std::vector<llvm::Type *> retTypes;
    for (int i=0; i<Vars.size(); i++)
         retTypes.push_back(get_type_from_data(Vars[i]->GetDataTree()));
    
    StructType *retTy = StructType::create(
        *TheContext,
        retTypes,
        "ret_expr_type"
    );

    Value *ret_val = UndefValue::get(retTy);

    for (int i=0; i<Vars.size(); i++)
        ret_val = Builder->CreateInsertValue(ret_val, Vars[i]->codegen(scope_struct),
                                             {static_cast<unsigned>(i)});

    Builder->CreateRet(ret_val);

    return ret_val;
}







Data_Tree NewTupleExprAST::GetDataTree(bool from_assignment) {
    if(!data_type.empty)
        return data_type;

    data_type.Type = "tuple";
    for (int i=0; i<Values.size(); i++) {
        std::string type = Values[i]->GetType();
        data_type.Nested_Data.push_back(Data_Tree(type));
    }
    data_type.empty=false;

    return data_type;
}

Value *NewTupleExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return ConstantFP::get(*TheContext, APFloat(0.0f));

    std::vector<Value *> values;

    GetDataTree();

    values.push_back(scope_struct);


    seen_var_attr = true;
    bool is_type=true;
    for (int i=0; i<Values.size(); i++) {
        Value *value = Values[i]->codegen(scope_struct);
        std::string type = Values[i]->GetType();


        if (!is_type) {
            if(!in_str(type, primary_data_tokens)) {
                std::string copy_fn = type + "_Copy";

                Function *F = TheModule->getFunction(copy_fn);
                if (F)
                    value = callret(copy_fn, {scope_struct, value});
            }
            is_type=true;
        } else
            is_type=false;
        values.push_back(value);
    }

    seen_var_attr = false;

    return callret("list_New", values);
}





Data_Tree NewVecExprAST::GetDataTree(bool from_assignment) {
    if(!data_type.empty)
        return data_type;
    // data_type.Type = "tuple";
    // for (int i=1; i<Values.size(); i=i+2) {
    //     Data_Tree type = Values[i]->GetDataTree();
    //     data_type.Nested_Data.push_back(type);
    // }
    data_type.Type = "array";
    data_type.Nested_Data.push_back(Values[1]->GetDataTree());
    data_type.empty=false;

    return data_type;
}

Value *NewVecExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return const_float(0);

    std::vector<Value *> values;
    values.push_back(scope_struct);
    

    bool is_type=true;
    for (int i=0; i<Values.size(); i++) {
        std::string type;
        Value *value;

        if (!is_type) {
            type = Values[i]->GetDataTree().Type;
            value = Values[i]->codegen(scope_struct);

            if(!in_vec(type, primary_data_tokens)) {
                std::string copy_fn = type + "_Copy";
                Function *F = TheModule->getFunction(copy_fn);
                if (F)
                    value = callret(copy_fn, {scope_struct, value});
            }
            values.push_back(value);
            is_type=true;
        } else
            is_type=false;
    }

    std::string type = data_type.Nested_Data[0].Type;
    if(!in_vec(type, primary_data_tokens)&&type!="str")
        type = "void";
    std::string Callee = "array_" + type + "_NewVec";
    return callret(Callee, values);
}


Value *NewDictExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return ConstantFP::get(*TheContext, APFloat(0.0f));

    std::vector<Value *> values;

    values.push_back(scope_struct);

    seen_var_attr = true;

    for (int i=0; i<Values.size()-1; i++)
    {
        std::string key_type = Keys[i]->GetType();
        std::string type = Values[i]->GetType();

        if (key_type!="str")
        {
            LogErrorS(parser_struct.line, "Dictionary key must be of type string");
            return const_float(0);
        } 
        Value *key = Keys[i]->codegen(scope_struct);
        values.push_back(key);

        auto element_type = std::make_unique<StringExprAST>(type);
        values.push_back(element_type->codegen(scope_struct));

        Value *value = Values[i]->codegen(scope_struct);


        if(!in_str(type, primary_data_tokens))
        {

            std::string copy_fn = type + "_Copy";

            Function *F = TheModule->getFunction(copy_fn);
            if (F)
                value = callret(copy_fn, {scope_struct, value});
        }

        values.push_back(value);
    }


    Value *value = Values[Values.size()-1]->codegen(scope_struct);
    values.push_back(value);

    seen_var_attr = false;

    return callret("dict_New", values);
}








Value *ObjectExprAST::codegen(Value *scope_struct) {
    if (not ShallCodegen)
        return ConstantFP::get(*TheContext, APFloat(0.0f));
    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    // Register all variables and emit their initializer.
    Value *previous_obj;
    bool has_init=false;



    for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
        const std::string &VarName = VarNames[i].first;
        ExprAST *Init = VarNames[i].second.get();

        if(!isSelf&&!isAttribute) {   
            Value *ptr;
            if (!Init) // no attribution
                ptr = callret("allocate_pool", {scope_struct, const_int(ClassSize[ClassName]),
                                        const_int16(data_name_to_type()[ClassName])});
            else
                ptr = Init->codegen(scope_struct);
            Allocate_On_Pointer_Stack(scope_struct, parser_struct.function_name, VarName, Data_Tree(ClassName), ptr);

            StoreVal(TheFunction, parser_struct.function_name, VarName, ptr, Data_Tree("ClassName"));


            if (HasInit[i]) { // callee init
                if(!has_init)
                    previous_obj = get_scope_obj(scope_struct);
                has_init = true;
                set_scope_obj(scope_struct, ptr);

                int arg_type_check_offset = 1;
                std::vector<Value *> ArgsV = {scope_struct};

                std::string Callee = ClassName + "___init__";
                std::vector<Data_Tree> ArgTypes;
                
                // initialize compound types
                StructType *st = struct_types["class_"+ClassName]; 
                for (auto attr : ClassAttrsName[ClassName]) {
                  Data_Tree dt = data_typeVars[ClassName][attr];
                  std::string type = dt.Type;
                  std::string create_fn = type+"_Create";
                  if (in_vec(type, compound_tokens)||type=="channel") {
                    int attr_idx = ClassAttrs[ClassName][attr];
                    std::vector<Value *> ArgsDT_Create = {scope_struct};
                    if (create_fn=="array_Create")
                        ArgsDT_Create.push_back(const_int16(data_name_to_type()[dt.Nested_Data[0].Type]));
                    else if (type=="channel") { 
                        ArgsDT_Create.push_back(const_int16(data_name_to_type()[dt.Nested_Data[0].Type]));
                        ArgsDT_Create.push_back(const_int(stoi(dt.Nested_Data[1].Type)));
                    } else { //map
                        Data_Tree *dt_ptr = new Data_Tree(type);
                        dt_ptr->Nested_Data.push_back(dt.Nested_Data[0]);
                        if(type=="map")
                            dt_ptr->Nested_Data.push_back(dt.Nested_Data[1]);
                        ArgsDT_Create.push_back(VoidPtr_toValue(dt_ptr));
                    }
                    Value *compound = callret(create_fn, ArgsDT_Create);
                    Value *compound_gep = Builder->CreateStructGEP(st, ptr, attr_idx);

                    // p2t("obj() " + ClassName);
                    MakeWriteBarrier(parser_struct, TheFunction, scope_struct, ptr, compound_gep, compound, type);
                  }
                }

                ArgsV = Codegen_Argument_List(parser_struct, std::move(ArgsV), Args[i], ArgTypes, scope_struct, \
                        Callee, false, arg_type_check_offset); 
                Set_Stack_Top(scope_struct, parser_struct.function_name);
                call(Callee, ArgsV);
            }
        }      
    }
    if(has_init) 
        set_scope_obj(scope_struct, previous_obj);
    return const_float(0.0f);
}





Value *NewExprAST::codegen(Value *scope_struct) {
    Value *nullPtr = ConstantPointerNull::get(
            cast<PointerType>(int8PtrTy)
            );

    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    std::vector<Value *> ArgsV = {scope_struct};

    if (Args.size()>0) {
        std::vector<Data_Tree> ArgTypes;
        ArgsV = Codegen_Argument_List(parser_struct, std::move(ArgsV), Args, ArgTypes, scope_struct, \
                Callee, !is_high_level_obj, 1);
    }

    if(!Check_ArgsV_Count(Callee, ArgsV, parser_struct, 1))
        return const_float(0);

    if(in_str(Callee, vararg_methods))
        ArgsV.push_back(const_int(TERMINATE_VARARG));



    if (is_high_level_obj) {
        Value *previous_obj = get_scope_obj(scope_struct);
        Value *ptr = callret("allocate_pool", {scope_struct, const_int(ClassSize[DataName]),\
                                               const_int16(data_name_to_type()[DataName])});
        StructType *st = struct_types["class_"+DataName]; 
        for (auto attr : ClassAttrsName[DataName]) {
          Data_Tree dt = data_typeVars[DataName][attr];
          std::string type = dt.Type;
          std::string create_fn = type+"_Create";
          if (in_vec(type, compound_tokens)) {
            int attr_idx = ClassAttrs[DataName][attr];
            std::vector<Value *> ArgsDT_Create = {scope_struct};
            if (create_fn=="array_Create")
                ArgsDT_Create.push_back(const_int16(data_name_to_type()[dt.Nested_Data[0].Type]));
            else if (type=="channel") { 
                ArgsDT_Create.push_back(const_int16(data_name_to_type()[dt.Nested_Data[0].Type]));
                ArgsDT_Create.push_back(const_int(stoi(dt.Nested_Data[1].Type)));
            } else { //map
                Data_Tree *dt_ptr = new Data_Tree(type);
                dt_ptr->Nested_Data.push_back(dt.Nested_Data[0]);
                if(type=="map")
                    dt_ptr->Nested_Data.push_back(dt.Nested_Data[1]);
                ArgsDT_Create.push_back(VoidPtr_toValue(dt_ptr));
            }
            Value *compound = callret(create_fn, ArgsDT_Create);
            Value *compound_gep = Builder->CreateStructGEP(st, ptr, attr_idx);
            // Builder->CreateStore(compound, compound_gep);

            // p2t("new " + DataName);
            MakeWriteBarrier(parser_struct, TheFunction, scope_struct, ptr, compound_gep, compound, type);
          }
        }
        set_scope_obj(scope_struct, ptr);
        call(Callee, ArgsV);
        set_scope_obj(scope_struct, previous_obj);
        return ptr;
    }

    std::string dt_type = "DT_" + DataName;
    if(struct_create_fn.count(dt_type)>0) {
        Value *initial_value = (struct_type_size.count(DataName)>0) ? \
                    callret("allocate_pool", {scope_struct,
                    const_int(struct_type_size[DataName]),
                    const_int16(data_name_to_type()[DataName])}) : nullptr;

        std::vector<Value*> ArgsV_slice(ArgsV.begin()+1, ArgsV.end()); // skip ctx
        return struct_create_fn[dt_type](parser_struct, TheFunction,
                "", DataName, Data_Tree(DataName),
                scope_struct, 
                initial_value,
                Args,
                ArgsV_slice); 
    }

    return callret(Callee, ArgsV);
}






Function *PrototypeAST::codegen() {
    if (!ShallCodegen)
        return nullptr;
    // Make the function type:  float(float,float) etc.

    std::vector<llvm::Type *> types;
    for (auto &type : Types) {
        llvm::Type *Ty = get_type_from_data(type);
        if (type.is_buffer)
            Ty = Ty->getPointerTo();
        types.push_back(Ty);
    }

    llvm::Type *retTy = get_type_from_data(ReturnType);
    FunctionType *FT = FunctionType::get(retTy, types, false); 


    auto linkage = (parser_struct.gpu==2) ? Function::InternalLinkage : Function::ExternalLinkage;

    auto llvm_module = (parser_struct.gpu>0) ? PtxModule.get() : TheModule.get();
    Function *F = Function::Create(FT, linkage, Name, llvm_module);

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : F->args()) {
        std::string ArgName;
        if(Idx>=Args.size())
            ArgName = std::to_string(Idx++); // proto case
        else
            ArgName = Args[Idx++];           // def case
        // std::cout << "add arg " << ArgName << " to " << Name << "\n";
        Arg.setName(ArgName);

        // if (!Arg.getType()->isPointerTy()) {
        //     continue;
        // }
        // // scope_struct
        // if (Arg.getName() == "scope_struct") {
        //     LogBlue("No alias for scope_struct");
        //     Arg.addAttr(Attribute::ReadOnly); 
        // }
        // Arg.addAttr(Attribute::NoAlias);
        // Arg.addAttr(Attribute::NonNull);
    }

    if (parser_struct.gpu==1) {
        llvm::Metadata *MDVals[] = {
            llvm::ValueAsMetadata::get(F),
            llvm::MDString::get(*TheContext, "kernel"),
            llvm::ValueAsMetadata::get(
                llvm::ConstantInt::get(
                    llvm::Type::getInt32Ty(*TheContext),
                    1))
        };

        PtxModule->getOrInsertNamedMetadata("nvvm.annotations")
            ->addOperand(llvm::MDNode::get(*TheContext, MDVals));

        F->setCallingConv(llvm::CallingConv::PTX_Kernel);
        F->addFnAttr(Attribute::NoInline);
    }
    if (parser_struct.gpu==2) {
        F->setCallingConv(llvm::CallingConv::C);
        F->addFnAttr(Attribute::AlwaysInline);
        F->setDSOLocal(true);
    }
    
    return F;
}




Value *ReduceExprAST::codegen(Value *scope_struct) {
    std::string gpu_str = (parser_struct.gpu>0)  ? "gpu_" : "";
    fn += "_" + gpu_str + functional_type + "_" + op_map[Op];
    if(parser_struct.gpu==0)
        return callret(fn, {scope_struct, LHS->codegen(scope_struct)});
    return callret(fn, {LHS->codegen(scope_struct)});
}


Value *LambdaExprAST::codegen(Value *scope_struct) {

    std::vector<llvm::Type *> types;
    for (auto &type : ArgsType) {
        llvm::Type *Ty = get_type_from_data(type);
        if (type.is_buffer)
            Ty = Ty->getPointerTo();
        types.push_back(Ty);
    }


    Data_Tree ret_dt = Body->GetDataTree();
    llvm::Type *retTy = get_type_from_data(ret_dt);
    FunctionType *FT = FunctionType::get(retTy, types, false); 


    auto linkage = (parser_struct.gpu==2) ? Function::InternalLinkage : Function::ExternalLinkage;

    auto llvm_module = (parser_struct.gpu>0) ? PtxModule.get() : TheModule.get();
    Function *F = Function::Create(FT, linkage, lambda_fn, llvm_module);
    unsigned Idx = 0;
    for (auto &Arg : F->args()) {
        std::string ArgName;
        if(Idx>=Args.size())
            ArgName = std::to_string(Idx++); // proto case
        else
            ArgName = Args[Idx++];           // def case
        Arg.setName(ArgName);
    }

    F->addFnAttr(Attribute::AlwaysInline);
    if (parser_struct.gpu==2) {
        F->setCallingConv(llvm::CallingConv::C);
        F->setDSOLocal(true);
    }

    BasicBlock *CurBB = Builder->GetInsertBlock(); 

    BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", F);
    Builder->SetInsertPoint(BB);


    int args_count = ArgsType.size();
    auto it = F->arg_begin();

    for (int i=0; i<args_count; ++i, ++it) {
        llvm::Argument &Arg = *it;

        std::string arg_name = Arg.getName().str();

        if (arg_name == "scope_struct") {
            StructType *st = struct_types["scope_struct"];
            scope_struct = &Arg;
            Value *stack_top_value_gep = Builder->CreateStructGEP(st, scope_struct, 3); 
            function_values[lambda_fn]["QQ_stack_top"] = Builder->CreateLoad(intTy, stack_top_value_gep);
            fn_stack_offset[lambda_fn] = 0;
        } else {
            function_values[lambda_fn][arg_name] = &Arg;
            StoreVal(F, lambda_fn, arg_name, &Arg,
                        data_typeVars[lambda_fn][arg_name]);
        }
       
    }

    Value *ret = Body->codegen(scope_struct);

    Builder->CreateRet(ret);
    // Builder->CreateRetVoid(); 

    // Function *TheFunction = Builder->GetInsertBlock()->getParent();
    Builder->SetInsertPoint(CurBB);

    return const_int(0);
}

Value *MapitExprAST::codegen(Value *scope_struct) {
    Lambda->codegen(scope_struct);


    if(parser_struct.gpu==0)
        return callret(fn, {scope_struct, LHS->codegen(scope_struct), getFunction(Lambda->lambda_fn)});
    else 
        return callret(fn, {LHS->codegen(scope_struct), getFunction(Lambda->lambda_fn)});
}





Value *LayoutExprAST::codegen(Value *scope_struct) {

    llvm::Type *ty = get_type_from_data(dt.Nested_Data[0]);

    Value *ptr = Args[0]->codegen(scope_struct);

    Value *offset = const_int(0);
    for (int i=0; i<Strides.size(); ++i) {
        offset = Builder->CreateAdd(offset, 
                        Builder->CreateMul(const_int(Strides[i]),
                                           Args[i+1]->codegen(scope_struct))
                    );
    }

    return Builder->CreateGEP(ty, ptr, offset);
}



Value *NameableExprAST::codegen(Value *scope_struct) {}
Value *EmptyStrExprAST::codegen(Value *scope_struct) {}


std::string Get_Nested_Name(std::vector<std::string> expressions_string_vec, Parser_Struct parser_struct, bool to_last) {

    int last = expressions_string_vec.size();
    if (!to_last)
        last--;


    std::string _class=parser_struct.function_name;

    int i=0;
    if(expressions_string_vec[i]=="self") {
        _class = parser_struct.class_name; 
        i++;
    } else if (Object_toClass[_class].count(expressions_string_vec[i])==0) {
        _class = data_typeVars[_class][expressions_string_vec[i]].Nested_Data[0].Type;
        i++;
    }


    for(; i<last; ++i)
    {

        std::string next_class = Object_toClass[_class][expressions_string_vec[i]].Type;

        if (next_class=="")
            return _class;
        _class = next_class;
    }
    return _class;
}






int Get_Object_Offset(std::vector<std::string> expressions_string_vec, Parser_Struct parser_struct) {

    int last = expressions_string_vec.size()-1;


    std::string _class=parser_struct.function_name; 

    int i=0;
    if(expressions_string_vec[i]=="self") {
        _class = parser_struct.class_name; 
        i++;
    }


    for(; i<last; ++i)
    {
        _class = Object_toClass[_class][expressions_string_vec[i]].Type;
    }
    // p2t("Offset of " + expressions_string_vec[last] + " is " + std::to_string(ClassVariables[_class][expressions_string_vec[last]]));

    return ClassVariables[_class][expressions_string_vec[last]];
}

Value *SelfExprAST::codegen(Value *scope_struct) {
    return get_scope_obj(scope_struct);
}

Value *NestedStrExprAST::codegen(Value *scope_struct) {  

    if(skip)
        return Inner_Expr->codegen(scope_struct);


    int offset;
    if(Inner_Expr->Name=="self")
    {
        Value *obj_ptr = Inner_Expr->codegen(scope_struct);

        offset = ClassVariables[parser_struct.class_name][Name];

        obj_ptr = callret("offset_object_ptr", {obj_ptr, const_int(offset)});

        std::string _type = typeVars[parser_struct.class_name][Name];
        if(_type!="int"&&_type!="float" && (!IsLeaf||Load_Last))
        {
            obj_ptr = callret("object_Load_slot", {obj_ptr});
        }

        return obj_ptr;

    } else if (height>1) { 

        Value *obj_ptr = Inner_Expr->codegen(scope_struct);


        offset = Get_Object_Offset(Expr_String, parser_struct);

        obj_ptr = callret("offset_object_ptr", {obj_ptr, const_int(offset)});


        std::string fn_name = Get_Nested_Name(Expr_String, parser_struct, false);
        std::string _type = typeVars[fn_name][Name];


        if(_type!="int"&&_type!="float" && (!IsLeaf||Load_Last))
            obj_ptr = callret("object_Load_slot", {obj_ptr});


        return obj_ptr;

    } else if (height==1) {
        std::string var_type = typeVars[parser_struct.function_name][Name];
        // std::cout << "----LOADING HEIGHT==1 ALLOCA " << Name << " OF TYPE " << var_type << " AT FUNCTION " << parser_struct.function_name << ".\n"; 
        return load_alloca(Name, var_type, parser_struct.function_name);    
    }
    else {
        LogErrorS(parser_struct.line, "uncaught");
    }
}



std::string NestedVectorIdxExprAST::GetType(bool from_assignment) {  
    return "";
}


Value *NestedVectorIdxExprAST::codegen(Value *scope_struct) {  
    return const_int(0);
}




Data_Tree NestedVariableExprAST::GetDataTree(bool from_assignment) {
    return data_type;
}

Value *NestedVariableExprAST::codegen(Value *scope_struct) {
    // std::cout << "Nested Variable Expr" << ".\n";
    // Print_Names_Str(Inner_Expr->Expr_String);

    Value *ptr = Inner_Expr->codegen(scope_struct);

    if (Load_Val&&(Type=="float"||Type=="int"))
        return callret("object_Load_"+Type, {ptr});

    return ptr;
}

Value *ViewExprAST::codegen(Value *scope_struct) {
    Value *view_val = UndefValue::get(struct_types["DT_str"]);
    Value *inner = LHS->codegen(scope_struct);
    inner = Builder->CreateExtractValue(inner, {0});
    Value *size_val = RHS->codegen(scope_struct);
    if (has_R_cast)
        size_val = Builder->CreateIntCast(size_val, intTy, true);
    view_val = Builder->CreateInsertValue(view_val, inner, {0});
    view_val = Builder->CreateInsertValue(view_val, size_val, {1});
    return view_val;
} 

Value *NameableLLVMIRCall::codegen(Value *scope_struct) {  
    int arg_type_check_offset=1, target_args_size=Args.size();
    bool is_nsk_fn = in_str(Callee, native_methods);

    // std::vector<Value*> ArgsV = {scope_struct};


    if (Callee=="pow"&&target_args_size!=2) {
        LogErrorS(parser_struct.line, "Function pow expected 2 arguments, but got " + std::to_string(target_args_size));
        return const_float(0);
    }
    if (Callee=="sqrt"&&target_args_size!=1) {
        LogErrorS(parser_struct.line, "Function sqrt expected 1 argument, but got " + std::to_string(target_args_size));
        return const_float(0);
    }


    if (ReturnType=="")
        GetDataTree();



    Value *ret;
    if(Callee=="pow") { // pow
        Value *x_value = Args[0]->codegen(scope_struct);
        if (Args[0]->GetDataTree().Type=="int")
            x_value = Builder->CreateSIToFP(x_value, Type::getFloatTy(*TheContext), "lfp");

        Value *exponent_value = Args[1]->codegen(scope_struct);
        if (Args[1]->GetDataTree().Type=="int")
            exponent_value = Builder->CreateSIToFP(exponent_value, Type::getFloatTy(*TheContext), "lfp");

        ret = Builder->CreateBinaryIntrinsic(Intrinsic::pow, x_value, exponent_value);
    } else if (Callee=="sqrt") { // sqrt
        Value *x_value = Args[0]->codegen(scope_struct);
        if (Args[0]->GetDataTree().Type=="int")
            x_value = Builder->CreateSIToFP(x_value, Type::getFloatTy(*TheContext), "lfp");

        ret = Builder->CreateUnaryIntrinsic(Intrinsic::sqrt, x_value);
    } else
        LogErrorS(-1, "LLVM IR Function " + Callee + " not implemented");



    if(ReturnType=="void_ptr")
        LogErrorS(-1, "return " + Callee);  

    return ret;
}



Value *NestedCallExprAST::codegen(Value *scope_struct) {
    return const_float(0);
}
Value *NameableRoot::codegen(Value *scope_struct) {
    return const_float(0);
}

Value *RecoverUniqueGlobal(Value *scope_struct, std::string name) {
    // pointer to [N x i8*]
    Value *stack_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 2);
    // element pointer: &pointers_stack[i]
    // {0, i} first index the array object, then the element
    Value *void_ptr_gep = Builder->CreateGEP(ArrayType::get(int8PtrTy, ContextStackSize), stack_gep, { const_int(0), const_int(Global_Uniques_Idx[name]) });
    Value *ret = Builder->CreateLoad(int8PtrTy, void_ptr_gep);
    return  ret;
}

Value *Nameable::codegen(Value *scope_struct) {
    if (!ShallCodegen)
        return const_float(0.0f);
    Data_Tree dt = GetDataTree();
    std::string type = dt.Type;

    if(Depth==1) {
        if(IsUnique)
            return RecoverUniqueGlobal(scope_struct, Name);
        if(Name=="self")
            return get_scope_obj(scope_struct);
        // buffers
        if (function_allocas[parser_struct.function_name].count(Name)>0)
            return function_allocas[parser_struct.function_name][Name]; 
        if (function_values[parser_struct.function_name].count(Name)>0)
            return function_values[parser_struct.function_name][Name];
        if (Name=="tid") {
            Value *tid_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 1);
            Value *tid = Builder->CreateSub(Builder->CreateLoad(intTy, tid_gep), const_int(1));
            function_values[parser_struct.function_name]["tid"] = tid;
            return tid;
        }
        if (Name=="tN") {
            Value *tN_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 7);
            Value *tN = Builder->CreateLoad(intTy, tN_gep);
            function_values[parser_struct.function_name]["tN"] = tN;
            return tN;
        }
        if (Name=="tHW") {
            Value *tHW = callret("tHW_fn", {scope_struct});
            function_values[parser_struct.function_name]["tHW"] = tHW;
            return tHW;
        }
        return getFunctionCheck(Name);
    }

    std::string scope = Inner->GetDataTree().Type;
    Value *obj_ptr = Inner->codegen(scope_struct);


    StructType *st = struct_types["class_"+scope]; 

    Data_Tree attr_type = data_typeVars[scope][Name];
    int offset = ClassVariables[scope][Name];
    int attr_idx = ClassAttrs[scope][Name];


    // p2t(Name + "|" + scope + ": " + std::to_string(attr_idx) + " -- " + std::to_string(offset) + " / " + std::to_string(ClassSize[scope]));

    obj_ptr = Builder->CreateStructGEP(st, obj_ptr, attr_idx);

    // if (Name=="ptr"){
    //     p2t("ptr");
    //     call("print_void_ptr", {obj_ptr});
    // }

    if(Load_Last||!IsLeaf) {
        llvm::Type *Ty = get_type_from_data(attr_type);
        obj_ptr = (!attr_type.is_array) \
                    ? Builder->CreateLoad(Ty, obj_ptr) \
                    : Builder->CreateInBoundsGEP(Ty, obj_ptr, {const_int(0), const_int(0)}); //&arr[0]
    }
    // if (Name=="dims"){
    //     p2t("dims post");
    //     call("print_void_ptr", {obj_ptr});
    // }

    return obj_ptr;
}









Value *NameableIdx::codegen(Value *scope_struct) {
    Data_Tree inner_dt = Inner->GetDataTree();



    std::string compound_type = UnmangleVec(inner_dt);
    std::string type;
    if (compound_type=="tuple") {
        if (IntExprAST *expr = dynamic_cast<IntExprAST*>(Idx->Idxs[0].start.get())) {
            int idx = expr->Val;
            type = inner_dt.Nested_Data[idx].Type;
        }
    }  
    else if(in_vec(compound_type, compound_tokens)||ends_with(compound_type, "_vec")) {
        if (inner_dt.Nested_Data.size()==0) {
            if(compound_type=="list")
                type = "any";
            else
                return LogErrorV(parser_struct.line, "Missing " + compound_type + " nested type.");
        } else
            type = inner_dt.Nested_Data[0].Type;
    }
    else
        type = compound_type;


    Value *loaded_var = Inner->codegen(scope_struct);
    Value *idx = Idx_Calc_Codegen(compound_type, loaded_var, Idx, scope_struct);

    if (inner_dt.is_array) {
        llvm::Type *Ty = get_type_from_data(Data_Tree(inner_dt.Type));
        Value *gep = Builder->CreateInBoundsGEP(Ty, loaded_var, idx); //&arr[0]
        return Builder->CreateLoad(Ty, gep);
    }
    if (inner_dt.is_buffer) {
        llvm::Type *Ty = get_type_from_data(Data_Tree(inner_dt.Type));
        Value *gep = Builder->CreateGEP(Ty, loaded_var, idx); //&arr[0]
        return Builder->CreateLoad(Ty, gep);
    }

    if (compound_type=="charv") {
        Value *charv_gep = Builder->CreateInBoundsGEP(int8Ty, loaded_var, idx); //&arr[0]
        return Builder->CreateLoad(int8Ty, charv_gep);
    }

    if (compound_type=="str") {
        Value *ptr = Builder->CreateExtractValue(loaded_var, {0});
        Value *char_gep = Builder->CreateInBoundsGEP(int8Ty, ptr, idx); //&arr[0]
        return Builder->CreateLoad(int8Ty, char_gep);
    }
    


    if (compound_type == "map") {
        Value *query = idx;

        StructType *st = struct_types["map"];
        StructType *st_node = struct_types["map_node"];

        Value *nullPtr = ConstantPointerNull::get(
                    cast<PointerType>(int8PtrTy)
                );

        Data_Tree key_dt = inner_dt.Nested_Data[0], val_dt = inner_dt.Nested_Data[1];
        std::string key_type = key_dt.Type;
        std::string value_type = val_dt.Type;
        if (query->getType()==intTy&&key_type=="float")
            query = Builder->CreateSIToFP(query, floatTy);
        else if(Idx->GetDataTree().Type!=key_type)
            return LogErrorV(parser_struct.line, "Querying " + key_type + " map with " + Idx->GetDataTree().Type);


        Value *capacity_gep = Builder->CreateStructGEP(st, loaded_var, 1);
        Value *map_capacity = Builder->CreateLoad(intTy, capacity_gep);

        Value *nodes_gep = Builder->CreateStructGEP(st, loaded_var, 5);
        Value *nodes = Builder->CreateLoad(int8PtrTy->getPointerTo(), nodes_gep);

        Function *TheFunction = Builder->GetInsertBlock()->getParent();

        BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "map.loop.bb", TheFunction);
        BasicBlock *NextPtrBB = BasicBlock::Create(*TheContext, "map.next_ptr.bb", TheFunction);
        BasicBlock *FromNullBB = BasicBlock::Create(*TheContext, "map.from_null.bb", TheFunction);
        BasicBlock *GetKeyBB = BasicBlock::Create(*TheContext, "map.get_key.bb", TheFunction);
        BasicBlock *LoadValBB = BasicBlock::Create(*TheContext, "map.get_val.bb", TheFunction);

         

        Value *query_hash;
        if (key_type=="str") {
            // query = CopyStrVal(scope_struct, query);
            query_hash = str_view_llvm_hash(query, TheFunction); 
        }
        if (key_type=="float")
            query_hash = float_llvm_hash(query);
        if (key_type=="int")
            query_hash = query;
        if (key_type=="array")
            query_hash = callret("hash_array_"+key_dt.Nested_Data[0].Type, {scope_struct, query});
        if (in_vec(key_type,int_types) && key_type!="int")
            query_hash = Builder->CreateIntCast(query, intTy, true);


        Value *hash_pos = Builder->CreateURem(query_hash, map_capacity);

        Value *node_gep = Builder->CreateGEP(int8PtrTy, nodes, hash_pos);
        Value *node = Builder->CreateLoad(int8PtrTy, node_gep);
        BasicBlock *curBB = Builder->GetInsertBlock();


        // Verify node loop
        Builder->CreateBr(LoopBB);

        Builder->SetInsertPoint(LoopBB);
        PHINode *cur_node = Builder->CreatePHI(int8PtrTy, 1);
        cur_node->addIncoming(node, curBB);

        Value *IsNull = Builder->CreateICmpEQ(cur_node, nullPtr);
        Builder->CreateCondBr(IsNull, FromNullBB, GetKeyBB);


        // Get Key
        Builder->SetInsertPoint(GetKeyBB);
        Value *key_gep = Builder->CreateStructGEP(st_node, cur_node, 0);
        Value *keyCond, *key;
        if (key_type=="int"||key_type=="i64") {
            llvm::Type *ty = get_type_from_data(Data_Tree(key_type));
            Value *key_void_ptr = Builder->CreateLoad(int8PtrTy, key_gep);
            Value *key_int_ptr = Builder->CreateBitCast(key_void_ptr, ty->getPointerTo());
            key = Builder->CreateLoad(ty, key_int_ptr);
            keyCond = Builder->CreateICmpEQ(key, query);
        } else if (key_type=="float") {
            Value *key_void_ptr = Builder->CreateLoad(int8PtrTy, key_gep);
            Value *key_float_ptr = Builder->CreateBitCast(key_void_ptr, floatTy->getPointerTo());
            key = Builder->CreateLoad(floatTy, key_float_ptr);
            keyCond = Builder->CreateFCmpUEQ(key, query);
        } else if (key_type=="array") {
            key = Builder->CreateLoad(int8PtrTy, key_gep);
            keyCond = callret("array_eq_"+key_dt.Nested_Data[0].Type, {scope_struct, query, key});
        } else {
            key = Builder->CreateLoad(int8PtrTy, key_gep);
            keyCond = strview_to_strcmp(scope_struct, query, key);
        }
        Builder->CreateCondBr(keyCond, LoadValBB, NextPtrBB);

        // Get next node
        Builder->SetInsertPoint(NextPtrBB);
        Value *next_node_gep = Builder->CreateStructGEP(st_node, cur_node, 2);
        Value *next_node = Builder->CreateLoad(int8PtrTy, next_node_gep);
        cur_node->addIncoming(next_node, NextPtrBB);
        Builder->CreateBr(LoopBB);

        // From Null
        Builder->SetInsertPoint(FromNullBB);
        call("map_bad_key_"+key_type, {scope_struct, query});
        Builder->CreateUnreachable();

        // Get node value
        Builder->SetInsertPoint(LoadValBB);

        Value *value_gep = Builder->CreateStructGEP(st_node, cur_node, 1);
        Value *value;
        if (value_type=="int") {
            Value *value_void_ptr = Builder->CreateLoad(int8PtrTy, value_gep);
            Value *value_int_ptr = Builder->CreateBitCast(value_void_ptr, intTy->getPointerTo());
            value = Builder->CreateLoad(intTy, value_int_ptr);
        } else if (value_type=="float") {
            Value *value_void_ptr = Builder->CreateLoad(int8PtrTy, value_gep);
            Value *value_float_ptr = Builder->CreateBitCast(value_void_ptr, floatTy->getPointerTo());
            value = Builder->CreateLoad(floatTy, value_float_ptr);
        } else
            value = Builder->CreateLoad(int8PtrTy, value_gep);


        return value;
    }



    if(Idx->idx_slice_or_query=="query")
        return callret(compound_type+"_Query", {scope_struct, loaded_var, idx});

    auto &indices = Idx->Idxs;
    if (compound_type=="array") {

        Data_Tree elem_dt = Inner->GetDataTree().Nested_Data[0];
        llvm::Type *elemTy = get_type_from_data(elem_dt); 

        // v[i:j]
        if (indices[0].is_slice) {
            Value *start = CalcArrayIdx(scope_struct, loaded_var, indices[0].start);
            Value *end = CalcArrayIdx(scope_struct, loaded_var, indices[0].end);

            return callret("array_slice", {scope_struct, loaded_var,\
                    const_int16(data_name_to_type()[elem_dt.Type]),\
                    start, end});
        }

        // v[i]
        Check_Is_Array_Inbounds(parser_struct, loaded_var, idx);
        Value *vec = Load_Array(parser_struct.function_name, loaded_var);
        Value *element = Builder->CreateGEP(elemTy, vec, idx);
        return Builder->CreateLoad(elemTy, element, "elem"); 
    }

    std::string idx_fn = compound_type + "_Idx";
    if (TheModule->getFunction(idx_fn))
        return callret(idx_fn, {scope_struct, loaded_var, idx});

    LogError(parser_struct.line, "Could not handle idx expression for " + compound_type);
}


Value *PositionalArgExprAST::codegen(Value *scope_struct) {
    return Inner->codegen(scope_struct);
}


inline bool Check_Args_Count(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> &Args,
        int target_args_size, Parser_Struct parser_struct) {
    Function *CalleeF;
    CalleeF = getFunction(Callee);

    if (Callee=="list_append")
        target_args_size++;
    if (!CalleeF)
    {
        std::string _error = "The referenced function "+ Callee +" was not yet declared.";
        LogErrorV(parser_struct.line, _error);
        return false;
    }

    // If argument mismatch error.
    if ((CalleeF->arg_size()) != target_args_size && !in_str(Callee, vararg_methods))
    {
        // std::cout << "CalleeF->arg_size() " << std::to_string(CalleeF->arg_size()) << " target_args_size " << std::to_string(target_args_size) << "\n";
        std::string _error = "Incorrect parameters used on function " + Callee + " call.\n\t    Expected " +  std::to_string(CalleeF->arg_size()-1) + " arguments, got " + std::to_string(target_args_size-1);
        LogErrorV(parser_struct.line, _error);
        return false;
    }
    return true;
}

Value *NameableAppend::codegen(Value *scope_struct) {  

    Value *loaded_var = Inner->codegen(scope_struct);

    Value *appended_val = Args[0]->codegen(scope_struct);

    std::string elem_type = inner_dt.Nested_Data[0].Type;


    if (inner_dt.Type=="array") {
        Function *TheFunction = Builder->GetInsertBlock()->getParent();
        BasicBlock *good_sizeBB = BasicBlock::Create(*TheContext, "array.append.ok_size", TheFunction);
        BasicBlock *bad_sizeBB = BasicBlock::Create(*TheContext, "array.append.bad_size", TheFunction);
        BasicBlock *postBB = BasicBlock::Create(*TheContext, "array.append.post", TheFunction);

        if (elem_type=="float"&&Args[0]->GetDataTree().Type=="int")
            appended_val = Builder->CreateSIToFP(appended_val, floatTy, "lfp");


        StructType *st = struct_types["scope_struct"];
        llvm::Type *elemTy;
        Value *vec = Load_Array(parser_struct.function_name, loaded_var);

        if (!in_vec(elem_type, primary_data_tokens)) {
            Value *gc_gep = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 5);
            Value *gc = Builder->CreateLoad(int8PtrTy, gc_gep);
            Value *marking = Builder->CreateLoad(boolTy,
                                        Builder->CreateStructGEP(struct_types["GC"], gc, 3));

            BasicBlock *MarkingBB = BasicBlock::Create(*TheContext, "array.marking", TheFunction);
            BasicBlock *StandardBB = BasicBlock::Create(*TheContext, "array.standard", TheFunction);
            
            Builder->CreateCondBr(marking, MarkingBB, StandardBB);
            Builder->SetInsertPoint(MarkingBB);

            if (elem_type=="str") {
                Value *str  = Builder->CreateExtractValue(appended_val, {0});
                Value *size = Builder->CreateExtractValue(appended_val, {1});
                call("GC_array_append_str_barrier",
                     {const_int16(data_name_to_type()[elem_type]), scope_struct, loaded_var, str, size});
            } else
                call("GC_array_append_barrier",
                     {const_int16(data_name_to_type()[elem_type]), scope_struct, loaded_var, appended_val});
            Builder->CreateBr(postBB);

            Builder->SetInsertPoint(StandardBB);
        }

        elemTy = get_type_from_str(elem_type); 


        Value *vsize_gep = Builder->CreateStructGEP(st, loaded_var, 0);
        Value *vsize = Builder->CreateLoad(intTy, vsize_gep);

        Value *size_gep = Builder->CreateStructGEP(st, loaded_var, 1);
        Value *size = Builder->CreateLoad(intTy, size_gep);





        Value *Cond = Builder->CreateICmpSLT(vsize, size);
        Builder->CreateCondBr(Cond, good_sizeBB, bad_sizeBB);


        //good size
        Builder->SetInsertPoint(good_sizeBB);
        Value *elem_gep = Builder->CreateGEP(elemTy, vec, vsize); 
        Builder->CreateStore(appended_val, elem_gep);
        Value *next_vsize = Builder->CreateAdd(vsize, const_int(1));
        Builder->CreateStore(next_vsize, vsize_gep);
        Builder->CreateBr(postBB);


        //bad size (thus double array size)
        Builder->SetInsertPoint(bad_sizeBB);
        call("array_double_size", {scope_struct, loaded_var}); 
        vec = Load_Array(parser_struct.function_name, loaded_var);
        elem_gep = Builder->CreateGEP(elemTy, vec, vsize); 
        Builder->CreateStore(appended_val, elem_gep);
        next_vsize = Builder->CreateAdd(vsize, const_int(1));
        Builder->CreateStore(next_vsize, vsize_gep);
        Builder->CreateBr(postBB);


        //post
        Builder->SetInsertPoint(postBB); 
        // std::cout << "append post: " << parser_struct.function_name  << " " << postBB << "\n";
    }


    return const_float(0.0f);
}



Value *getValAddress(Function *TheFunction, Value *val, Data_Tree dt, int i) {
    if (dt.is_buffer||dt.is_array)
        return val;

    // Needs stack address encapsulation for a register value
    llvm::Type *Ty = get_type_from_data(dt);

    Value *alloca = CreateEntryBlockAlloca(TheFunction, "args_"+std::to_string(i),
            Ty);
    Builder->CreateStore(val, alloca);

    Value *address = Builder->CreateInBoundsGEP(
        Ty,
        alloca,
        {const_int(0), const_int(0)}
    );
    
    return address;
}


Value *callgpu(Function *TheFunction, std::string fn,
                      Value *gx, Value *gy, Value *gz, Value *bx, Value *by, Value *bz,
                      const std::vector<Value *> &args, std::vector<Data_Tree> &Types) {

    auto ptx = EmitPtx();
    // PtxModule->print(llvm::errs(), nullptr);


    // Encapsulate within void **args
    int nargs = args.size();

    ArrayType *ArgArrayTy = ArrayType::get(int8PtrTy, nargs);
    Value *ArgArray = CreateEntryBlockAlloca(TheFunction, fn+"_args", ArgArrayTy);

    for (size_t i = 0; i < nargs; ++i) {
        Value *ElemPtr = Builder->CreateInBoundsGEP(
            ArgArrayTy, ArgArray,
            {const_int(0), const_int(i)}
        );

        Builder->CreateStore(getValAddress(TheFunction, args[i], Types[i], i), ElemPtr);
    }

    Value *ArgBase = Builder->CreateInBoundsGEP(
        ArgArrayTy, ArgArray,
        {const_int(0), const_int(0)}
    );
    

    call("neve_gpu_launch", {global_str(fn), global_str(ptx),
                                gx, gy, gz, bx, by, bz,
                                ArgBase
                             });

    return const_float(0);
}

Value *LaunchExprAST::codegen(Value *scope_struct) {
    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    std::cout << "launch codegen" << "\n";

    std::vector<Value*> ArgsV = {scope_struct};
    std::vector<Data_Tree> ArgTypes;

    ArgsV = Codegen_Argument_List(parser_struct, std::move(ArgsV), Args, ArgTypes,\
            scope_struct,\
            fn_name, false, 0);


    if(kernel_fn.count(fn_name)==0)
        LogErrorC(parser_struct.line, "Kernel " + fn_name + " not found.");
    

    Value *grid = Grid->codegen(scope_struct);
    Value *block = Block->codegen(scope_struct);
    call("array_print_int",  {scope_struct, grid});
    call("array_print_int",  {scope_struct, block});

    Value *grid_gep = Builder->CreateStructGEP(struct_types["array"], grid, 3);
    Value *block_gep = Builder->CreateStructGEP(struct_types["array"], block, 3);
    Value *grid_v = Builder->CreateLoad(int8PtrTy, grid_gep);
    Value *block_v = Builder->CreateLoad(int8PtrTy, block_gep);

    

    Value *bx, *by, *bz, *tx, *ty, *tz;
    bx = Builder->CreateLoad(intTy, Builder->CreateGEP(intTy, grid_v, const_int(0)));
    by = Builder->CreateLoad(intTy, Builder->CreateGEP(intTy, grid_v, const_int(1)));
    bz = Builder->CreateLoad(intTy, Builder->CreateGEP(intTy, grid_v, const_int(2)));
    tx = Builder->CreateLoad(intTy, Builder->CreateGEP(intTy, block_v, const_int(0)));
    ty = Builder->CreateLoad(intTy, Builder->CreateGEP(intTy, block_v, const_int(1)));
    tz = Builder->CreateLoad(intTy, Builder->CreateGEP(intTy, block_v, const_int(2)));



    std::vector<Value*> ArgsV_slice(ArgsV.begin()+1, ArgsV.end()); // skip ctx
    callgpu(TheFunction, fn_name,
            bx, by, bz, tx, ty, tz,
            ArgsV_slice, ArgTypes);
    return const_int(0);
}

Value *NameableCall::codegen(Value *scope_struct) {  

    Function *TheFunction = Builder->GetInsertBlock()->getParent();

    int target_args_size=Args.size()+1;

    Value *previous_obj, *previous_stack_top;

    bool may_allocate = ((!is_nsk_fn||Callee=="scope_struct_Sweep")&&\
                          gpu_fn.count(Callee)==0&&llvm_callee.count(Callee)==0);

    if (may_allocate) {
        // Recovers the stack top value for the shadow stack (similar to assembly)
        // Also, prevents the case in which it allocates a slot for an argument

        previous_stack_top = Load_Stack_Top(parser_struct.function_name);
        // if (!in_vec(parser_struct.function_name, {"BPE_train", "BPE_get_buff", "BPE_get_masks"})) {
        //     p2t("----------------");
        //     Value *stack_top_value = function_values[parser_struct.function_name]["QQ_stack_top"];
        //     Value *offset =const_int(fn_stack_offset[parser_struct.function_name]);
        //     p2t("load as " + parser_struct.function_name + " to " + Callee);
        //     call("print_int", {previous_stack_top});
        //     call("print_int", {stack_top_value});
        //     call("print_int", {offset});
        //     p2t("----------------");
        // }
        Set_Stack_Top(scope_struct, parser_struct.function_name);
    }


    std::vector<Value*> ArgsV = {scope_struct};
    std::vector<Data_Tree> ArgTypes;

    Value *obj_ptr;
    bool shall_swap = false;
    if(Depth>1&&!FromLib) {
        if (ends_with(Callee, "__init__")&&isSelf) {
            Inner->IsLeaf=true;
            Inner->Load_Last=false; // inhibits Load_slot
        }

        obj_ptr = Inner->codegen(scope_struct);

        if(!is_nsk_fn)
            shall_swap=true;
        else {
            ArgTypes.push_back(Inner->GetDataTree());
            ArgsV.push_back(obj_ptr);
            target_args_size++;
            
            std::string type = Inner->GetDataTree().Type;
            if(!in_vec(type, {"vec", "str"})&&!in_vec(type, int_types)) {
                BasicBlock *GotNullBB = BasicBlock::Create(*TheContext, "nested_call.bad.bb", TheFunction);
                BasicBlock *AfterBB = BasicBlock::Create(*TheContext, "nested_call.ok.bb", TheFunction);

                Value *nullPtr = ConstantPointerNull::get(
                        cast<PointerType>(int8PtrTy)
                        );

                Builder->CreateCondBr(Builder->CreateICmpEQ(obj_ptr, nullPtr), GotNullBB, AfterBB);  

                Builder->SetInsertPoint(GotNullBB);
                call("LogErrorCall", {const_int(parser_struct.line), global_str("Could not call " + Callee + ", got a nullptr as object")});
                Builder->CreateUnreachable();

                Builder->SetInsertPoint(AfterBB);
            }
        }
    }
    // if(!Check_Args_Count(Callee, Args, target_args_size, parser_struct))
    //   return const_float(0);

    if (ReturnType=="")
        GetDataTree();

    if (!is_first_citizen)
        ArgsV = Codegen_Argument_List(parser_struct, std::move(ArgsV), Args, ArgTypes,\
                scope_struct,\
                Callee, is_nsk_fn, arg_type_check_offset);
    else {
        for (int i=0; i<Args.size(); ++i)
            ArgsV.push_back(Args[i]->codegen(scope_struct));
    }
    if (shall_swap)
        previous_obj = swap_scope_obj(scope_struct, obj_ptr); 

    if (has_obj_overwrite) // last time that cur fn values are used
        cur_self = nullptr;



  Value *ret;
  if (llvm_callee.count(Callee)>0) {
    std::vector<Value*> ArgsV_slice(ArgsV.begin()+1, ArgsV.end()); // skip ctx
    ret = llvm_callee[Callee](parser_struct, TheFunction, Callee, data_type, ArgTypes,
                              scope_struct, Args, ArgsV_slice);
  }
  else if (is_first_citizen) {
    Data_Tree inner_dt = Inner->GetDataTree();
    Value *fn = Inner->codegen(scope_struct);
    std::vector<llvm::Type*> types = {int8PtrTy};
    for (int i=1; i<inner_dt.Nested_Data.size(); ++i)
        types.push_back(get_type_from_data(inner_dt.Nested_Data[i]));
    llvm::FunctionType *fnTy =
        llvm::FunctionType::get(
            get_type_from_data(inner_dt.Nested_Data[0]),
            types,
            false
        );
    ret = Builder->CreateCall(fnTy, fn, ArgsV);
  } else {
    if (gpu_fn.count(Callee)>0) {
        std::vector<Value*> ArgsV_slice(ArgsV.begin()+1, ArgsV.end()); // skip ctx
        ret = callret(Callee, ArgsV_slice);
    } else
        ret = callret(Callee, ArgsV);
  }
  
  cur_self = nullptr;

  if (has_obj_overwrite) // Retrieve previous object
    set_scope_obj(scope_struct, previous_obj);
  if (may_allocate)
      Set_Stack_Top(scope_struct, parser_struct.function_name);
  

  // if(ReturnType=="")
  //   when it is void

  if(ReturnType=="void_ptr")
    LogErrorS(-1, "return " + Callee);


  if(begins_with(Callee, "map_get_")) {

    Value *packed_val = Builder->CreateExtractValue(ret, {0});
    Value *packed_bool = Builder->CreateExtractValue(ret, {1});
    packed_bool = Builder->CreateIntCast(packed_bool, boolTy, true);

    ret = Builder->CreateInsertValue(ret, packed_bool, {1});
    Data_Tree dt = GetDataTree().Nested_Data[0];

    StructType *structTy = StructType::create(
        *TheContext,
        {get_type_from_data(dt),boolTy},
        "tulpe_ty"
    );
    Value *struct_ret = UndefValue::get(structTy);
    struct_ret = Builder->CreateInsertValue(struct_ret, packed_val, {0});
    struct_ret = Builder->CreateInsertValue(struct_ret, packed_bool, {1});
    ret = struct_ret;
  }

  return ret;
}





