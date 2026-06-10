#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"

#include <iostream>
#include <memory>

using namespace llvm;

#include "../compiler_frontend/modules.h"

void Generate_LLVM_Functions() {
    PointerType *floatPtrTy, *int8PtrTy;

    floatPtrTy = Type::getFloatTy(*TheContext)->getPointerTo();
    int8PtrTy = Type::getInt8Ty(*TheContext)->getPointerTo();
    

	FunctionType *print_randomsTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_randoms", print_randomsTy);

	FunctionType *randintTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("randint", randintTy);

	FunctionType *randuTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("randu", randuTy);

	FunctionType *randnTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("randn", randnTy);

	FunctionType *printTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("print", printTy);

	FunctionType *print_void_ptrTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("print_void_ptr", print_void_ptrTy);

	FunctionType *print_void_ptrCTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("print_void_ptrC", print_void_ptrCTy);

	FunctionType *print_boolTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt1Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_bool", print_boolTy);

	FunctionType *print_int16Ty= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt16Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_int16", print_int16Ty);

	FunctionType *print_intTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_int", print_intTy);

	FunctionType *print_floatTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_float", print_floatTy);

	FunctionType *print_int64Ty= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt64Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_int64", print_int64Ty);

	FunctionType *print_uint64Ty= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt64Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_uint64", print_uint64Ty);

	FunctionType *print_codegenTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("print_codegen", print_codegenTy);

	FunctionType *print_codegen_silentTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("print_codegen_silent", print_codegen_silentTy);

	FunctionType *LogErrorCallTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt32Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("LogErrorCall", LogErrorCallTy);

	FunctionType *GC_array_append_str_barrierTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt16Ty(*TheContext), int8PtrTy, int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("GC_array_append_str_barrier", GC_array_append_str_barrierTy);

	FunctionType *GC_array_append_barrierTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt16Ty(*TheContext), int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("GC_array_append_barrier", GC_array_append_barrierTy);

	FunctionType *GC_write_barrier_strTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt16Ty(*TheContext), int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("GC_write_barrier_str", GC_write_barrier_strTy);

	FunctionType *GC_write_barrier_objTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{Type::getInt16Ty(*TheContext), int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("GC_write_barrier_obj", GC_write_barrier_objTy);

	FunctionType *read_floatTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("read_float", read_floatTy);

	FunctionType *float_ptr_printTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("float_ptr_print", float_ptr_printTy);

	FunctionType *float_to_strTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("float_to_str", float_to_strTy);

	FunctionType *float_to_str_bufferTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getFloatTy(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("float_to_str_buffer", float_to_str_bufferTy);

	FunctionType *nsk_powTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("nsk_pow", nsk_powTy);

	FunctionType *nsk_sqrtTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("nsk_sqrt", nsk_sqrtTy);

	FunctionType *dive_voidTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("dive_void", dive_voidTy);

	FunctionType *dive_intTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("dive_int", dive_intTy);

	FunctionType *dive_floatTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getFloatTy(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("dive_float", dive_floatTy);

	FunctionType *emerge_voidTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("emerge_void", emerge_voidTy);

	FunctionType *emerge_intTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("emerge_int", emerge_intTy);

	FunctionType *emerge_floatTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("emerge_float", emerge_floatTy);

	FunctionType *get_tidTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("get_tid", get_tidTy);





	FunctionType *map_CreateTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_Create", map_CreateTy);

	FunctionType *map_node_reclaimTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_node_reclaim", map_node_reclaimTy);

	FunctionType *map_sizeTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_size", map_sizeTy);

	FunctionType *map_expandTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_expand", map_expandTy);

	FunctionType *map_has_strTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy, struct_types["DT_str"]},
		false
	);
	TheModule->getOrInsertFunction("map_has_str", map_has_strTy);

	FunctionType *map_has_intTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_has_int", map_has_intTy);

	FunctionType *map_has_i64Ty= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getInt64Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_has_i64", map_has_i64Ty);

	FunctionType *map_get_str_intTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, struct_types["DT_str"]},
		false
	);
	TheModule->getOrInsertFunction("map_get_str_int", map_get_str_intTy);

	FunctionType *map_get_i64_anyTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getInt64Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_get_i64_any", map_get_i64_anyTy);

	FunctionType *map_get_i64_intTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getInt64Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_get_i64_int", map_get_i64_intTy);

	FunctionType *map_has_floatTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_has_float", map_has_floatTy);

	FunctionType *map_has_charTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_has_char", map_has_charTy);

	FunctionType *print_strTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("print_str", print_strTy);

	FunctionType *map_printTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_print", map_printTy);

	FunctionType *map_node_set_bucketTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_node_set_bucket", map_node_set_bucketTy);

	FunctionType *map_node_set_nextTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_node_set_next", map_node_set_nextTy);

	FunctionType *map_node_overwrite_bucketTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_node_overwrite_bucket", map_node_overwrite_bucketTy);

	FunctionType *map_node_overwriteTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_node_overwrite", map_node_overwriteTy);

	FunctionType *map_keys_strTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_keys_str", map_keys_strTy);

	FunctionType *map_keys_arrayTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_keys_array", map_keys_arrayTy);

	FunctionType *map_keys_i64Ty= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_keys_i64", map_keys_i64Ty);

	FunctionType *map_keysTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_keys", map_keysTy);

	FunctionType *map_valuesTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_values", map_valuesTy);

	FunctionType *map_values_intTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_values_int", map_values_intTy);

	FunctionType *map_bad_key_strTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_bad_key_str", map_bad_key_strTy);

	FunctionType *map_bad_key_intTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_bad_key_int", map_bad_key_intTy);

	FunctionType *map_bad_key_i64Ty= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt64Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_bad_key_i64", map_bad_key_i64Ty);

	FunctionType *map_bad_key_arrayTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_bad_key_array", map_bad_key_arrayTy);

	FunctionType *map_bad_key_floatTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("map_bad_key_float", map_bad_key_floatTy);

	FunctionType *map_clearTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("map_clear", map_clearTy);

	FunctionType *channel_CreateTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt16Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("channel_Create", channel_CreateTy);

	FunctionType *void_channel_messageTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("void_channel_message", void_channel_messageTy);

	FunctionType *channel_void_messageTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("channel_void_message", channel_void_messageTy);

	FunctionType *str_channel_messageTy= FunctionType::get(
		struct_types["DT_str"],
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("str_channel_message", str_channel_messageTy);

	FunctionType *channel_str_messageTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, struct_types["DT_str"]},
		false
	);
	TheModule->getOrInsertFunction("channel_str_message", channel_str_messageTy);

	FunctionType *str_channel_IdxTy= FunctionType::get(
		struct_types["DT_str"],
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, struct_types["DT_str"]->getPointerTo(), int8PtrTy, struct_types["DT_str"]->getPointerTo(), int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("str_channel_Idx", str_channel_IdxTy);

	FunctionType *str_channel_terminateTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("str_channel_terminate", str_channel_terminateTy);

	FunctionType *str_channel_aliveTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("str_channel_alive", str_channel_aliveTy);

	FunctionType *float_channel_terminateTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("float_channel_terminate", float_channel_terminateTy);

	FunctionType *float_channel_aliveTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("float_channel_alive", float_channel_aliveTy);

	FunctionType *int_channel_messageTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("int_channel_message", int_channel_messageTy);

	FunctionType *channel_int_messageTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("channel_int_message", channel_int_messageTy);

	FunctionType *int_channel_IdxTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("int_channel_Idx", int_channel_IdxTy);

	FunctionType *int_channel_sumTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("int_channel_sum", int_channel_sumTy);

	FunctionType *int_channel_terminateTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("int_channel_terminate", int_channel_terminateTy);

	FunctionType *int_channel_aliveTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("int_channel_alive", int_channel_aliveTy);

	FunctionType *CreateNotesVectorTy= FunctionType::get(
		int8PtrTy,
		{},
		false
	);
	TheModule->getOrInsertFunction("CreateNotesVector", CreateNotesVectorTy);

	FunctionType *Dispose_NotesVectorTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("Dispose_NotesVector", Dispose_NotesVectorTy);

	FunctionType *Add_To_NotesVector_floatTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("Add_To_NotesVector_float", Add_To_NotesVector_floatTy);

	FunctionType *Add_To_NotesVector_intTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("Add_To_NotesVector_int", Add_To_NotesVector_intTy);

	FunctionType *Add_To_NotesVector_strTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("Add_To_NotesVector_str", Add_To_NotesVector_strTy);

	FunctionType *memp_startTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("memp_start", memp_startTy);

	FunctionType *memp_endTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("memp_end", memp_endTy);

	FunctionType *_quit_Ty= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("_quit_", _quit_Ty);

	FunctionType *fexists_CTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("fexists_C", fexists_CTy);

	FunctionType *read_intTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("read_int", read_intTy);

	FunctionType *i64_to_str_bufferTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt64Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("i64_to_str_buffer", i64_to_str_bufferTy);

	FunctionType *i16_to_str_bufferTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt16Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("i16_to_str_buffer", i16_to_str_bufferTy);

	FunctionType *i8_to_str_bufferTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt8Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("i8_to_str_buffer", i8_to_str_bufferTy);

	FunctionType *int_to_str_bufferTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("int_to_str_buffer", int_to_str_bufferTy);

	FunctionType *int_print_bitsTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("int_print_bits", int_print_bitsTy);

	FunctionType *i8_print_bitsTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt8Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("i8_print_bits", i8_print_bitsTy);

	FunctionType *i16_print_bitsTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt16Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("i16_print_bits", i16_print_bitsTy);

	FunctionType *i64_print_bitsTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt64Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("i64_print_bits", i64_print_bitsTy);

	FunctionType *get_sizeTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("get_size", get_sizeTy);

	FunctionType *list_NewTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		true //vararg
	);
	TheModule->getOrInsertFunction("list_New", list_NewTy);

	FunctionType *list_append_intTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("list_append_int", list_append_intTy);

	FunctionType *list_append_floatTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("list_append_float", list_append_floatTy);

	FunctionType *list_append_boolTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getInt1Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("list_append_bool", list_append_boolTy);

	FunctionType *list_appendTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("list_append", list_appendTy);

	FunctionType *list_printTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("list_print", list_printTy);

	FunctionType *tuple_printTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("tuple_print", tuple_printTy);

	FunctionType *list_CreateTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("list_Create", list_CreateTy);

	FunctionType *list_shuffleTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("list_shuffle", list_shuffleTy);

	FunctionType *list_sizeTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("list_size", list_sizeTy);

	FunctionType *list_CalculateIdxTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		true //vararg
	);
	TheModule->getOrInsertFunction("list_CalculateIdx", list_CalculateIdxTy);

	FunctionType *to_intTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("to_int", to_intTy);

	FunctionType *to_floatTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("to_float", to_floatTy);

	FunctionType *to_boolTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("to_bool", to_boolTy);

	FunctionType *assign_wise_list_IdxTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("assign_wise_list_Idx", assign_wise_list_IdxTy);

	FunctionType *int_list_Store_IdxTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("int_list_Store_Idx", int_list_Store_IdxTy);

	FunctionType *float_list_Store_IdxTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getFloatTy(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("float_list_Store_Idx", float_list_Store_IdxTy);

	FunctionType *list_Store_IdxTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("list_Store_Idx", list_Store_IdxTy);

	FunctionType *zipTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		true //vararg
	);
	TheModule->getOrInsertFunction("zip", zipTy);

	FunctionType *list_IdxTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("list_Idx", list_IdxTy);

	FunctionType *tuple_IdxTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("tuple_Idx", tuple_IdxTy);

	FunctionType *__slee_p_Ty= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("__slee_p_", __slee_p_Ty);

	FunctionType *random_sleepTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("random_sleep", random_sleepTy);

	FunctionType *silent_sleepTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("silent_sleep", silent_sleepTy);

	FunctionType *start_timerTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("start_timer", start_timerTy);

	FunctionType *end_timerTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("end_timer", end_timerTy);

	FunctionType *dir_existsTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("dir_exists", dir_existsTy);

	FunctionType *path_existsTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("path_exists", path_existsTy);

	FunctionType *__idx__Ty= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		true //vararg
	);
	TheModule->getOrInsertFunction("__idx__", __idx__Ty);

	FunctionType *__sliced_idx__Ty= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		true //vararg
	);
	TheModule->getOrInsertFunction("__sliced_idx__", __sliced_idx__Ty);

	FunctionType *bool_to_strTy= FunctionType::get(
		int8PtrTy,
		{Type::getInt1Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("bool_to_str", bool_to_strTy);

	FunctionType *bool_to_str_bufferTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt1Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("bool_to_str_buffer", bool_to_str_bufferTy);

	FunctionType *GetEmptyCharTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("GetEmptyChar", GetEmptyCharTy);

	FunctionType *FreeCharFromFuncTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("FreeCharFromFunc", FreeCharFromFuncTy);

	FunctionType *FreeCharTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("FreeChar", FreeCharTy);

	FunctionType *CopyStringTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("CopyString", CopyStringTy);

	FunctionType *ConcatStrTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("ConcatStr", ConcatStrTy);

	FunctionType *ConcatStrFreeLeftTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("ConcatStrFreeLeft", ConcatStrFreeLeftTy);

	FunctionType *ConcatFloatToStrTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("ConcatFloatToStr", ConcatFloatToStrTy);

	FunctionType *ConcatNumToStrFreeTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("ConcatNumToStrFree", ConcatNumToStrFreeTy);

	FunctionType *minTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("min", minTy);

	FunctionType *maxTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("max", maxTy);

	FunctionType *logE2fTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("logE2f", logE2fTy);

	FunctionType *roundETy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("roundE", roundETy);

	FunctionType *floorETy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("floorE", floorETy);

	FunctionType *logical_notTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("logical_not", logical_notTy);

	FunctionType *charv_printTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("charv_print", charv_printTy);

	FunctionType *get_barrierTy= FunctionType::get(
		int8PtrTy,
		{Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("get_barrier", get_barrierTy);

	FunctionType *str_CopyTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("str_Copy", str_CopyTy);

	FunctionType *str_eqTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("str_eq", str_eqTy);

	FunctionType *str_str_addTy= FunctionType::get(
		struct_types["DT_str"],
		{int8PtrTy, struct_types["DT_str"], struct_types["DT_str"]},
		false
	);
	TheModule->getOrInsertFunction("str_str_add", str_str_addTy);

	FunctionType *str_floatTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, struct_types["DT_str"]},
		false
	);
	TheModule->getOrInsertFunction("str_float", str_floatTy);

	FunctionType *str_int_addTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("str_int_add", str_int_addTy);

	FunctionType *str_float_addTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("str_float_add", str_float_addTy);

	FunctionType *int_str_addTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("int_str_add", int_str_addTy);

	FunctionType *float_str_addTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getFloatTy(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("float_str_add", float_str_addTy);

	FunctionType *str_bool_addTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getInt1Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("str_bool_add", str_bool_addTy);

	FunctionType *bool_str_addTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt1Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("bool_str_add", bool_str_addTy);

	FunctionType *PrintStrTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("PrintStr", PrintStrTy);

	FunctionType *cat_str_floatTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("cat_str_float", cat_str_floatTy);

	FunctionType *str_split_idxTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("str_split_idx", str_split_idxTy);

	FunctionType *can_convert_to_floatTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("can_convert_to_float", can_convert_to_floatTy);

	FunctionType *str_to_floatTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("str_to_float", str_to_floatTy);

	FunctionType *str_str_differentTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("str_str_different", str_str_differentTy);

	FunctionType *str_str_equalTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("str_str_equal", str_str_equalTy);

	FunctionType *str_DeleteTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("str_Delete", str_DeleteTy);

	FunctionType *readlineTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("readline", readlineTy);

	FunctionType *_glob_b_Ty= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("_glob_b_", _glob_b_Ty);

	FunctionType *psweepTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("psweep", psweepTy);

	FunctionType *join_gcTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("join_gc", join_gcTy);

	FunctionType *sweepTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("sweep", sweepTy);

	FunctionType *scope_struct_Join_GCTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Join_GC", scope_struct_Join_GCTy);

	FunctionType *scope_struct_Alloc_GCTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Alloc_GC", scope_struct_Alloc_GCTy);

	FunctionType *GC_printTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("GC_print", GC_printTy);

	FunctionType *array_CreateTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt16Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("array_Create", array_CreateTy);

	FunctionType *array_cloneTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_clone", array_cloneTy);

	FunctionType *array_sliceTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("array_slice", array_sliceTy);

	FunctionType *array_popTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_pop", array_popTy);

	FunctionType *array_sizeTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_size", array_sizeTy);

	FunctionType *array_bad_idxTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("array_bad_idx", array_bad_idxTy);

	FunctionType *array_double_sizeTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_double_size", array_double_sizeTy);

	FunctionType *array_clearTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_clear", array_clearTy);

	FunctionType *array_int_NewVecTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		true //vararg
	);
	TheModule->getOrInsertFunction("array_int_NewVec", array_int_NewVecTy);

	FunctionType *array_void_NewVecTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		true //vararg
	);
	TheModule->getOrInsertFunction("array_void_NewVec", array_void_NewVecTy);

	FunctionType *array_print_intTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_print_int", array_print_intTy);

	FunctionType *array_print_charTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_print_char", array_print_charTy);

	FunctionType *arange_intTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("arange_int", arange_intTy);

	FunctionType *zeros_intTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("zeros_int", zeros_intTy);

	FunctionType *randint_arrayTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("randint_array", randint_arrayTy);

	FunctionType *ones_intTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("ones_int", ones_intTy);

	FunctionType *array_int_addTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("array_int_add", array_int_addTy);

	FunctionType *randfloat_arrayTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("randfloat_array", randfloat_arrayTy);

	FunctionType *array_print_floatTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_print_float", array_print_floatTy);

	FunctionType *arange_floatTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getFloatTy(*TheContext), Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("arange_float", arange_floatTy);

	FunctionType *zeros_floatTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("zeros_float", zeros_floatTy);

	FunctionType *ones_floatTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("ones_float", ones_floatTy);

	FunctionType *array_sum_intTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_sum_int", array_sum_intTy);

	FunctionType *array_prod_intTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_prod_int", array_prod_intTy);

	FunctionType *array_Split_ParallelTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_Split_Parallel", array_Split_ParallelTy);

	FunctionType *array_print_strTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_print_str", array_print_strTy);

	FunctionType *array_shuffle_strTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_shuffle_str", array_shuffle_strTy);

	FunctionType *hash_array_intTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("hash_array_int", hash_array_intTy);

	FunctionType *array_eq_intTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("array_eq_int", array_eq_intTy);

	FunctionType *print_vec_i8Ty= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_vec_i8", print_vec_i8Ty);

	FunctionType *print_vec_i16Ty= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_vec_i16", print_vec_i16Ty);

	FunctionType *print_vec_intTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_vec_int", print_vec_intTy);

	FunctionType *print_vec_i64Ty= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_vec_i64", print_vec_i64Ty);

	FunctionType *print_vec_floatTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("print_vec_float", print_vec_floatTy);

	FunctionType *putchardTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("putchard", putchardTy);

	FunctionType *printdTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("printd", printdTy);

	FunctionType *allocate_voidTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext), int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("allocate_void", allocate_voidTy);

	FunctionType *allocate_poolTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt16Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("allocate_pool", allocate_poolTy);

	FunctionType *offset_object_ptrTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("offset_object_ptr", offset_object_ptrTy);

	FunctionType *object_Attr_floatTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getFloatTy(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("object_Attr_float", object_Attr_floatTy);

	FunctionType *object_Attr_intTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("object_Attr_int", object_Attr_intTy);

	FunctionType *object_Load_floatTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("object_Load_float", object_Load_floatTy);

	FunctionType *object_Load_intTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("object_Load_int", object_Load_intTy);

	FunctionType *object_Load_slotTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("object_Load_slot", object_Load_slotTy);

	FunctionType *tie_object_to_objectTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("tie_object_to_object", tie_object_to_objectTy);

	FunctionType *object_Attr_on_Offset_floatTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getFloatTy(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("object_Attr_on_Offset_float", object_Attr_on_Offset_floatTy);

	FunctionType *object_Attr_on_Offset_intTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("object_Attr_on_Offset_int", object_Attr_on_Offset_intTy);

	FunctionType *object_Attr_on_OffsetTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("object_Attr_on_Offset", object_Attr_on_OffsetTy);

	FunctionType *object_Load_on_Offset_floatTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("object_Load_on_Offset_float", object_Load_on_Offset_floatTy);

	FunctionType *object_Load_on_Offset_intTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("object_Load_on_Offset_int", object_Load_on_Offset_intTy);

	FunctionType *object_Load_on_OffsetTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("object_Load_on_Offset", object_Load_on_OffsetTy);

	FunctionType *object_ptr_Load_on_OffsetTy= FunctionType::get(
		int8PtrTy,
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("object_ptr_Load_on_Offset", object_ptr_Load_on_OffsetTy);

	FunctionType *object_ptr_Attribute_objectTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("object_ptr_Attribute_object", object_ptr_Attribute_objectTy);

	FunctionType *print_stack1Ty= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("print_stack1", print_stack1Ty);

	FunctionType *print_stackTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("print_stack", print_stackTy);

	FunctionType *scope_struct_specTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_spec", scope_struct_specTy);

	FunctionType *set_scope_lineTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("set_scope_line", set_scope_lineTy);

	FunctionType *scope_struct_CreateFirstTy= FunctionType::get(
		int8PtrTy,
		{},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_CreateFirst", scope_struct_CreateFirstTy);

	FunctionType *scope_struct_CreateTy= FunctionType::get(
		int8PtrTy,
		{},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Create", scope_struct_CreateTy);

	FunctionType *set_scope_thread_idTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("set_scope_thread_id", set_scope_thread_idTy);

	FunctionType *get_scope_thread_idTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("get_scope_thread_id", get_scope_thread_idTy);

	FunctionType *scope_struct_Reset_ThreadsTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Reset_Threads", scope_struct_Reset_ThreadsTy);

	FunctionType *scope_struct_Increment_ThreadTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Increment_Thread", scope_struct_Increment_ThreadTy);

	FunctionType *scope_struct_PrintTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Print", scope_struct_PrintTy);

	FunctionType *scope_struct_Save_for_AsyncTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Save_for_Async", scope_struct_Save_for_AsyncTy);

	FunctionType *scope_struct_Load_for_AsyncTy= FunctionType::get(
		int8PtrTy,
		{Type::getInt32Ty(*TheContext), int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Load_for_Async", scope_struct_Load_for_AsyncTy);

	FunctionType *scope_struct_Store_Asyncs_CountTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Store_Asyncs_Count", scope_struct_Store_Asyncs_CountTy);

	FunctionType *scope_struct_Get_Async_ScopeTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext), Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Get_Async_Scope", scope_struct_Get_Async_ScopeTy);

	FunctionType *ctx_print_bufferTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy, Type::getInt32Ty(*TheContext)},
		false
	);
	TheModule->getOrInsertFunction("ctx_print_buffer", ctx_print_bufferTy);

	FunctionType *scope_struct_printTy= FunctionType::get(
		Type::getFloatTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_print", scope_struct_printTy);

	FunctionType *scope_struct_SweepTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Sweep", scope_struct_SweepTy);

	FunctionType *scope_struct_DeleteTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("scope_struct_Delete", scope_struct_DeleteTy);

	FunctionType *nullptr_getTy= FunctionType::get(
		int8PtrTy,
		{},
		false
	);
	TheModule->getOrInsertFunction("nullptr_get", nullptr_getTy);

	FunctionType *is_nullTy= FunctionType::get(
		Type::getInt1Ty(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("is_null", is_nullTy);

	FunctionType *LockMutexTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("LockMutex", LockMutexTy);

	FunctionType *UnlockMutexTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("UnlockMutex", UnlockMutexTy);

	FunctionType *Delete_PtrTy= FunctionType::get(
		Type::getVoidTy(*TheContext),
		{int8PtrTy},
		false
	);
	TheModule->getOrInsertFunction("Delete_Ptr", Delete_PtrTy);

	FunctionType *prebuildTy= FunctionType::get(
		Type::getInt32Ty(*TheContext),
		{},
		false
	);
	TheModule->getOrInsertFunction("prebuild", prebuildTy);

}