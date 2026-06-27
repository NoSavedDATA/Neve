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

Data_Tree min_ret_dt(Parser_Struct parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args) {
  return Args[0]->GetDataTree();
}
Data_Tree max_ret_dt(Parser_Struct parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args) {
  return Args[0]->GetDataTree();
}
Data_Tree array_clone_dt(Parser_Struct parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  Data_Tree dt = Data_Tree("array");
  dt.Nested_Data.push_back(inner->GetDataTree().Nested_Data[0]);
  return dt;
}
Data_Tree array_pop_dt(Parser_Struct parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  return inner->GetDataTree().Nested_Data[0];
}
Data_Tree map_keys_dt(Parser_Struct parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  Data_Tree dt = Data_Tree("array");
  dt.Nested_Data.push_back(inner->GetDataTree().Nested_Data[0]);
  return dt;
}
Data_Tree map_values_dt(Parser_Struct parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  Data_Tree dt = Data_Tree("array");
  dt.Nested_Data.push_back(inner->GetDataTree().Nested_Data[1]);
  return dt;
}
Data_Tree map_get_dt(Parser_Struct parser_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::unique_ptr<Nameable> &inner) {
  Data_Tree dt = Data_Tree("tuple");
  dt.Nested_Data.push_back(inner->GetDataTree().Nested_Data[1]);
  dt.Nested_Data.push_back(Data_Tree("bool"));
  return dt;
}

Value *DT_charv_Create(Parser_Struct parser_struct, Function *TheFunction,
                      std::string name, std::string type, Data_Tree data_type,
                      Value *scope_struct, Value *initial_value,
                      std::vector<std::unique_ptr<ExprAST>> &Args,
                      std::vector<Value*> &ArgsV) { 
    int size;
    if (auto num_expr = dynamic_cast<IntExprAST*>(Args[0].get()))
        size = num_expr->Val;

    llvm::Type *charTy = ArrayType::get(int8Ty, size);
    AllocaInst *charv = CreateEntryBlockAlloca(TheFunction, "charv", charTy);

    function_allocas[parser_struct.function_name][name] = charv;
    
    return Builder->CreateInBoundsGEP(
        charTy,
        charv,
        {const_int(0), const_int(0)}
    );
}


Value *DT_vec_Create(Parser_Struct parser_struct, Function *TheFunction,
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
        LogError(parser_struct.line, "Vec expected type");
    if (auto num_expr = dynamic_cast<IntExprAST*>(Args[1].get()))
            size = num_expr->Val;
    else
        LogError(parser_struct.line, "Vec expected size");

    llvm::Type *vecTy = VectorType::get(ty, size, false);
    Value *vec = llvm::UndefValue::get(vecTy);
    return vec;
}


Value *print_bb(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    bb_name(Builder->GetInsertBlock());
    return const_int(0);
}Value *print_Value(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    v_ir(ArgsV[0]);
    return const_int(0);
}

Value *to_char(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, int_types))
        LogError(parser_struct.line, "Cannot cast " + type + " to i8.");
    return Builder->CreateIntCast(ArgsV[0], int8Ty, true); // true for signed extend
}
Value *i8(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, {"int", "i16", "i64"}))
        LogError(parser_struct.line, "Cannot cast " + type + " to i8.");
    bool is_signed = type!="char";
    return Builder->CreateIntCast(ArgsV[0], int8Ty, is_signed); // true for signed extend
}
Value *i16(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, {"int", "i8", "i64", "char"}))
        LogError(parser_struct.line, "Cannot cast " + type + " to i16.");
    bool is_signed = type!="char";
    return Builder->CreateIntCast(ArgsV[0], int16Ty, is_signed); // true for signed extend
}
Value *to_int(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, {"i8", "i16", "i64", "char"}))
        LogError(parser_struct.line, "Cannot cast " + type + " to int.");
    bool is_signed = type!="char";
    return Builder->CreateIntCast(ArgsV[0], intTy, is_signed);
}
Value *i64(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    const std::string &type = Args[0]->GetDataTree().Type;
    if(!in_vec(type, {"int", "i16", "i8"}))
        LogError(parser_struct.line, "Cannot cast " + type + " to i64.");
    return Builder->CreateIntCast(ArgsV[0], int64Ty, true); // true for signed extend
}


