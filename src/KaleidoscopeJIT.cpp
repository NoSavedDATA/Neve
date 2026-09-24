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
#include <execution>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "KaleidoscopeJIT.h"
#include "compiler_frontend/expressions.h"
#include "compiler_frontend/ownership.h"
#include "compiler_frontend/function_ast.h"
#include "compiler_frontend/modules.h"
#include "runtime/common/extension_functions.h"
#include "runtime/compiler_frontend/global_vars.h"
#include "runtime/compiler_frontend/logging_v.h"
#include "llvm/Support/Error.h"


class PrototypeAST;
class ExprAST;


// void GetOwnedValues(ExprAST *expr, int &last_offset) {
//     if (auto *new_expr = dynamic_cast<NewExprAST*>(expr)) {
//         if (new_expr->IsOwn) {
//             new_expr->OwnedPoolOffset = last_offset;
//             last_offset += ClassSize[new_expr->DataName];
//         }
//     }
// }
//   // Set own pool
//   int last_offset=0;
//   for (auto &body : Body) {
//     body->Traverse([&last_offset](ExprAST *node) {
//         GetOwnedValues(node, last_offset);
//     });
//   }
 



/// FunctionAST - This class represents a function definition itself.
FunctionAST::FunctionAST(Parser_Struct *parser_struct, std::unique_ptr<PrototypeAST> Proto,
                std::vector<std::unique_ptr<ExprAST>> Body)
        : parser_struct(parser_struct), Proto(std::move(Proto)), Body(std::move(Body)) {
}
  



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
    fn_map[F->getName()] = F.get();
    fn_vec.push_back(std::move(F));
    return llvm::Error::success(); 
}


llvm::Error KaleidoscopeJIT::addGeneric(std::unique_ptr<FunctionAST> F) {
    Template_FnAST[F->getProto().getName()][F->getProto().CArgs] = F.get();
    // fn_map[F->getName()] = F.get();
    fn_generic_vec.push_back(std::move(F));
    return llvm::Error::success(); 
}



void gen_generics() {
    for(auto &[fn_ast, proto, parser_struct, fn, base_name] : generics_fn) {
        if (in_vec(base_name, fn_called)) {
            erase(fn_called, base_name);
            fn_ast->parser_struct->function_name = base_name;
            TheJIT->fn_map[base_name]->codegen();   
        }

        fn_ast->parser_struct->function_name = fn;
        fn_ast->parser_struct->cvalues = FunctionProtos[fn]->CArgs.cvalues;
        fn_ast->function_name = fn;
        // BasicBlock *CurBB = Builder->GetInsertBlock();
        // FunctionProtos[fn]->codegen();
        fn_ast->Proto = nullptr;
        fn_ast->codegen();
        // Builder->SetInsertPoint(CurBB);
    }
}

void warmup_generics() {
    for(auto &[fn_ast, proto, parser_struct, fn, base_name] : generics_fn) {
        
        if (parser_struct->gpu>0) {
            int gpu = parser_struct->gpu;
            parser_struct->gpu = (kernel_fn.count(base_name)>0) ? 1 : 2;
            proto->codegen();
            parser_struct->gpu = gpu;
        }
        if (!fn_ast)
            LogErrorC(-1, "Template for " +base_name + " failed");
        FunctionProtos[fn] = std::move(proto);
        // TheJIT->fn_map[fn] = fn_ast;
        // fn_called.push_back(fn);
    }
}

llvm::Error KaleidoscopeJIT::genAST() {
    std::cout << " -- codegen --" << "\n";
    warmup_generics();
    fn_map["__anon_expr"]->codegen(); // let main setup globals
    gen_generics();
    for (int i=fn_called.size()-1;i>=0;--i) {
        if (!fn_map.count(fn_called[i]))
            continue; // skip llvm defined fn
        fn_map[fn_called[i]]->codegen();
    }
    

    fn_called.clear();
    fn_vec.clear();
    fn_map.clear();
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
