

#include <map>
#include <string>
#include <vector>
#include "../nsk_cpp.h"



std::map<std::string, std::string> functions_return_type;

void set_functions_return_type() {









  functions_return_type = {{"_glob_b_", "str_vec"}, {"glob", "str_vec"},
                           {"to_string", "str"}, {"cat_str_float", "str"}, {"str_split_idx", "str"}, {"str_to_float", "float"},
						{"IndexStrVec", "str"}, {"str_vec_Idx", "str"}, {"ShuffleStrVec", "str_vec"},
						{"map_Create", "map"}, {"map_node_reclaim", "int"}, {"hash_ptr", "int"}, {"map_size", "int"}, {"map_has_str", "bool"}, {"map_has_void_ptr", "bool"}, {"map_has_int", "bool"}, {"map_has_i64", "bool"}, {"map_get_str_int", ""}, {"map_get_i64_any", ""}, {"map_get_i64_int", ""}, {"map_has_float", "bool"}, {"map_has_char", "bool"}, {"map_print", "int"}, {"map_node_set_bucket", "int"}, {"map_node_set_next", "int"}, {"map_node_overwrite_bucket", "int"}, {"map_node_overwrite", "int"}, {"map_keys_str", "array"}, {"map_keys_void_ptr", "array"}, {"map_keys_array", "array"}, {"map_keys_i64", "array"}, {"map_keys", "array"}, {"map_values", "array"}, {"map_values_int", "array"}, {"map_clear", "int"}, 
						{"read_float", "float"}, {"float_ptr_print", "float"}, {"float_to_str", "str"}, {"float_to_str_buffer", "int"}, {"nsk_pow", "float"}, {"nsk_sqrt", "float"}, 
						{"charv_print", "int"}, 
						{"is_null", "bool"}, 
						{"array_Create", "array"}, {"array_clone", "array"}, {"array_slice", "array"}, {"array_size", "int"}, {"array_bad_idx", "int"}, {"array_clear", "float"}, {"array_int_NewVec", "array"}, {"array_float_NewVec", "array"}, {"array_void_NewVec", "array"}, {"array_print_int", "float"}, {"array_print_char", "float"}, {"arange_int", "array"}, {"zeros_int", "array"}, {"randint_array", "array"}, {"ones_int", "array"}, {"array_int_add", "array"}, {"randfloat_array", "array"}, {"array_print_float", "int"}, {"arange_float", "array"}, {"zeros_float", "array"}, {"ones_float", "array"}, {"array_sum_int", "int"}, {"array_prod_int", "int"}, {"array_Split_Parallel", "array"}, {"array_print_str", "int"}, {"array_shuffle_str", "int"}, {"hash_array_int", "int"}, {"array_eq_int", "bool"}, 
						{"randint", "int"}, {"randu", "float"}, {"randn", "float"}, 
						{"putchard", "float"}, {"printd", "float"}, 
						{"min", "float"}, {"max", "float"}, {"logE2f", "float"}, {"roundE", "float"}, {"floorE", "float"}, {"logical_not", "float"}, 
						{"object_Load_float", "float"}, {"object_Load_int", "int"}, {"object_Load_on_Offset_float", "float"}, {"object_Load_on_Offset_int", "int"}, 
						{"emerge_int", "int"}, {"emerge_float", "float"}, {"tHW_fn", "int"}, {"get_tid", "int"}, 
						{"_quit_", "float"}, {"fexists_C", "bool"}, 
						{"print", "float"}, {"print_void_ptrC", "float"}, {"print_int", "int"}, 
						{"GetEmptyChar", "str"}, {"CopyString", "str"}, {"ConcatStr", "str"}, {"ConcatStrFreeLeft", "str"}, {"ConcatFloatToStr", "str"}, {"ConcatNumToStrFree", "str"}, 
						{"read_int", "int"}, {"i64_to_str_buffer", "int"}, {"i16_to_str_buffer", "int"}, {"i8_to_str_buffer", "int"}, {"int_to_str_buffer", "int"}, {"int_print_bits", "int"}, {"i8_print_bits", "int"}, {"i16_print_bits", "int"}, {"i64_print_bits", "int"}, {"get_size", "int"}, 
						{"scope_struct_spec", "float"}, {"scope_struct_CreateFirst", ""}, {"scope_struct_Create", ""}, {"get_scope_thread_id", "int"}, {"scope_struct_Reset_Threads", "float"}, {"scope_struct_Increment_Thread", "float"}, {"ctx_print_buffer", "float"}, {"scope_struct_print", "float"}, 
						{"memp_start", "int"}, {"memp_end", "int"}, 
						{"psweep", "float"}, {"join_gc", "float"}, {"sweep", "float"}, {"GC_print", "float"}, 
						{"float_to_bf16", ""}, {"bf16_to_float", "float"}, {"bf16_to_str_buffer", "int"}, 
						{"str_eq", "bool"}, {"str_str_add", "str"}, {"str_float", "float"}, {"str_int_add", "str"}, {"str_float_add", "str"}, {"int_str_add", "str"}, {"float_str_add", "str"}, {"str_bool_add", "str"}, {"bool_str_add", "str"}, {"PrintStr", "float"}, {"cat_str_float", "str"}, {"str_split_idx", "str"}, {"can_convert_to_float", "bool"}, {"str_to_float", "float"}, {"str_str_different", "bool"}, {"str_str_equal", "bool"}, {"readline", "str"}, {"_glob_b_", "array"}, 
						{"channel_void_message", "int"}, {"str_channel_message", "str"}, {"channel_str_message", "int"}, {"str_channel_Idx", "str"}, {"str_channel_alive", "int"}, {"float_channel_terminate", "float"}, {"float_channel_alive", "int"}, {"int_channel_message", "int"}, {"channel_int_message", "float"}, {"int_channel_Idx", "int"}, {"int_channel_sum", "int"}, {"int_channel_terminate", "float"}, {"int_channel_alive", "bool"}, 
						{"dir_exists", "float"}, {"path_exists", "float"}, 
						{"prebuild", "int"}, 
						{"__slee_p_", "int"}, {"silent_sleep", "float"}, {"start_timer", "float"}, {"end_timer", "float"}, 
						{"CreateNotesVector", "list"}, {"Dispose_NotesVector", "float"}, {"Add_To_NotesVector_float", "list"}, {"Add_To_NotesVector_int", "list"}, {"Add_To_NotesVector_str", "list"}, 
						{"list_New", "list"}, {"list_append", "float"}, {"list_print", "float"}, {"tuple_print", "float"}, {"list_Create", "list"}, {"list_shuffle", "float"}, {"list_size", "int"}, {"list_CalculateIdx", "int"}, {"to_int", "int"}, {"to_float", "float"}, {"to_bool", "bool"}, {"int_list_Store_Idx", "float"}, {"float_list_Store_Idx", "float"}, {"list_Store_Idx", "float"}, {"zip", "list"}, 
						{"print_vec_i8", "int"}, {"print_vec_i16", "int"}, {"print_vec_int", "int"}, {"print_vec_i64", "int"}, {"print_vec_float", "int"}, 
						{"bool_to_str", "str"}, {"bool_to_str_buffer", "int"}, 
						{"__idx__", "int"}, {"__sliced_idx__", "int"}, 

	};

	fn_ret_dt["channel_void_message"] = Data_Tree("int");
	fn_ret_dt["str_channel_message"] = Data_Tree("str");
	fn_ret_dt["channel_str_message"] = Data_Tree("int");
	fn_ret_dt["str_channel_Idx"] = Data_Tree("str");
	fn_ret_dt["str_channel_alive"] = Data_Tree("int");
	fn_ret_dt["float_channel_terminate"] = Data_Tree("float");
	fn_ret_dt["float_channel_alive"] = Data_Tree("int");
	fn_ret_dt["int_channel_message"] = Data_Tree("int");
	fn_ret_dt["channel_int_message"] = Data_Tree("float");
	fn_ret_dt["int_channel_Idx"] = Data_Tree("int");
	fn_ret_dt["int_channel_sum"] = Data_Tree("int");
	fn_ret_dt["int_channel_terminate"] = Data_Tree("float");
	fn_ret_dt["int_channel_alive"] = Data_Tree("bool");
	fn_ret_dt["float_to_bf16"] = Data_Tree("");
	fn_ret_dt["bf16_to_float"] = Data_Tree("float");
	fn_ret_dt["bf16_to_str_buffer"] = Data_Tree("int");
	fn_ret_dt["map_Create"] = Data_Tree("map");
	fn_ret_dt["map_node_reclaim"] = Data_Tree("int");
	fn_ret_dt["hash_ptr"] = Data_Tree("int");
	fn_ret_dt["map_size"] = Data_Tree("int");
	fn_ret_dt["map_has_str"] = Data_Tree("bool");
	fn_ret_dt["map_has_void_ptr"] = Data_Tree("bool");
	fn_ret_dt["map_has_int"] = Data_Tree("bool");
	fn_ret_dt["map_has_i64"] = Data_Tree("bool");
	fn_ret_dt["map_get_str_int"] = Data_Tree("");
	fn_ret_dt["map_get_i64_any"] = Data_Tree("");
	fn_ret_dt["map_get_i64_int"] = Data_Tree("");
	fn_ret_dt["map_has_float"] = Data_Tree("bool");
	fn_ret_dt["map_has_char"] = Data_Tree("bool");
	fn_ret_dt["map_print"] = Data_Tree("int");
	fn_ret_dt["map_node_set_bucket"] = Data_Tree("int");
	fn_ret_dt["map_node_set_next"] = Data_Tree("int");
	fn_ret_dt["map_node_overwrite_bucket"] = Data_Tree("int");
	fn_ret_dt["map_node_overwrite"] = Data_Tree("int");
	fn_ret_dt["map_keys_str"] = Data_Tree("array");
	fn_ret_dt["map_keys_void_ptr"] = Data_Tree("array");
	fn_ret_dt["map_keys_array"] = Data_Tree("array");
	fn_ret_dt["map_keys_i64"] = Data_Tree("array");
	fn_ret_dt["map_keys"] = Data_Tree("array");
	fn_ret_dt["map_values"] = Data_Tree("array");
	fn_ret_dt["map_values_int"] = Data_Tree("array");
	fn_ret_dt["map_clear"] = Data_Tree("int");
	fn_ret_dt["CreateNotesVector"] = Data_Tree("list");
	fn_ret_dt["Dispose_NotesVector"] = Data_Tree("float");
	fn_ret_dt["Add_To_NotesVector_float"] = Data_Tree("list");
	fn_ret_dt["Add_To_NotesVector_int"] = Data_Tree("list");
	fn_ret_dt["Add_To_NotesVector_str"] = Data_Tree("list");
	fn_ret_dt["scope_struct_spec"] = Data_Tree("float");
	fn_ret_dt["scope_struct_CreateFirst"] = Data_Tree("");
	fn_ret_dt["scope_struct_Create"] = Data_Tree("");
	fn_ret_dt["get_scope_thread_id"] = Data_Tree("int");
	fn_ret_dt["scope_struct_Reset_Threads"] = Data_Tree("float");
	fn_ret_dt["scope_struct_Increment_Thread"] = Data_Tree("float");
	fn_ret_dt["ctx_print_buffer"] = Data_Tree("float");
	fn_ret_dt["scope_struct_print"] = Data_Tree("float");
	fn_ret_dt["bool_to_str"] = Data_Tree("str");
	fn_ret_dt["bool_to_str_buffer"] = Data_Tree("int");
	fn_ret_dt["memp_start"] = Data_Tree("int");
	fn_ret_dt["memp_end"] = Data_Tree("int");
	fn_ret_dt["charv_print"] = Data_Tree("int");
	fn_ret_dt["read_int"] = Data_Tree("int");
	fn_ret_dt["i64_to_str_buffer"] = Data_Tree("int");
	fn_ret_dt["i16_to_str_buffer"] = Data_Tree("int");
	fn_ret_dt["i8_to_str_buffer"] = Data_Tree("int");
	fn_ret_dt["int_to_str_buffer"] = Data_Tree("int");
	fn_ret_dt["int_print_bits"] = Data_Tree("int");
	fn_ret_dt["i8_print_bits"] = Data_Tree("int");
	fn_ret_dt["i16_print_bits"] = Data_Tree("int");
	fn_ret_dt["i64_print_bits"] = Data_Tree("int");
	fn_ret_dt["get_size"] = Data_Tree("int");
	fn_ret_dt["min"] = Data_Tree("float");
	fn_ret_dt["max"] = Data_Tree("float");
	fn_ret_dt["logE2f"] = Data_Tree("float");
	fn_ret_dt["roundE"] = Data_Tree("float");
	fn_ret_dt["floorE"] = Data_Tree("float");
	fn_ret_dt["logical_not"] = Data_Tree("float");
	fn_ret_dt["array_Create"] = Data_Tree("array");
	fn_ret_dt["array_clone"] = Data_Tree("array");
	fn_ret_dt["array_slice"] = Data_Tree("array");
	fn_ret_dt["array_size"] = Data_Tree("int");
	fn_ret_dt["array_bad_idx"] = Data_Tree("int");
	fn_ret_dt["array_clear"] = Data_Tree("float");
	fn_ret_dt["array_int_NewVec"] = Data_Tree("array");
	fn_ret_dt["array_float_NewVec"] = Data_Tree("array");
	fn_ret_dt["array_void_NewVec"] = Data_Tree("array");
	fn_ret_dt["array_print_int"] = Data_Tree("float");
	fn_ret_dt["array_print_char"] = Data_Tree("float");
	fn_ret_dt["arange_int"] = Data_Tree("array");
	fn_ret_dt["zeros_int"] = Data_Tree("array");
	fn_ret_dt["randint_array"] = Data_Tree("array");
	fn_ret_dt["ones_int"] = Data_Tree("array");
	fn_ret_dt["array_int_add"] = Data_Tree("array");
	fn_ret_dt["randfloat_array"] = Data_Tree("array");
	fn_ret_dt["array_print_float"] = Data_Tree("int");
	fn_ret_dt["arange_float"] = Data_Tree("array");
	fn_ret_dt["zeros_float"] = Data_Tree("array");
	fn_ret_dt["ones_float"] = Data_Tree("array");
	fn_ret_dt["array_sum_int"] = Data_Tree("int");
	fn_ret_dt["array_prod_int"] = Data_Tree("int");
	fn_ret_dt["array_Split_Parallel"] = Data_Tree("array");
	fn_ret_dt["array_print_str"] = Data_Tree("int");
	fn_ret_dt["array_shuffle_str"] = Data_Tree("int");
	fn_ret_dt["hash_array_int"] = Data_Tree("int");
	fn_ret_dt["array_eq_int"] = Data_Tree("bool");
	fn_ret_dt["is_null"] = Data_Tree("bool");
	fn_ret_dt["prebuild"] = Data_Tree("int");
	fn_ret_dt["str_eq"] = Data_Tree("bool");
	fn_ret_dt["str_str_add"] = Data_Tree("str");
	fn_ret_dt["str_float"] = Data_Tree("float");
	fn_ret_dt["str_int_add"] = Data_Tree("str");
	fn_ret_dt["str_float_add"] = Data_Tree("str");
	fn_ret_dt["int_str_add"] = Data_Tree("str");
	fn_ret_dt["float_str_add"] = Data_Tree("str");
	fn_ret_dt["str_bool_add"] = Data_Tree("str");
	fn_ret_dt["bool_str_add"] = Data_Tree("str");
	fn_ret_dt["PrintStr"] = Data_Tree("float");
	fn_ret_dt["cat_str_float"] = Data_Tree("str");
	fn_ret_dt["str_split_idx"] = Data_Tree("str");
	fn_ret_dt["can_convert_to_float"] = Data_Tree("bool");
	fn_ret_dt["str_to_float"] = Data_Tree("float");
	fn_ret_dt["str_str_different"] = Data_Tree("bool");
	fn_ret_dt["str_str_equal"] = Data_Tree("bool");
	fn_ret_dt["readline"] = Data_Tree("str");
	fn_ret_dt["_glob_b_"] = Data_Tree("array");
	fn_ret_dt["object_Load_float"] = Data_Tree("float");
	fn_ret_dt["object_Load_int"] = Data_Tree("int");
	fn_ret_dt["object_Load_on_Offset_float"] = Data_Tree("float");
	fn_ret_dt["object_Load_on_Offset_int"] = Data_Tree("int");
	fn_ret_dt["print"] = Data_Tree("float");
	fn_ret_dt["print_void_ptrC"] = Data_Tree("float");
	fn_ret_dt["print_int"] = Data_Tree("int");
	fn_ret_dt["_quit_"] = Data_Tree("float");
	fn_ret_dt["fexists_C"] = Data_Tree("bool");
	fn_ret_dt["GetEmptyChar"] = Data_Tree("str");
	fn_ret_dt["CopyString"] = Data_Tree("str");
	fn_ret_dt["ConcatStr"] = Data_Tree("str");
	fn_ret_dt["ConcatStrFreeLeft"] = Data_Tree("str");
	fn_ret_dt["ConcatFloatToStr"] = Data_Tree("str");
	fn_ret_dt["ConcatNumToStrFree"] = Data_Tree("str");
	fn_ret_dt["emerge_int"] = Data_Tree("int");
	fn_ret_dt["emerge_float"] = Data_Tree("float");
	fn_ret_dt["tHW_fn"] = Data_Tree("int");
	fn_ret_dt["get_tid"] = Data_Tree("int");
	fn_ret_dt["print_vec_i8"] = Data_Tree("int");
	fn_ret_dt["print_vec_i16"] = Data_Tree("int");
	fn_ret_dt["print_vec_int"] = Data_Tree("int");
	fn_ret_dt["print_vec_i64"] = Data_Tree("int");
	fn_ret_dt["print_vec_float"] = Data_Tree("int");
	fn_ret_dt["dir_exists"] = Data_Tree("float");
	fn_ret_dt["path_exists"] = Data_Tree("float");
	fn_ret_dt["psweep"] = Data_Tree("float");
	fn_ret_dt["join_gc"] = Data_Tree("float");
	fn_ret_dt["sweep"] = Data_Tree("float");
	fn_ret_dt["GC_print"] = Data_Tree("float");
	fn_ret_dt["list_New"] = Data_Tree("list");
	fn_ret_dt["list_append"] = Data_Tree("float");
	fn_ret_dt["list_print"] = Data_Tree("float");
	fn_ret_dt["tuple_print"] = Data_Tree("float");
	fn_ret_dt["list_Create"] = Data_Tree("list");
	fn_ret_dt["list_shuffle"] = Data_Tree("float");
	fn_ret_dt["list_size"] = Data_Tree("int");
	fn_ret_dt["list_CalculateIdx"] = Data_Tree("int");
	fn_ret_dt["to_int"] = Data_Tree("int");
	fn_ret_dt["to_float"] = Data_Tree("float");
	fn_ret_dt["to_bool"] = Data_Tree("bool");
	fn_ret_dt["int_list_Store_Idx"] = Data_Tree("float");
	fn_ret_dt["float_list_Store_Idx"] = Data_Tree("float");
	fn_ret_dt["list_Store_Idx"] = Data_Tree("float");
	fn_ret_dt["zip"] = Data_Tree("list");
	fn_ret_dt["read_float"] = Data_Tree("float");
	fn_ret_dt["float_ptr_print"] = Data_Tree("float");
	fn_ret_dt["float_to_str"] = Data_Tree("str");
	fn_ret_dt["float_to_str_buffer"] = Data_Tree("int");
	fn_ret_dt["nsk_pow"] = Data_Tree("float");
	fn_ret_dt["nsk_sqrt"] = Data_Tree("float");
	fn_ret_dt["randint"] = Data_Tree("int");
	fn_ret_dt["randu"] = Data_Tree("float");
	fn_ret_dt["randn"] = Data_Tree("float");
	fn_ret_dt["__slee_p_"] = Data_Tree("int");
	fn_ret_dt["silent_sleep"] = Data_Tree("float");
	fn_ret_dt["start_timer"] = Data_Tree("float");
	fn_ret_dt["end_timer"] = Data_Tree("float");
	fn_ret_dt["__idx__"] = Data_Tree("int");
	fn_ret_dt["__sliced_idx__"] = Data_Tree("int");
	fn_ret_dt["putchard"] = Data_Tree("float");
	fn_ret_dt["printd"] = Data_Tree("float");

}