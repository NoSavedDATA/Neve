#pragma once

#include "../runtime/common/extension_functions.h"
#include "../runtime/data_types/include.h"
#include "../simd/include.h"
#include "../KaleidoscopeJIT.h"
#include "../nsk_cpp.h"

#include "expressions.h"
#include "include.h"
#include "logging.h"
#include "modules.h"
#include "parser.h"



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
}

inline Value *get_scope_obj(Value *scope_struct) {
        StructType *st = struct_types["scope_struct"]; 
        Value *obj_gep = Builder->CreateStructGEP(st, scope_struct, 4);
        return Builder->CreateLoad(int8PtrTy, obj_gep);
}


inline Value *get_scope_owned_pool(Value *scope_struct) {
  Value *owned_pool_gep = Builder->CreateStructGEP(
            struct_types["scope_struct"],
            scope_struct, 8
          );
  return Builder->CreateLoad(int8PtrTy, owned_pool_gep);
}

inline Value *get_scope_owned_pool(Value *scope_struct,
        Value *&prev_offset, Value *&prev_stride) {
  Value *owned_pool_gep = Builder->CreateStructGEP(
            struct_types["scope_struct"],
            scope_struct, 8
          );
  prev_offset = Builder->CreateLoad(intTy, 
                      Builder->CreateStructGEP(
                        struct_types["scope_struct"],
                        scope_struct, 10
                      ));
  prev_stride = Builder->CreateLoad(intTy, 
                      Builder->CreateStructGEP(
                        struct_types["scope_struct"],
                        scope_struct, 11
                      ));
  return Builder->CreateLoad(int8PtrTy, owned_pool_gep);
}


inline void set_scope_owned_pool(Value *scope_struct, Value *ownedpool) {
      Value *owned_pool_gep = Builder->CreateStructGEP(
                struct_types["scope_struct"],
                scope_struct, 8
              );
      Builder->CreateStore(
                ownedpool,
                owned_pool_gep);
}

inline void set_scope_owned_pool(Value *scope_struct, Value *ownedpool,
                    Value *prev_offset, Value *prev_stride) {
      Value *owned_pool_gep = Builder->CreateStructGEP(
                struct_types["scope_struct"],
                scope_struct, 8
              );
      Builder->CreateStore(
                ownedpool,
                owned_pool_gep);


      Builder->CreateStore(
                prev_offset,
                Builder->CreateStructGEP(
                    struct_types["scope_struct"],
                    scope_struct, 10
                  ));
      Builder->CreateStore(
                prev_stride,
                Builder->CreateStructGEP(
                    struct_types["scope_struct"],
                    scope_struct, 11
                  ));
}


inline void set_scope_retpool(
        Value *scope_struct, Value *ownedpool,
        int stride, int offset) {
    StructType *st = struct_types["scope_struct"];
    Value *retpool_gep = Builder->CreateStructGEP(
            st, scope_struct, 9
          );
    Value *retpool_offset_gep = Builder->CreateStructGEP(
            st, scope_struct, 10
          );
    Value *retpool_stride_gep = Builder->CreateStructGEP(
            st, scope_struct, 11
          );

    Value *owned_pool_offset = Builder->CreateGEP(
        int8Ty, ownedpool, const_int(offset)
    );

    Builder->CreateStore(owned_pool_offset, retpool_gep);
    Builder->CreateStore(const_int(0), retpool_offset_gep);
    Builder->CreateStore(const_int(stride), retpool_stride_gep);
}



inline void print_scope_escape_retoffset(
        Value *scope_struct) {
    StructType *st = struct_types["scope_struct"];

    Value *retpool_offset_gep = Builder->CreateStructGEP(
            st, scope_struct, 10
          );
    Value *offset = Builder->CreateLoad(intTy,retpool_offset_gep);
}


inline Value *get_scope_escape_retoffset(
        Value *scope_struct) {
    StructType *st = struct_types["scope_struct"];

    Value *retpool = 
        Builder->CreateLoad(int8PtrTy,
            Builder->CreateStructGEP(
                st, scope_struct, 9
              ));

    Value *retpool_offset_gep = Builder->CreateStructGEP(
            st, scope_struct, 10
          );
    Value *offset = Builder->CreateLoad(intTy,retpool_offset_gep);

    Value *stride = 
        Builder->CreateLoad(intTy,
            Builder->CreateStructGEP(
                st, scope_struct, 11
              ));
    // call("print_int", {offset});
    // call("print_int", {stride});

    Value *next_offset = Builder->CreateAdd(offset, stride);
    Builder->CreateStore(next_offset, retpool_offset_gep);

    Value *ret = Builder->CreateGEP(int8Ty, retpool, offset);
    return ret;
}
