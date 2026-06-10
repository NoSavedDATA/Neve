#include "../nsk_cpp_llvm.h"




Value *simd_add(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    Data_Tree dt = LHS->GetDataTree();
    if (dt.Nested_Data[0].Type=="float")
        return Builder->CreateFAdd(L, R);
    return Builder->CreateAdd(L, R);
}
Value *simd_mult(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    Data_Tree dt = LHS->GetDataTree();
    if (dt.Nested_Data[0].Type=="float")
        return Builder->CreateFMul(L, R);
    
    return Builder->CreateMul(L, R);
}

Value *simd_equal(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    auto *cmp = Builder->CreateICmpEQ(L, R);
    auto *vecTy = get_type_from_data(LHS->GetDataTree());
    return Builder->CreateSExt(cmp, vecTy);
}
Value *simd_dif(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    auto *cmp = Builder->CreateICmpNE(L, R);
    auto *vecTy = get_type_from_data(LHS->GetDataTree());
    return Builder->CreateSExt(cmp, vecTy);
}
Value *simd_and(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    auto *cmp = Builder->CreateAnd(L, R);
    auto *vecTy = get_type_from_data(LHS->GetDataTree());
    return Builder->CreateSExt(cmp, vecTy);
}
Value *simd_or(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    auto *cmp = Builder->CreateOr(L, R);
    auto *vecTy = get_type_from_data(LHS->GetDataTree());
    return Builder->CreateSExt(cmp, vecTy);
}
Value *simd_higher(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    auto *cmp = Builder->CreateICmpSGT(L, R);
    auto *vecTy = get_type_from_data(LHS->GetDataTree());
    return Builder->CreateSExt(cmp, vecTy);
}
Value *simd_minor(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    auto *cmp = Builder->CreateICmpSLT(L, R);
    auto *vecTy = get_type_from_data(LHS->GetDataTree());
    return Builder->CreateSExt(cmp, vecTy);
}
Value *simd_highereq(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    auto *cmp = Builder->CreateICmpSGE(L, R);
    auto *vecTy = get_type_from_data(LHS->GetDataTree());
    return Builder->CreateSExt(cmp, vecTy);
}
Value *simd_minoreq(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
    auto *cmp = Builder->CreateICmpSLE(L, R);
    auto *vecTy = get_type_from_data(LHS->GetDataTree());
    return Builder->CreateSExt(cmp, vecTy);
}

// Value *simd_lshift(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
//     return Builder->CreateShl(L, R);
// }
// Value *simd_rshift(std::unique_ptr<ExprAST> &LHS, std::unique_ptr<ExprAST> &RHS, Value *L, Value *R) {
//     return Builder->CreateLShr(L, R);
// }
static bool is_i8_vector(Value *V) {
    auto *VT = dyn_cast<VectorType>(V->getType());
    if (!VT)
        return false;

    auto *ElemTy = VT->getElementType();
    return ElemTy->isIntegerTy(8);
}

static int get_vector_elements(Value *V) {
    auto *VT = cast<VectorType>(V->getType());
    return VT->getElementCount().getKnownMinValue();
}
Value *simd_rshift(std::unique_ptr<ExprAST> &LHS,
                   std::unique_ptr<ExprAST> &RHS,
                   Value *L,
                   Value *R) {

    if (is_i8_vector(L)) {

        auto *VecTy8 = cast<VectorType>(L->getType());

        int NumElems =
            VecTy8->getElementCount().getKnownMinValue();

        LLVMContext &Ctx = *TheContext;

        Constant *MaskF0 =
            ConstantVector::getSplat(
                VecTy8->getElementCount(),
                ConstantInt::get(
                    Type::getInt8Ty(Ctx),
                    0xF0
                )
            );

        Value *Masked =
            Builder->CreateAnd(L, MaskF0);

        auto *VecTy16 =
            VectorType::get(
                Type::getInt16Ty(Ctx),
                NumElems / 2,
                false
            );

        Value *As16 =
            Builder->CreateBitCast(Masked, VecTy16);

        Value *Shifted16 =
            Builder->CreateLShr(
                As16,
                ConstantInt::get(
                    Type::getInt16Ty(Ctx),
                    4
                )
            );

        Constant *Mask0F =
            ConstantVector::getSplat(
                VecTy16->getElementCount(),
                ConstantInt::get(
                    Type::getInt16Ty(Ctx),
                    0x000F
                )
            );

        Value *Nibble16 =
            Builder->CreateAnd(Shifted16, Mask0F);

        return Builder->CreateBitCast(
            Nibble16,
            VecTy8
        );
    }

    return Builder->CreateLShr(L, R);
}
Value *simd_lshift(std::unique_ptr<ExprAST> &LHS,
                   std::unique_ptr<ExprAST> &RHS,
                   Value *L,
                   Value *R) {

    if (is_i8_vector(L)) {

        auto *VecTy8 = cast<VectorType>(L->getType());
        int NumElems = get_vector_elements(L);

        LLVMContext &Ctx = *TheContext;

        auto *VecTy16 =
            VectorType::get(Type::getInt16Ty(Ctx),
                            NumElems / 2,
                            false);

        Value *As16 =
            Builder->CreateBitCast(L, VecTy16);

        Constant *Shift16 = ConstantVector::getSplat(
            ElementCount::getFixed(NumElems / 2),
            ConstantInt::get(Type::getInt16Ty(Ctx), 4)
        );

        Value *Shifted16 =
            Builder->CreateLShr(
                As16,
                ConstantInt::get(Type::getInt16Ty(Ctx), 4)
            );

        return Builder->CreateBitCast(Shifted16, VecTy8);
    }

    return Builder->CreateShl(L, R);
}
