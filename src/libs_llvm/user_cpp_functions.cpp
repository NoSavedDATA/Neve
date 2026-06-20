

#include <map>
#include <string>
#include <vector>

#include "../runtime/compiler_frontend/global_vars.h"
#include "../include.h"
#include "../nsk_cpp.h"




void set_user_functions() {


    user_cpp_functions = {"printtt",
        "tensor_repeat_interleave",
		"__idx__", "__sliced_idx__", 
		"psweep", "join_gc", "sweep", "scope_struct_Join_GC", "scope_struct_Alloc_GC", "GC_print", 
		"channel_Create", "void_channel_message", "channel_void_message", "str_channel_message", "channel_str_message", "str_channel_Idx", "str_channel_terminate", "str_channel_alive", "float_channel_terminate", "float_channel_alive", "int_channel_message", "channel_int_message", "int_channel_Idx", "int_channel_sum", "int_channel_terminate", "int_channel_alive", 
		"min", "max", "logE2f", "roundE", "floorE", "logical_not", 
		"LockMutex", "UnlockMutex", 
		"print_vec_i8", "print_vec_i16", "print_vec_int", "print_vec_i64", "print_vec_float", 
		"randint", "randu", "randn", 
		"read_int", "i64_to_str_buffer", "i16_to_str_buffer", "i8_to_str_buffer", "int_to_str_buffer", "int_print_bits", "i8_print_bits", "i16_print_bits", "i64_print_bits", "get_size", 
		"nullptr_get", "is_null", 
		"array_Create", "array_clone", "array_slice", "array_pop", "array_size", "array_bad_idx", "array_double_size", "array_clear", "array_int_NewVec", "array_void_NewVec", "array_print_int", "array_print_char", "arange_int", "zeros_int", "randint_array", "ones_int", "array_int_add", "randfloat_array", "array_print_float", "arange_float", "zeros_float", "ones_float", "array_sum_int", "array_prod_int", "array_Split_Parallel", "array_print_str", "array_shuffle_str", "hash_array_int", "array_eq_int", 
		"print_stack1", "print_stack", "scope_struct_spec", "set_scope_line", "scope_struct_CreateFirst", "scope_struct_Create", "set_scope_thread_id", "get_scope_thread_id", "scope_struct_Reset_Threads", "scope_struct_Increment_Thread", "scope_struct_Print", "scope_struct_Save_for_Async", "scope_struct_Load_for_Async", "scope_struct_Store_Asyncs_Count", "scope_struct_Get_Async_Scope", "ctx_print_buffer", "scope_struct_print", "scope_struct_Sweep", "scope_struct_Delete", 
		"_quit_", "fexists_C", 
		"offset_object_ptr", "object_Attr_float", "object_Attr_int", "object_Load_float", "object_Load_int", "object_Load_slot", "tie_object_to_object", "object_Attr_on_Offset_float", "object_Attr_on_Offset_int", "object_Attr_on_Offset", "object_Load_on_Offset_float", "object_Load_on_Offset_int", "object_Load_on_Offset", "object_ptr_Load_on_Offset", "object_ptr_Attribute_object", 
		"allocate_void", "allocate_pool", 
		"str_Copy", "str_eq", "str_str_add", "str_float", "str_int_add", "str_float_add", "int_str_add", "float_str_add", "str_bool_add", "bool_str_add", "PrintStr", "cat_str_float", "str_split_idx", "can_convert_to_float", "str_to_float", "str_str_different", "str_str_equal", "str_Delete", "readline", "_glob_b_", 
		"list_New", "list_append_int", "list_append_float", "list_append_bool", "list_append", "list_print", "tuple_print", "list_Create", "list_shuffle", "list_size", "list_CalculateIdx", "to_int", "to_float", "to_bool", "assign_wise_list_Idx", "int_list_Store_Idx", "float_list_Store_Idx", "list_Store_Idx", "zip", "list_Idx", "tuple_Idx", 
		"dive_void", "dive_int", "dive_float", "emerge_void", "emerge_int", "emerge_float", "tHW_fn", "get_tid", "pthread_create_aux", "pthread_join_aux", "pthread_create_aux", "pthread_join_aux", 
		"print", "print_void_ptr", "print_void_ptrC", "print_bool", "print_int16", "print_int", "print_float", "print_int64", "print_uint64", 
		"map_Create", "map_node_reclaim", "map_size", "map_expand", "map_has_str", "map_has_int", "map_has_i64", "map_get_str_int", "map_get_i64_any", "map_get_i64_int", "map_has_float", "map_has_char", "print_str", "map_print", "map_node_set_bucket", "map_node_set_next", "map_node_overwrite_bucket", "map_node_overwrite", "map_keys_str", "map_keys_array", "map_keys_i64", "map_keys", "map_values", "map_values_int", "map_bad_key_str", "map_bad_key_int", "map_bad_key_i64", "map_bad_key_array", "map_bad_key_float", "map_clear", 
		"dir_exists", "path_exists", 
		"charv_print", 
		"memp_start", "memp_end", 
		"putchard", "printd", 
		"CreateNotesVector", "Dispose_NotesVector", "Add_To_NotesVector_float", "Add_To_NotesVector_int", "Add_To_NotesVector_str", 
		"get_barrier", 
		"prebuild", 
		"read_float", "float_ptr_print", "float_to_str", "float_to_str_buffer", "nsk_pow", "nsk_sqrt", 
		"bool_to_str", "bool_to_str_buffer", 
		"Delete_Ptr", 
		"GC_array_append_str_barrier", "GC_array_append_barrier", "GC_write_barrier_str", "GC_write_barrier_obj", 
		"print_codegen", "print_codegen_silent", "LogErrorCall", 
		"GetEmptyChar", "FreeCharFromFunc", "FreeChar", "CopyString", "ConcatStr", "ConcatStrFreeLeft", "ConcatFloatToStr", "ConcatNumToStrFree", 
		"__slee_p_", "random_sleep", "silent_sleep", "start_timer", "end_timer", 

	};


	struct_create_fn["DT_charv_Create"] = DT_charv_Create;
	struct_create_fn["DT_vec_Create"] = DT_vec_Create;
	llvm_callee["fexists"] = fexists;
	llvm_callee["dsize"] = dsize;
	llvm_callee["printl"] = printl;
	llvm_callee["print"] = print;
	llvm_callee["print_bb"] = print_bb;
	llvm_callee["print_Value"] = print_Value;
	llvm_callee["to_char"] = to_char;
	llvm_callee["i8"] = i8;
	llvm_callee["i16"] = i16;
	llvm_callee["to_int"] = to_int;
	llvm_callee["i64"] = i64;
	llvm_callee["c_open"] = c_open;
	llvm_callee["c_read"] = c_read;
	llvm_callee["err"] = err;
	llvm_callee["c_malloc"] = c_malloc;
	llvm_callee["c_malloc32"] = c_malloc32;
	llvm_callee["c_malloc64"] = c_malloc64;
	llvm_callee["c_malloc_str"] = c_malloc_str;
	llvm_callee["c_strlen"] = c_strlen;
	llvm_callee["str_size"] = str_size;
	llvm_callee["c_memcpy"] = c_memcpy;
	llvm_callee["c_memchr"] = c_memchr;
	llvm_callee["str_set"] = str_set;
	llvm_callee["str_offset"] = str_offset;
	llvm_callee["alloc"] = alloc;
	llvm_callee["min"] = min;
	llvm_callee["max"] = max;
	llvm_callee["shfl_sync"] = shfl_sync;
	llvm_callee["swap_bit"] = swap_bit;
	llvm_callee["ctz"] = ctz;
	llvm_callee["simd_load"] = simd_load;
	llvm_callee["simd_store"] = simd_store;
	llvm_callee["vec_make"] = vec_make;
	llvm_callee["vec_shuffle"] = vec_shuffle;
	llvm_callee["vec_movemask"] = vec_movemask;
	llvm_callee["vec_print"] = vec_print;
	clean_up_functions["map"] = map_Clean_Up;

	clean_up_functions["channel"] = channel_Clean_Up;

	clean_up_functions["tensor"] = tensor_Clean_Up;

	clean_up_functions["array"] = array_Clean_Up;

	clean_up_functions["list"] = list_Clean_Up;


}