#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IntrinsicsNVPTX.h>
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include <llvm/IR/InlineAsm.h>

#include <string>
#include <map>
#include <unordered_map>
#include <vector>


#include <filesystem>
#include <fstream>


#include "../runtime/common/extension_functions.h"
#include "../runtime/data_types/include.h"
#include "../compiler_frontend/codegen.h"
#include "../simd/codegen.h"
#include "../KaleidoscopeJIT.h"

#include "include.h"




using namespace llvm;
namespace fs = std::filesystem;


inline void printTy(Value *v) {
    llvm::Type *ty = v->getType();
    ty->print(llvm::errs());
    llvm::errs() << "\n";
}
inline void bb_name(BasicBlock *bb) {
    errs() << "bb: " << bb->getName() << "\n";
}
inline void v_ir(Value *v) {
    v->print(errs());
    errs() << "\n";
}

Data_Tree min_ret_dt(Parser_Struct * parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args) {
  return Args[0]->GetDataTree();
}
Data_Tree max_ret_dt(Parser_Struct * parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args) {
  return Args[0]->GetDataTree();
}
Data_Tree array_clone_dt(Parser_Struct * parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  Data_Tree dt = Data_Tree("array");
  dt.Nested_Data.push_back(inner->GetDataTree().Nested_Data[0]);
  return dt;
}
Data_Tree array_pop_dt(Parser_Struct * parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  return inner->GetDataTree().Nested_Data[0];
}
Data_Tree map_keys_dt(Parser_Struct * parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  Data_Tree dt = Data_Tree("array");
  dt.Nested_Data.push_back(inner->GetDataTree().Nested_Data[0]);
  return dt;
}
Data_Tree map_values_dt(Parser_Struct * parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  Data_Tree dt = Data_Tree("array");
  dt.Nested_Data.push_back(inner->GetDataTree().Nested_Data[1]);
  return dt;
}
Data_Tree map_get_dt(Parser_Struct * parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  Data_Tree dt = Data_Tree("tuple");
  dt.Nested_Data.push_back(inner->GetDataTree().Nested_Data[1]);
  dt.Nested_Data.push_back(Data_Tree("bool"));
  return dt;
}

Value *DT_charv_Create(Parser_Struct * parser_struct, Function *TheFunction,
                      std::string name, std::string type, Data_Tree data_type,
                      Value *scope_struct, Value *initial_value,
                      std::vector<std::unique_ptr<ExprAST>> &Args,
                      std::vector<Value*> &ArgsV) { 
    int size;
    if (auto num_expr = dynamic_cast<IntExprAST*>(Args[0].get()))
        size = num_expr->Val;

    llvm::Type *charTy = ArrayType::get(int8Ty, size);
    AllocaInst *charv = CreateEntryBlockAlloca(TheFunction, "charv", charTy);

    function_allocas[parser_struct->function_name][name] = charv;
    
    return Builder->CreateInBoundsGEP(
        charTy,
        charv,
        {const_int(0), const_int(0)}
    );
}


Value *DT_vec_Create(Parser_Struct * parser_struct, Function *TheFunction,
                      std::string name, std::string type, Data_Tree data_type,
                      Value *scope_struct, Value *initial_value,
                      std::vector<std::unique_ptr<ExprAST>> &Args,
                      std::vector<Value*> &ArgsV) { 
    int size;
    llvm::Type *ty;
    Data_Tree dt;
    
    if (auto num_expr = dynamic_cast<IntExprAST*>(Args[0].get())) {
        size = num_expr->Val;
        const std::string &data_type = data_type_to_name()[num_expr->Val];
        dt = Data_Tree(data_type);
        ty = get_type_from_data(dt);
    } else
        LogError(parser_struct->line, "Vec expected type");
    if (auto num_expr = dynamic_cast<IntExprAST*>(Args[1].get()))
            size = num_expr->Val;
    else
        LogError(parser_struct->line, "Vec expected size");

    llvm::Type *vecTy = VectorType::get(ty, size, false);
    Value *vec = llvm::UndefValue::get(vecTy);
    return vec;
}


Value *print_bb(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    bb_name(Builder->GetInsertBlock());
    return const_int(0);
}Value *print_Value(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    v_ir(ArgsV[0]);
    return const_int(0);
}

