#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Target/TargetMachine.h"


#include "src/KaleidoscopeJIT.h"

#include <algorithm>
#include <cstdarg>
#include <cassert>
#include <cctype>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <iostream>
#include <numeric>
#include <utility>
#include <vector>
#include <iomanip>
#include <math.h>
#include <fenv.h>
#include <tuple>
#include <chrono>
#include <thread>
#include <random>
#include <float.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "src/include.h"
#include "src/pre_main.h"


using namespace llvm;
using namespace llvm::orc;


//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//



void linkExecutable() {
    // std::string out = R"(ld.lld-19 \
    //   /usr/lib/x86_64-linux-gnu/crt1.o \
    //   /usr/lib/x86_64-linux-gnu/crti.o \
    //   /usr/lib/gcc/x86_64-linux-gnu/9/crtbegin.o \
    //   output.o \
    //   --start-group static/runtime.a --end-group \
    //   -L/usr/lib/gcc/x86_64-linux-gnu/9 -lstdc++ \
    //   -L/lib/x86_64-linux-gnu -lgcc_s -lgcc -lc -lm \
    //   /usr/lib/gcc/x86_64-linux-gnu/9/crtend.o \
    //   /usr/lib/x86_64-linux-gnu/crtn.o \
    //   --dynamic-linker /lib64/ld-linux-x86-64.so.2 \
    //   -o output)";
    
    // std::string out = "clang++-19 -no-pie output.o static/runtime.a -o output";
    std::string out = "clang++-19 -static -static-libstdc++ -static-libgcc \
    output.o static/runtime.a -o output";
    std::cout << "" << out << "\n";
    system(out.c_str());
}

static void Compile() {
    for (auto &FuncAST : AllFunctions)
        FuncAST->codegen();
    
    std::error_code EC;
    raw_fd_ostream dest("output.o", EC, sys::fs::OF_None);

    legacy::PassManager pass;

    if (CTM->addPassesToEmitFile(pass, dest, nullptr,
                                 llvm::CodeGenFileType::ObjectFile)) {
        errs() << "TargetMachine can't emit object file\n";
        return;
    }

    pass.run(*TheModule);
    dest.flush();
    linkExecutable();
}





__attribute__((constructor))
void early_init() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter(); // Prepare for target hardware
  InitializeNativeTargetAsmParser();
  // llvm::InitializeNVPTXTarget();
  // llvm::InitializeNVPTXTargetInfo();
  // llvm::InitializeNVPTXTargetMC();
  // llvm::InitializeNVPTXAsmPrinter();
}

int main(int argc, char* argv[]) {
  IsJIT = false;
  for (int i = 1; i < argc; i++) {
    // std::cout << "Argument " << i << ": " << argv[i] << "\n";
    Sys_Arguments.push_back(argv[i]);
  }

  build_dicts();
  TheJIT = ExitOnErr(KaleidoscopeJIT::Create());
  InitializeModule();
  InitializeTokenizer();
  MainLoop();
  Compile();
  return 0;
}

