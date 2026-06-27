//===- KaleidoscopeJIT.h - A simple JIT for Kaleidoscope --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Contains a simple JIT definition for use in the kaleidoscope tutorials.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/EPCIndirectionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <memory>
#include "KaleidoscopeJIT.h"
#include "compiler_frontend/expressions.h"
#include "compiler_frontend/function_ast.h"
#include "compiler_frontend/modules.h"
#include "llvm/Support/Error.h"

static llvm::ExitOnError ExitOnErr;

class PrototypeAST;
class ExprAST;

/// FunctionAST - This class represents a function definition itself.
FunctionAST::FunctionAST(Parser_Struct parser_struct, std::unique_ptr<PrototypeAST> Proto,
                std::vector<std::unique_ptr<ExprAST>> Body)
        : parser_struct(parser_struct), Proto(std::move(Proto)), Body(std::move(Body)) {}
  



/// This will compile FnAST to IR, rename the function to add the given
/// suffix (needed to prevent a name-clash with the function's stub),
/// and then take ownership of the module that the function was compiled
/// into.

namespace llvm {
namespace orc {

class KaleidoscopeJIT;


std::string KaleidoscopeJIT::MangleName(const std::string &Name) {
    return std::string(*Mangle(Name));
}

llvm::Error KaleidoscopeJIT::addAST(std::unique_ptr<FunctionAST> F) {
    fn_vec.push_back(std::move(F));
    return llvm::Error::success(); 
}

llvm::Error KaleidoscopeJIT::genAST() {
    for (auto &fn_ast : fn_vec)
        fn_ast->codegen();
    fn_vec.clear();
    return llvm::Error::success();
}

Expected<std::unique_ptr<KaleidoscopeJIT>> KaleidoscopeJIT::Create() {

    auto JTMB = ExitOnErr(
        llvm::orc::JITTargetMachineBuilder::detectHost()
    );

    auto J = ExitOnErr(
        LLJITBuilder()
            .setJITTargetMachineBuilder(JTMB)
            .create()
    );

    return std::make_unique<KaleidoscopeJIT>(std::move(J));
}

llvm::orc::JITDylib &KaleidoscopeJIT::getMainJITDylib() {
    return JIT->getMainJITDylib();
}

KaleidoscopeJIT::KaleidoscopeJIT(std::unique_ptr<LLJIT> J)
        : JIT(std::move(J)),
          Mangle(this->JIT->getExecutionSession(),
                 this->JIT->getDataLayout()) {}

Error KaleidoscopeJIT::addModule(ThreadSafeModule TSM) {
    return JIT->addIRModule(std::move(TSM));
}


} // end namespace orc
} // end namespace llvm