Value *to_char(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, int_types))
        LogError(parser_struct->line, "Cannot cast " + type + " to i8.");
    return Builder->CreateIntCast(ArgsV[0], int8Ty, true); // true for signed extend
}
Value *i8(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, {"int", "i16", "i64"}))
        LogError(parser_struct->line, "Cannot cast " + type + " to i8.");
    bool is_signed = type!="char";
    return Builder->CreateIntCast(ArgsV[0], int8Ty, is_signed); // true for signed extend
}
Value *i16(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, {"int", "i8", "i64", "char"}))
        LogError(parser_struct->line, "Cannot cast " + type + " to i16.");
    bool is_signed = type!="char";
    return Builder->CreateIntCast(ArgsV[0], int16Ty, is_signed); // true for signed extend
}
Value *to_int(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, {"i8", "i16", "i64", "char"}))
        LogError(parser_struct->line, "Cannot cast " + type + " to int.");
    bool is_signed = type!="char";
    return Builder->CreateIntCast(ArgsV[0], intTy, is_signed);
}
Value *i64(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, {"int", "i16", "i8"}))
        LogError(parser_struct->line, "Cannot cast " + type + " to i64.");
    return Builder->CreateIntCast(ArgsV[0], int64Ty, true); // true for signed extend
}



Value *bf16(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    // const std::string &type = Args[0]->GetDataTree().Type;
    // if(type=="int") 
    //     return float_to_bf16(Builder->CreateSIToFP(ArgsV[0], floatTy));
    return const_int(0);
}

Value *to_float(Parser_Struct * parser_struct,
                     Function *TheFunction,
                     std::string Callee,
                     Data_Tree data_type,
                     std::vector<Data_Tree> &args_type,
                     Value *scope_struct,
                     std::vector<std::unique_ptr<ExprAST>> &Args,
                     std::vector<Value*> &ArgsV) {

    IRBuilder<> &B = *Builder;
    Value *v = ArgsV[0];


    const std::string &type = Args[0]->GetDataTree().Type;

    if (type == "bf16") {
        // bf16 is uint16_t
        Value *u32 = B.CreateZExt(v, intTy);

        // restore IEEE float layout
        Value *shifted = B.CreateShl(u32, B.getInt32(16));

        // reinterpret bits as float
        Value *f32 = B.CreateBitCast(shifted, floatTy);
        return f32;
    }

    else
        LogError(parser_struct->line,
                 "Cannot cast " + type + " to float from bf16.");

}

Value *dsize(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    return callret("get_size", {scope_struct, ArgsV[0]});
}
Value *fexists(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *str = Builder->CreateExtractValue(ArgsV[0], {0});
    return callret("fexists_C", {scope_struct, str});
}
Value *c_open(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *str = Builder->CreateExtractValue(ArgsV[0], {0});

    BasicBlock *ErrBB = BasicBlock::Create(*TheContext, "file_read.good", TheFunction);
    BasicBlock *GoodBB = BasicBlock::Create(*TheContext, "file_read.no_file", TheFunction);

    Value *does_fexist = callret("fexists_C", {scope_struct, str});

    Builder->CreateCondBr(does_fexist, GoodBB, ErrBB);

    Builder->SetInsertPoint(ErrBB);
    Value *err_msg = callret("ConcatStr", {scope_struct, global_str("File "), str});
    err_msg = callret("ConcatStr", {scope_struct, err_msg, global_str(" not found.")});
    call("LogErrorCall", {const_int(parser_struct->line), err_msg});
    Builder->CreateUnreachable();

    Builder->SetInsertPoint(GoodBB);
    
    return callret("open", {str, const_int(0)});
}

Value *c_read(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *src = ArgsV[1];
    if (args_type[1].Type=="str")
        src = Builder->CreateExtractValue(src, {0});
    
    return callret("read", {ArgsV[0], src, ArgsV[2]});
}

Value *c_malloc(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    return callret("malloc", {ArgsV[0]});
}
Value *c_malloc32(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *size = ArgsV[0];
    return callret("aligned_alloc", {const_int64(32), size});
}
Value *c_malloc64(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *size = ArgsV[0];
    return callret("aligned_alloc", {const_int64(64), size});
}


Value *c_malloc_str(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *view_val = UndefValue::get(struct_types["DT_str"]);
    view_val = Builder->CreateInsertValue(view_val, callret("malloc", {ArgsV[0]}), {0});
    view_val = Builder->CreateInsertValue(view_val, ArgsV[0], {1});
    return view_val;
}

