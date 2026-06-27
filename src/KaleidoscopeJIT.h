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
#pragma once

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
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <memory>

#include "runtime/compiler_frontend/parser_struct.h"
#include "compiler_frontend/expressions.h"


class PrototypeAST;
class ExprAST;
class FunctionAST;


extern std::unordered_map<std::string, std::unique_ptr<FunctionAST>> GpuFunctions;

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  Parser_Struct parser_struct;
  std::unique_ptr<PrototypeAST> Proto;
  //std::vector<ExprAST> Body;
  std::vector<std::unique_ptr<ExprAST>> Body;

  public:
    FunctionAST(Parser_Struct, std::unique_ptr<PrototypeAST> Proto,
                std::vector<std::unique_ptr<ExprAST>> Body);
  
  const PrototypeAST& getProto() const;
  const std::string& getName() const;
  llvm::Function *codegen();
  llvm::Function *codegen_gpu(int idx=-1); // idx for compile time
};




/// This will compile FnAST to IR, rename the function to add the given
/// suffix (needed to prevent a name-clash with the function's stub),
/// and then take ownership of the module that the function was compiled
/// into.

namespace llvm {
namespace orc {


class KaleidoscopeJIT {
public:
    std::vector<std::unique_ptr<FunctionAST>> fn_vec;
    
    llvm::orc::MangleAndInterner Mangle;
    static Expected<std::unique_ptr<KaleidoscopeJIT>> Create();

    Error addModule(ThreadSafeModule TSM);
    JITDylib &getMainJITDylib();

    std::string MangleName(const std::string &Name);

    Error addAST(std::unique_ptr<FunctionAST> AST); 
    Error genAST(); 

    const DataLayout &getDataLayout() const {
        return JIT->getDataLayout();
    }
    KaleidoscopeJIT(std::unique_ptr<LLJIT> J);


    std::unique_ptr<LLJIT> JIT;
};

} // end namespace orc
} // end namespace llvm