Value *dsize(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    return callret("get_size", {scope_struct, ArgsV[0]});
}
Value *fexists(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>> &Args, std::vector<Value*> &ArgsV) {
    Value *str = Builder->CreateExtractValue(ArgsV[0], {0});
    return callret("fexists_C", {scope_struct, str});
}
Value *c_open(Parser_Struct parser_struct, Function *TheFunction,
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
    call("LogErrorCall", {const_int(parser_struct.line), err_msg});
    Builder->CreateUnreachable();

    Builder->SetInsertPoint(GoodBB);
    
    return callret("open", {str, const_int(0)});
}

Value *c_read(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *src = ArgsV[1];
    if (args_type[1].Type=="str")
        src = Builder->CreateExtractValue(src, {0});
    
    return callret("read", {ArgsV[0], src, ArgsV[2]});
}

Value *c_malloc(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    return callret("malloc", {ArgsV[0]});
}
Value *c_malloc32(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *size = ArgsV[0];
    return callret("aligned_alloc", {const_int64(32), size});
}
Value *c_malloc64(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *size = ArgsV[0];
    return callret("aligned_alloc", {const_int64(64), size});
}


Value *c_malloc_str(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *view_val = UndefValue::get(struct_types["DT_str"]);
    view_val = Builder->CreateInsertValue(view_val, callret("malloc", {ArgsV[0]}), {0});
    view_val = Builder->CreateInsertValue(view_val, ArgsV[0], {1});
    return view_val;
}