Value *alloc(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    
    Value *type_id;
    std::string type;
    if(auto *str_expr = dynamic_cast<StringExprAST*>(Args[1].get())) {
        type = str_expr->Val;
        type_id = const_uint16(data_name_to_type()[type]);
    }
    else {
        LogErrorS(parser_struct->line, "alloc requires const string");
        std::exit(0);
    }

    Value *ret = callret("allocate_pool", {scope_struct, ArgsV[0], type_id});
    if (type=="str") {
        Value *view_val = UndefValue::get(struct_types["DT_str"]);
        view_val = Builder->CreateInsertValue(view_val, ret, {0});
        view_val = Builder->CreateInsertValue(view_val, ArgsV[0], {1});
        ret = view_val;
    }
    return ret;
}

Value *str_size(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    return Builder->CreateExtractValue(ArgsV[0], {1});
}

Value *c_strlen(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *str = Builder->CreateExtractValue(ArgsV[0], {0});
    return callret("strlen", {str});
}

Value *c_memcpy(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *str = Builder->CreateExtractValue(ArgsV[0], {0});
    Value *src = ArgsV[1];
    if (args_type[1].Type=="str")
        src = Builder->CreateExtractValue(src, {0});
    
    Builder->CreateMemCpy(str, Align(1), src, Align(1), ArgsV[2]);
    return const_int(0);
}

Value *str_set(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *last_c_gep = Builder->CreateInBoundsGEP(int8Ty, ArgsV[0], ArgsV[1]);
    Builder->CreateStore(Builder->CreateTrunc(ArgsV[2], int8Ty), last_c_gep);
    return const_int(0);
}

Value *str_offset(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *ptr_val = ArgsV[0];
    Value *offset = Builder->CreateExtractValue(ptr_val, {0});
    return Builder->CreateGEP(int8Ty, offset, ArgsV[1]);
}





Value *c_memchr(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *buf = ArgsV[0];
    buf = Builder->CreateExtractValue(buf, {0});

    Value* newlineVal = Builder->CreateIntCast(ArgsV[1], intTy, /*isSigned=*/true);
    Value *len = ArgsV[2];

    Value* pos = callret("memchr", { buf, newlineVal, len });
    Value* isNotNull = Builder->CreateICmpNE(
        pos,
        ConstantPointerNull::get(cast<PointerType>(int8PtrTy))
    );

    Value* posInt = Builder->CreatePtrToInt(pos, int64Ty);
    Value* bufInt = Builder->CreatePtrToInt(buf, int64Ty);

    Value* diff = Builder->CreateSub(posInt, bufInt);

    Value* ret = Builder->CreateSelect(
        isNotNull,
        diff,
        len
    );
    return ret;
}

