#include "llvm/IR/LLVMContext.h"


#include "../compiler_frontend/include.h"
#include "../compiler_frontend/modules.h"


std::map<std::string, StructType*> struct_types;
std::unordered_map<std::string, int> struct_type_size;

std::unordered_map<std::string, std::function<Value*(Parser_Struct, Function*, std::string, std::string, Data_Tree, Value*, Value*, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*>&)>> struct_create_fn;

std::unordered_map<std::string, std::function<Value*(Parser_Struct, Function *, std::string, Data_Tree, std::vector<Data_Tree>&, Value*, std::vector<std::unique_ptr<ExprAST>>&, std::vector<Value*>&)>> llvm_callee;


std::unordered_map<std::string, std::function<Value*(Parser_Struct, Function*, 
                                                       Data_Tree, Data_Tree,
                                                       std::unique_ptr<ExprAST>&,
                                                       std::unique_ptr<ExprAST>&,
                                                       Value*,
                                                       Value*, Value*)>> llvm_data_ops;

std::unordered_map<std::string, std::function<Value*(Parser_Struct, Function*, 
                                                       Data_Tree, Data_Tree,
                                                       std::unique_ptr<ExprAST>&,
                                                       std::unique_ptr<ExprAST>&,
                                                       Value*, Value*,
                                                       Value*, Value*)>> llvm_store_idx;


void Generate_Struct_Types() {
    // Get llvm types
    llvm::Type *int8PtrTy = Type::getInt8Ty(*TheContext)->getPointerTo();
    llvm::Type *int8Ty = Type::getInt8Ty(*TheContext);
    llvm::Type *boolTy = Type::getInt1Ty(*TheContext);
    llvm::Type *floatTy = Type::getFloatTy(*TheContext);
    llvm::Type *longTy   = Type::getInt64Ty(*TheContext);
    llvm::Type *int64Ty   = Type::getInt64Ty(*TheContext);
    llvm::Type *intTy = Type::getInt32Ty(*TheContext);
    llvm::Type *intPtrTy = Type::getInt32Ty(*TheContext)->getPointerTo();

    // std::vector<void*>
    StructType *void_vecTy = StructType::create(
        *TheContext,
        {int8PtrTy, longTy, longTy},
        "std.vector.int"
    );


    // --- DT_int_vecs ---
    // std::vector<int>
    StructType *vecIntTy = StructType::create(
        *TheContext,
        {intPtrTy, longTy, longTy},
        "std.vector.int"
    );
    // DT_int_vecs
    StructType *int_vecTy  = StructType::create(
        *TheContext,
        {intTy, vecIntTy, intPtrTy},
        "st.DT_int_vec"
    );
    struct_types["int_vec"] = int_vecTy;

    // DT_str
    struct_types["DT_str"] = StructType::create(
        *TheContext,
        {int8PtrTy, intTy},
        "st.DT_str"
    ); 

    // array
    struct_types["array"] = StructType::create(
        *TheContext,
        {intTy, intTy, intTy, int8PtrTy},
        "st.DT_array"
    ); 

    // --- map ---
    struct_types["map_node"]  = StructType::create(
        *TheContext,
        {int8PtrTy, int8PtrTy, int8PtrTy},
        "st.map_node"
    );
    // map
    struct_types["map"]  = StructType::create(
        *TheContext,
        {intTy, intTy, intTy, intTy, intTy, struct_types["map_node"]->getPointerTo()->getPointerTo()},
        "st.DT_map"
    );
    
    // --- Scope_Struct --- 
    // GC
    StructType *GC_Struct_Type = StructType::create(
        *TheContext,
        {intTy, longTy, longTy, boolTy, void_vecTy},
        "st.GC"
    );
    // Scope_Struct
    StructType *Scope_Struct_Type = StructType::create(
        *TheContext,
        {intTy, intTy, ArrayType::get(int8PtrTy, ContextStackSize),
         intTy, int8PtrTy, GC_Struct_Type,
         ArrayType::get(int8Ty, PrintBufferSize),
         intTy},
        "st.scope_struct"
    );
    struct_types["GC"] = GC_Struct_Type;
    struct_types["scope_struct"] = Scope_Struct_Type;
}

void Generate_Class_Types() {
    // high-level classes

    for (auto &class_pair : Classes) {
        const std::string &class_name = class_pair.first;
        // LogBlue(class_name);
        std::vector<llvm::Type*> types;
        for (auto &attr : ClassAttrsName[class_name]) {
            Data_Tree dt = data_typeVars[class_name][attr];
            types.push_back(get_type_from_data(dt));
        }
        StructType *st = StructType::create(
            *TheContext,
            types,
            "class_"+class_name
        );
        struct_types["class_"+class_name] = st;

        uint16_t type16 = data_name_to_type()[class_name];
        if (type_info[type16]==nullptr) {
            const llvm::DataLayout &DL = TheModule->getDataLayout();
            const llvm::StructLayout *layout = DL.getStructLayout(st);

            ClassSize[class_name] = layout->getSizeInBytes();

            int idx=0, pointers_count=0;
            std::vector<uint16_t> offsets, types16;
            for (auto &attr : ClassAttrsName[class_name]) {
                Data_Tree dt = data_typeVars[class_name][attr];
                // uint64_t offset = layout->getElementOffset(idx);
                // LogBlue("offset of " + std::to_string(idx) + " is " + std::to_string(offset));
                if (!in_vec(dt.Type, {primary_data_tokens}) && dt.Type!="charv") {
                    uint64_t offset = layout->getElementOffset(idx);
                    offsets.push_back(offset);
                    types16.push_back(data_name_to_type()[dt.Type]);
                    pointers_count++;
                }
                idx++;
            }

            // Handle class low level pointers information
            TypeInfo *class_info = (TypeInfo*)malloc(sizeof(TypeInfo) + pointers_count*sizeof(PtrInfo));
            type_info[type16] = class_info;
            class_info->pointers_count = pointers_count;
            for (int i=0; i<pointers_count; ++i) {
                PtrInfo *ptr_info = &class_info->ptr_info[i];
                ptr_info->offset = offsets[i];
                ptr_info->type = types16[i];
            }
        }
    }
}