Value *alloc(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    
    Value *type_id;
    std::string type;
    if(auto *str_expr = dynamic_cast<StringExprAST*>(Args[1].get())) {
        type = str_expr->Val;
        type_id = const_uint16(data_name_to_type()[type]);
    }
    else {
        LogErrorS(parser_struct.line, "alloc requires const string");
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

Value *str_size(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    return Builder->CreateExtractValue(ArgsV[0], {1});
}

Value *c_strlen(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *str = Builder->CreateExtractValue(ArgsV[0], {0});
    return callret("strlen", {str});
}

Value *c_memcpy(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *str = Builder->CreateExtractValue(ArgsV[0], {0});
    Value *src = ArgsV[1];
    if (args_type[1].Type=="str")
        src = Builder->CreateExtractValue(src, {0});
    
    Builder->CreateMemCpy(str, Align(1), src, Align(1), ArgsV[2]);
    return const_int(0);
}

Value *str_set(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *last_c_gep = Builder->CreateInBoundsGEP(int8Ty, ArgsV[0], ArgsV[1]);
    Builder->CreateStore(Builder->CreateTrunc(ArgsV[2], int8Ty), last_c_gep);
    return const_int(0);
}

Value *str_offset(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    Value *ptr_val = ArgsV[0];
    Value *offset = Builder->CreateExtractValue(ptr_val, {0});
    return Builder->CreateGEP(int8Ty, offset, ArgsV[1]);
}





Value *c_memchr(Parser_Struct parser_struct, Function *TheFunction,
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

Value *shfl_sync(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {

    // Value *mask = ConstantInt::get(intTy, 0xFFFFFFFF);
    // Value *width  = ConstantInt::get(intTy, 31);

    // Value *delta = Builder->CreateIntCast(ArgsV[1], intTy, false);
    // Value *pred  = ConstantInt::get(Type::getInt1Ty(*TheContext), 1);

    // Function *shfl =
    //     Intrinsic::getDeclaration(
    //         PtxModule.get(),
    //         Intrinsic::nvvm_shfl_sync_bfly_f32);
    //         // Intrinsic::nvvm_shfl_sync_down_f32);

    // // shfl->print(llvm::errs());

    // Value *res = Builder->CreateCall(shfl, {mask, ArgsV[0], delta, width});
    // return res;
    return const_int(0);
}
Value *cp_async16(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {


//     Value *smem_ptr = ArgsV[0];
//     Value *gmem_ptr = ArgsV[1];

//     Function *cp =
//         Intrinsic::getDeclaration(
//             PtxModule.get(),
//             Intrinsic::nvvm_cp_async_cg_shared_global_16);


//     // smem_ptr = Builder->CreateAddrSpaceCast(
//     //     smem_ptr,
//     //     PointerType::get(*TheContext, 3));

//     gmem_ptr = Builder->CreateAddrSpaceCast(
//         gmem_ptr,
//         PointerType::get(*TheContext, 1)); // enforce global space

//     return Builder->CreateCall(cp, {smem_ptr, gmem_ptr});
    return const_int(0);
}

Value *cp_commit_group(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    // Function *commit =
    //     Intrinsic::getDeclaration(
    //         PtxModule.get(),
    //         Intrinsic::nvvm_cp_async_commit_group);

    // Builder->CreateCall(commit);
    return const_int(0);
}

Value *cp_wait_group(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    // Function *wait =
    //     Intrinsic::getDeclaration(
    //         PtxModule.get(),
    //         Intrinsic::nvvm_cp_async_wait_group);

    // Builder->CreateCall(wait, {ArgsV[0]});

    return const_int(0);
}

Value *cp_wait_all(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    // Function *wait_all =
    // Intrinsic::getDeclaration(
    //     PtxModule.get(),
    //     Intrinsic::nvvm_cp_async_wait_all);

    // Builder->CreateCall(wait_all);
    return const_int(0);
}

Value *ldmatrix_x4(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {

    // Value *addr = ArgsV[0];
    // Value *addr = Builder->CreatePtrToInt(ArgsV[0], intTy);
    // addr = Builder->CreateTrunc(addr, intTy);

    // std::string asmStr =
    //     "{ldmatrix.sync.aligned.m8n8.x4.b16 "
    //         "%0, %1, %2, %3, [%4];}";

    // std::vector<Type*> outTypes = {
    //     intTy, intTy, intTy, intTy
    // };

    // std::vector<Type*> inTypes = {
    //     intTy->getPointerTo(3)   // shared memory
    // };

    // FunctionType *FTy =
    //     FunctionType::get(
    //         StructType::get(intTy, intTy, intTy, intTy),
    //         { intTy->getPointerTo(3) },
    //         false);
    // InlineAsm *IA =
    //     InlineAsm::get(
    //         FTy,
    //         asmStr,
    //         "=r,=r,=r,=r,r",
    //         true,   // has side effects
    //         false);
    // Value *res =
    //     Builder->CreateCall(IA, {ArgsV[0]});


    // Function *ldmatrix =
    //     Intrinsic::getDeclaration(
    //         PtxModule.get(),
    //         Intrinsic::nvvm_ldmatrix_sync_aligned_m8n8_x4_trans_b16);
    // ldmatrix->dump();

    // Value *addr = ArgsV[0];
    // Value *v0 = Builder->CreateGEP(int16Ty, addr, const_int(0));
    // Value *v1 = Builder->CreateGEP(int16Ty, addr, const_int(2));
    // Value *v2 = Builder->CreateGEP(int16Ty, addr, const_int(4));
    // Value *v3 = Builder->CreateGEP(int16Ty, addr, const_int(8));
    // v0 = Builder->CreateLoad(intTy, v0);
    // v1 = Builder->CreateLoad(intTy, v1);
    // v2 = Builder->CreateLoad(intTy, v2);
    // v3 = Builder->CreateLoad(intTy, v3);

    // llvm::Type *structTy = StructType::get(intTy, intTy, intTy, intTy);
    // Value *view_val = UndefValue::get(structTy);
    // view_val = Builder->CreateInsertValue(view_val, v0, {0});
    // view_val = Builder->CreateInsertValue(view_val, v1, {1});
    // view_val = Builder->CreateInsertValue(view_val, v2, {2});
    // view_val = Builder->CreateInsertValue(view_val, v3, {3});

    // return view_val;
    return const_int(0);
}

Value *mma_16x8x16(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    // Function *mma =
    //     Intrinsic::getDeclaration(
    //         PtxModule.get(),
    //         Intrinsic::nvvm_mma_m16n8k16_row_col_f16_f32);
    // mma->dump();
    // mma->getFunctionType()->dump();

    // Value *fragA = ArgsV[0];
    // Value *fragB = ArgsV[1];
    // Type *half2Ty = FixedVectorType::get(halfTy, 2);

    // Value *A0 = Builder->CreateExtractValue(fragA, {0});
    // Value *A1 = Builder->CreateExtractValue(fragA, {1});
    // Value *A2 = Builder->CreateExtractValue(fragA, {2});
    // Value *A3 = Builder->CreateExtractValue(fragA, {3});

    // Value *B0 = Builder->CreateExtractValue(fragB, {0});
    // Value *B1 = Builder->CreateExtractValue(fragB, {1});

    // A0 = Builder->CreateBitCast(A0, half2Ty);
    // A1 = Builder->CreateBitCast(A1, half2Ty);
    // A2 = Builder->CreateBitCast(A2, half2Ty);
    // A3 = Builder->CreateBitCast(A3, half2Ty);

    // B0 = Builder->CreateBitCast(B0, half2Ty);
    // B1 = Builder->CreateBitCast(B1, half2Ty);

    // Value *C0 = const_float(0);
    // Value *C1 = const_float(0);
    // Value *C2 = const_float(0);
    // Value *C3 = const_float(0);

    // Value *res = Builder->CreateCall(mma, {
    //     A0, A1, A2, A3,
    //     B0, B1,
    //     C0, C1, C2, C3
    // });

    // return res;
    return const_int(0);
}

Value *ldmatrix_x2(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {

    // Value *addr = ArgsV[0];
    // Value *v0 = Builder->CreateGEP(int16Ty, addr, const_int(0));
    // Value *v1 = Builder->CreateGEP(int16Ty, addr, const_int(2));
    // v0 = Builder->CreateLoad(intTy, v0);
    // v1 = Builder->CreateLoad(intTy, v1);

    // llvm::Type *structTy = StructType::get(intTy, intTy);
    // Value *view_val = UndefValue::get(structTy);
    // view_val = Builder->CreateInsertValue(view_val, v0, {0});
    // view_val = Builder->CreateInsertValue(view_val, v1, {1});

    // return view_val;
    return const_int(0);
}


Value *syncthreads(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {
    // Function *barrier =
    //     Intrinsic::getDeclaration(
    //         PtxModule.get(),
    //         Intrinsic::nvvm_barrier0   // <-- __syncthreads
    //     );

    // Builder->CreateCall(barrier);
    return const_int(0);
}


Value *min(Parser_Struct parser_struct, Function *TheFunction,
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

    LogErrorC(parser_struct.line, "Could not handle min op for: " + type);

    return const_int(0);
}

Value *max(Parser_Struct parser_struct, Function *TheFunction,
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

    LogErrorC(parser_struct.line, "Could not handle max op for: " + type);

    return const_int(0);
}



Value *err(Parser_Struct parser_struct, Function *TheFunction,
                 std::string Callee, Data_Tree data_type, std::vector<Data_Tree> &args_type,
                 Value *scope_struct, std::vector<std::unique_ptr<ExprAST>>& Args, std::vector<Value*> &ArgsV) {

    call("LogErrorCall", {const_int(parser_struct.line), ArgsV[0]});
    call("_quit_", {});
    return const_int(0);
}





Value *printl(Parser_Struct parser_struct, Function *TheFunction,
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





Value *print(Parser_Struct parser_struct, Function *TheFunction,
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