Value *shfl_sync(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {

    Value *mask = ConstantInt::get(intTy, 0xFFFFFFFF);
    Value *width  = ConstantInt::get(intTy, 31);

    Value *delta = Builder->CreateIntCast(ArgsV[1], intTy, false);
    Value *pred  = ConstantInt::get(Type::getInt1Ty(*TheContext), 1);

    Function *shfl =
        Intrinsic::getDeclaration(
            PtxModule.get(),
            Intrinsic::nvvm_shfl_sync_bfly_f32);
            // Intrinsic::nvvm_shfl_sync_down_f32);

    // shfl->print(llvm::errs());

    Value *res = Builder->CreateCall(shfl, {mask, ArgsV[0], delta, width});
    return res;
}
Value *cp_async16(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {


    Value *smem_ptr = ArgsV[0];
    Value *gmem_ptr = ArgsV[1];

    Function *cp =
        Intrinsic::getDeclaration(
            PtxModule.get(),
            Intrinsic::nvvm_cp_async_cg_shared_global_16);


    // smem_ptr = Builder->CreateAddrSpaceCast(
    //     smem_ptr,
    //     PointerType::get(*TheContext, 3));

    gmem_ptr = Builder->CreateAddrSpaceCast(
        gmem_ptr,
        PointerType::get(*TheContext, 1)); // enforce global space

    return Builder->CreateCall(cp, {smem_ptr, gmem_ptr});
}

Value *cp_commit_group(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Function *commit =
        Intrinsic::getDeclaration(
            PtxModule.get(),
            Intrinsic::nvvm_cp_async_commit_group);

    Builder->CreateCall(commit);
    return const_int(0);
}

Value *cp_wait_group(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Function *wait =
        Intrinsic::getDeclaration(
            PtxModule.get(),
            Intrinsic::nvvm_cp_async_wait_group);

    Builder->CreateCall(wait, {ArgsV[0]});
    return const_int(0);
}

Value *cp_wait_all(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Function *wait_all =
    Intrinsic::getDeclaration(
        PtxModule.get(),
        Intrinsic::nvvm_cp_async_wait_all);

    Builder->CreateCall(wait_all);
    return const_int(0);
}

Value *ldmatrix_x4(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Function *ldmatrix =
        Intrinsic::getDeclaration(
            PtxModule.get(),
            Intrinsic::nvvm_ldmatrix_sync_aligned_m8n8_x4_b16, 
            {PointerType::get(*TheContext, 3)}
            );

    Value *ret = Builder->CreateCall(ldmatrix, {ArgsV[1]});

    Value *gep_0 = Builder->CreateGEP(intTy, ArgsV[0], const_int(0));
    Value *gep_1 = Builder->CreateGEP(intTy, ArgsV[0], const_int(1));
    Value *gep_2 = Builder->CreateGEP(intTy, ArgsV[0], const_int(2));
    Value *gep_3 = Builder->CreateGEP(intTy, ArgsV[0], const_int(3));
    Builder->CreateStore(Builder->CreateExtractValue(ret, {0}), gep_0);
    Builder->CreateStore(Builder->CreateExtractValue(ret, {1}), gep_1);
    Builder->CreateStore(Builder->CreateExtractValue(ret, {2}), gep_2);
    Builder->CreateStore(Builder->CreateExtractValue(ret, {3}), gep_3);
    return const_int(0);
}


Value *printff(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *str = Builder->CreateExtractValue(ArgsV[0], {0});
    return callret("printf", {str, ArgsV[1]});
}


Value *ldmatrix_x2(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Function *ldmatrix =
        Intrinsic::getDeclaration(
            PtxModule.get(),
            Intrinsic::nvvm_ldmatrix_sync_aligned_m8n8_x2_b16, 
            {PointerType::get(*TheContext, 3)}
            );

    Value *ret = Builder->CreateCall(ldmatrix, {ArgsV[1]});

    Value *gep_0 = Builder->CreateGEP(intTy, ArgsV[0], const_int(0));
    Value *gep_1 = Builder->CreateGEP(intTy, ArgsV[0], const_int(1));
    Builder->CreateStore(Builder->CreateExtractValue(ret, {0}), gep_0);
    Builder->CreateStore(Builder->CreateExtractValue(ret, {1}), gep_1);
    return const_int(0);
}
Value *ldmatrix_x2T(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Function *ldmatrix =
        Intrinsic::getDeclaration(
            PtxModule.get(),
            Intrinsic::nvvm_ldmatrix_sync_aligned_m8n8_x2_trans_b16, 
            {PointerType::get(*TheContext, 3)}
            );

    Value *ret = Builder->CreateCall(ldmatrix, {ArgsV[1]});

    Value *gep_0 = Builder->CreateGEP(intTy, ArgsV[0], const_int(0));
    Value *gep_1 = Builder->CreateGEP(intTy, ArgsV[0], const_int(1));
    Builder->CreateStore(Builder->CreateExtractValue(ret, {0}), gep_0);
    Builder->CreateStore(Builder->CreateExtractValue(ret, {1}), gep_1);
    return const_int(0);
}

Value *mma_16x8x16(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Function *mma =
        Intrinsic::getDeclaration(
            PtxModule.get(),
            Intrinsic::nvvm_mma_m16n8k16_row_col_bf16);



    Value *gepA_0 = Builder->CreateGEP(intTy, ArgsV[1], const_int(0));
    Value *gepA_1 = Builder->CreateGEP(intTy, ArgsV[1], const_int(1));
    Value *gepA_2 = Builder->CreateGEP(intTy, ArgsV[1], const_int(2));
    Value *gepA_3 = Builder->CreateGEP(intTy, ArgsV[1], const_int(3));
    Value *A0 = Builder->CreateLoad(intTy, gepA_0);
    Value *A1 = Builder->CreateLoad(intTy, gepA_1);
    Value *A2 = Builder->CreateLoad(intTy, gepA_2);
    Value *A3 = Builder->CreateLoad(intTy, gepA_3);

    Value *gepB_0 = Builder->CreateGEP(intTy, ArgsV[2], const_int(0));
    Value *gepB_1 = Builder->CreateGEP(intTy, ArgsV[2], const_int(1));
    Value *B0 = Builder->CreateLoad(intTy, gepB_0);
    Value *B1 = Builder->CreateLoad(intTy, gepB_1);

    Value *gepC_0 = Builder->CreateGEP(floatTy, ArgsV[0], const_int(0));
    Value *gepC_1 = Builder->CreateGEP(floatTy, ArgsV[0], const_int(1));
    Value *gepC_2 = Builder->CreateGEP(floatTy, ArgsV[0], const_int(2));
    Value *gepC_3 = Builder->CreateGEP(floatTy, ArgsV[0], const_int(3));
    Value *C0 = Builder->CreateLoad(floatTy, gepC_0);
    Value *C1 = Builder->CreateLoad(floatTy, gepC_1);
    Value *C2 = Builder->CreateLoad(floatTy, gepC_2);
    Value *C3 = Builder->CreateLoad(floatTy, gepC_3);

    Value *ret = Builder->CreateCall(mma, {
        A0, A1, A2, A3,
        B0, B1,
        C0, C1, C2, C3
    });

    Builder->CreateStore(Builder->CreateExtractValue(ret, {0}), gepC_0);
    Builder->CreateStore(Builder->CreateExtractValue(ret, {1}), gepC_1);
    Builder->CreateStore(Builder->CreateExtractValue(ret, {2}), gepC_2);
    Builder->CreateStore(Builder->CreateExtractValue(ret, {3}), gepC_3);

    return const_int(0);
}


Value *syncthreads(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Function *Barrier =
        Intrinsic::getOrInsertDeclaration(
            PtxModule.get(),
            Intrinsic::nvvm_barrier_cta_sync_all);

    Builder->CreateCall(Barrier, {
        const_int(0)
    });
    return const_int(0);
}


Value *min(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {

    std::string type = args_type[0].Type;
    Value *x=ArgsV[0], *y=ArgsV[1];
    if (type=="float")
        return Builder->CreateMinNum(x, y);
    if (in_vec(type,int_types)) {
        Value* cond = Builder->CreateICmpSLT(x, y); // signed min
        return Builder->CreateSelect(cond, x, y, "int_min");
    }
    
    std::string fn = "min_"+type;
    if (auto *F = TheModule->getFunction(fn))
        return callret(fn, {scope_struct, x, y});

    LogErrorC(parser_struct->line, "Could not handle min op for: " + type);

    return const_int(0);
}

Value *max(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    std::string type = args_type[0].Type;
    Value *x=ArgsV[0], *y=ArgsV[1];
    if (type=="float")
        return Builder->CreateMaxNum(x, y);
    if (in_vec(type,int_types)) {
        Value* cond = Builder->CreateICmpSGT(x, y); // signed max
        return Builder->CreateSelect(cond, x, y, "int_max");
    }
    
    std::string fn = "max_"+type;
    if (auto *F = TheModule->getFunction(fn))
        return callret(fn, {scope_struct, x, y});

    LogErrorC(parser_struct->line, "Could not handle max op for: " + type);

    return const_int(0);
}



Value *err(Parser_Struct * parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {

    call("LogErrorCall", {const_int(parser_struct->line), ArgsV[0]});
    call("_quit_", {});
    return const_int(0);
}





Value *printl(Parser_Struct * parser_struct, Function *TheFunction,
        std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
        Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {

    Value *offset = const_int(0);

    Value *print_buffer = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 6);

    llvm::Type *bufferTy = ArrayType::get(int8Ty, PrintBufferSize);

    for (int i=0; i<ArgsV.size(); ++i) {
        Value *print_val = ArgsV[i];
        Value *size;
        std::string arg_type = args_type[i].Type;

        Value *print_gep = Builder->CreateInBoundsGEP(bufferTy,
                print_buffer, {const_int(0), offset});
        
        if(arg_type=="str") {
            StructType *st = struct_types["DT_str"];
            Value *str_v = Builder->CreateExtractValue(print_val, {0});
            size = Builder->CreateExtractValue(print_val, {1});
            call("memcpy", {print_gep, str_v, Builder->CreateIntCast(size,int64Ty,true)});
        } else if (arg_type=="char") {
            size = const_int(1);
            Builder->CreateStore(print_val, print_gep);
        } else {
            std::string callee = arg_type + "_to_str_buffer";
            // if (arg_type=="bool") {
            //     printTy(print_val);
            //     call("print_bool", {print_val});
            // }
            size = callret(callee, {scope_struct, print_val, print_gep});
        }

        offset = Builder->CreateAdd(offset, size);
    }

    Value *buf_ptr = Builder->CreateInBoundsGEP( // &print_buffer[0]
            int8Ty,
            print_buffer,
            { const_int(0) }
            );
    call("write", {const_int(1), buf_ptr, Builder->CreateIntCast(offset,int64Ty,true)});

    return const_float(0);
}



Value *makePrintfBuffer(Value *arg, Type *Ty, std::string type) {
    IRBuilder<> &B = *Builder;

    AllocaInst *buf = B.CreateAlloca(B.getInt64Ty());

    if (in_vec(type, int_types)) {
        if (Ty->getIntegerBitWidth() < 64)
            arg = B.CreateSExtOrTrunc(arg, B.getInt64Ty());

        B.CreateStore(arg, buf);
        return B.CreateBitCast(buf, int8PtrTy);
    }

    // float -> double
    if (type=="float") {
        Value *d = B.CreateFPExt(arg, B.getDoubleTy());
        B.CreateStore(d, buf);
        return B.CreateBitCast(buf, int8PtrTy);
    }

    if (type=="bf16") {
        Value *u16 = arg;
        Value *u32 = B.CreateZExt(u16, B.getInt32Ty());
        Value *shifted = B.CreateShl(u32, B.getInt32(16));
        Value *f32 = B.CreateBitCast(shifted, B.getFloatTy());
        Value *f64 = B.CreateFPExt(f32, B.getDoubleTy());

        B.CreateStore(f64, buf);
        return B.CreateBitCast(buf, int8PtrTy);
    }

    return nullptr;
}

void print_gpu(Parser_Struct * parser_struct, Function *TheFunction,
         std::vector<Data_Tree> &args_type,
         std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {

    for (int i = 0; i < ArgsV.size(); i++) {
        Value *fmt = nullptr;
        Value *bufPtr = nullptr;
        std::string type = args_type[i].Type;
        if (type=="layout"||args_type[i].is_buffer||args_type[i].is_array)
            type = args_type[i].Nested_Data[0].Type;

        Type *Ty = ArgsV[i]->getType();


        if (in_vec(type, int_types))
            fmt = global_str("%d\n");
        else if (type == "float")
            fmt = global_str("%f\n");
        else if (type == "bf16")
            fmt = global_str("%f\n");   // bf16 prints as float
        else {
            std::cout << "CUDA PRINT FOR " << type << " NOT YET IMPLEMENTED\n";
            std::exit(0);
        }
        bufPtr = makePrintfBuffer(ArgsV[i], Ty, type);

        call("vprintf", {fmt, bufPtr});
    }

}



Value *print(Parser_Struct * parser_struct, Function *TheFunction,
        std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
        Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    if (parser_struct->gpu>0) {
        print_gpu(parser_struct, TheFunction, args_type, Args, ArgsV);
        return const_int(0);
    }
    
    Value *offset = const_int(0);

    Value *print_buffer = Builder->CreateStructGEP(struct_types["scope_struct"], scope_struct, 6);

    llvm::Type *bufferTy = ArrayType::get(int8Ty, PrintBufferSize);

    for (int i=0; i<ArgsV.size(); ++i) {
        Value *print_val = ArgsV[i];
        Value *size;
        std::string arg_type = args_type[i].Type;

        Value *print_gep = Builder->CreateInBoundsGEP(bufferTy,
                print_buffer, {const_int(0), offset});
        
        if(arg_type=="str") {
            StructType *st = struct_types["DT_str"];
            Value *str_v = Builder->CreateExtractValue(print_val, {0});
            size = Builder->CreateExtractValue(print_val, {1});
            call("memcpy", {print_gep, str_v, Builder->CreateIntCast(size,int64Ty,true)});
        } else if (arg_type=="char") {
            size = const_int(1);
            Builder->CreateStore(print_val, print_gep);
        } else {
            std::string callee = arg_type + "_to_str_buffer";
            // if (arg_type=="bool") {
            //     printTy(print_val);
            //     call("print_bool", {print_val});
            // }
            size = callret(callee, {scope_struct, print_val, print_gep});
        }

        offset = Builder->CreateAdd(offset, size);
    }
    Value *print_gep = Builder->CreateInBoundsGEP(bufferTy,
            print_buffer, {const_int(0), offset});
    call("memcpy", {print_gep, global_str("\n"), const_int64(1)});
    offset = Builder->CreateAdd(offset, const_int(1));

    Value *buf_ptr = Builder->CreateInBoundsGEP( // &print_buffer[0]
            int8Ty,
            print_buffer,
            { const_int(0) }
            );
    call("write", {const_int(1), buf_ptr, Builder->CreateIntCast(offset,int64Ty,true)});

    return const_float(0);
}


