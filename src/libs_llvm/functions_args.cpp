

#include <map>
#include <string>
#include <vector>


#include "../runtime/compiler_frontend/global_vars.h"




void set_functions_args_type() {

	
		
		
		
	
		
		Function_Arg_Types["print"]["0"] = "Scope_Struct";
		Function_Arg_Types["print"]["1"] = "str";
		
		Function_Arg_DataTypes["print"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["print"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["print"].push_back("0");
		Function_Arg_Names["print"].push_back("1");
		
		Function_Arg_Types["print_void_ptrC"]["0"] = "Scope_Struct";
		Function_Arg_Types["print_void_ptrC"]["1"] = "void";
		
		Function_Arg_DataTypes["print_void_ptrC"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["print_void_ptrC"]["1"] = Data_Tree("void");
		
		Function_Arg_Names["print_void_ptrC"].push_back("0");
		Function_Arg_Names["print_void_ptrC"].push_back("1");
	
		
		Function_Arg_Types["_quit_"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["_quit_"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["_quit_"].push_back("0");
		
		Function_Arg_Types["fexists_C"]["0"] = "Scope_Struct";
		Function_Arg_Types["fexists_C"]["1"] = "str";
		
		Function_Arg_DataTypes["fexists_C"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["fexists_C"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["fexists_C"].push_back("0");
		Function_Arg_Names["fexists_C"].push_back("1");
	
		
		
		
	
		
		Function_Arg_Types["GetEmptyChar"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["GetEmptyChar"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["GetEmptyChar"].push_back("0");
		
		Function_Arg_Types["CopyString"]["0"] = "Scope_Struct";
		Function_Arg_Types["CopyString"]["1"] = "str";
		
		Function_Arg_DataTypes["CopyString"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["CopyString"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["CopyString"].push_back("0");
		Function_Arg_Names["CopyString"].push_back("1");
		
		Function_Arg_Types["ConcatStr"]["0"] = "Scope_Struct";
		Function_Arg_Types["ConcatStr"]["1"] = "str";
		Function_Arg_Types["ConcatStr"]["2"] = "str";
		
		Function_Arg_DataTypes["ConcatStr"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["ConcatStr"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["ConcatStr"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["ConcatStr"].push_back("0");
		Function_Arg_Names["ConcatStr"].push_back("1");
		Function_Arg_Names["ConcatStr"].push_back("2");
		
		Function_Arg_Types["ConcatStrFreeLeft"]["0"] = "Scope_Struct";
		Function_Arg_Types["ConcatStrFreeLeft"]["1"] = "str";
		Function_Arg_Types["ConcatStrFreeLeft"]["2"] = "str";
		
		Function_Arg_DataTypes["ConcatStrFreeLeft"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["ConcatStrFreeLeft"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["ConcatStrFreeLeft"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["ConcatStrFreeLeft"].push_back("0");
		Function_Arg_Names["ConcatStrFreeLeft"].push_back("1");
		Function_Arg_Names["ConcatStrFreeLeft"].push_back("2");
		
		Function_Arg_Types["ConcatFloatToStr"]["0"] = "Scope_Struct";
		Function_Arg_Types["ConcatFloatToStr"]["1"] = "str";
		Function_Arg_Types["ConcatFloatToStr"]["2"] = "float";
		
		Function_Arg_DataTypes["ConcatFloatToStr"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["ConcatFloatToStr"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["ConcatFloatToStr"]["2"] = Data_Tree("float");
		
		Function_Arg_Names["ConcatFloatToStr"].push_back("0");
		Function_Arg_Names["ConcatFloatToStr"].push_back("1");
		Function_Arg_Names["ConcatFloatToStr"].push_back("2");
		
		Function_Arg_Types["ConcatNumToStrFree"]["0"] = "Scope_Struct";
		Function_Arg_Types["ConcatNumToStrFree"]["1"] = "str";
		Function_Arg_Types["ConcatNumToStrFree"]["2"] = "float";
		
		Function_Arg_DataTypes["ConcatNumToStrFree"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["ConcatNumToStrFree"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["ConcatNumToStrFree"]["2"] = Data_Tree("float");
		
		Function_Arg_Names["ConcatNumToStrFree"].push_back("0");
		Function_Arg_Names["ConcatNumToStrFree"].push_back("1");
		Function_Arg_Names["ConcatNumToStrFree"].push_back("2");
	
		
		Function_Arg_Types["print_stack1"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["print_stack1"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["print_stack1"].push_back("0");
		
		Function_Arg_Types["print_stack"]["0"] = "Scope_Struct";
		Function_Arg_Types["print_stack"]["1"] = "void";
		
		Function_Arg_DataTypes["print_stack"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["print_stack"]["1"] = Data_Tree("void");
		
		Function_Arg_Names["print_stack"].push_back("0");
		Function_Arg_Names["print_stack"].push_back("1");
		
		Function_Arg_Types["scope_struct_spec"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["scope_struct_spec"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["scope_struct_spec"].push_back("0");
		
		Function_Arg_Types["set_scope_line"]["0"] = "Scope_Struct";
		Function_Arg_Types["set_scope_line"]["1"] = "int";
		
		Function_Arg_DataTypes["set_scope_line"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["set_scope_line"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["set_scope_line"].push_back("0");
		Function_Arg_Names["set_scope_line"].push_back("1");
		
		
		
		
		
		
		
		Function_Arg_Types["set_scope_thread_id"]["0"] = "Scope_Struct";
		Function_Arg_Types["set_scope_thread_id"]["1"] = "int";
		
		Function_Arg_DataTypes["set_scope_thread_id"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["set_scope_thread_id"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["set_scope_thread_id"].push_back("0");
		Function_Arg_Names["set_scope_thread_id"].push_back("1");
		
		Function_Arg_Types["get_scope_thread_id"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["get_scope_thread_id"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["get_scope_thread_id"].push_back("0");
		
		Function_Arg_Types["scope_struct_Reset_Threads"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["scope_struct_Reset_Threads"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["scope_struct_Reset_Threads"].push_back("0");
		
		Function_Arg_Types["scope_struct_Increment_Thread"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["scope_struct_Increment_Thread"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["scope_struct_Increment_Thread"].push_back("0");
		
		Function_Arg_Types["scope_struct_Print"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["scope_struct_Print"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["scope_struct_Print"].push_back("0");
		
		Function_Arg_Types["scope_struct_Save_for_Async"]["0"] = "Scope_Struct";
		Function_Arg_Types["scope_struct_Save_for_Async"]["1"] = "str";
		
		Function_Arg_DataTypes["scope_struct_Save_for_Async"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["scope_struct_Save_for_Async"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["scope_struct_Save_for_Async"].push_back("0");
		Function_Arg_Names["scope_struct_Save_for_Async"].push_back("1");
		
		Function_Arg_Types["scope_struct_Store_Asyncs_Count"]["0"] = "Scope_Struct";
		Function_Arg_Types["scope_struct_Store_Asyncs_Count"]["1"] = "int";
		
		Function_Arg_DataTypes["scope_struct_Store_Asyncs_Count"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["scope_struct_Store_Asyncs_Count"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["scope_struct_Store_Asyncs_Count"].push_back("0");
		Function_Arg_Names["scope_struct_Store_Asyncs_Count"].push_back("1");
		
		Function_Arg_Types["scope_struct_Get_Async_Scope"]["0"] = "Scope_Struct";
		Function_Arg_Types["scope_struct_Get_Async_Scope"]["1"] = "int";
		Function_Arg_Types["scope_struct_Get_Async_Scope"]["2"] = "int";
		
		Function_Arg_DataTypes["scope_struct_Get_Async_Scope"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["scope_struct_Get_Async_Scope"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["scope_struct_Get_Async_Scope"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["scope_struct_Get_Async_Scope"].push_back("0");
		Function_Arg_Names["scope_struct_Get_Async_Scope"].push_back("1");
		Function_Arg_Names["scope_struct_Get_Async_Scope"].push_back("2");
		
		Function_Arg_Types["ctx_print_buffer"]["0"] = "Scope_Struct";
		Function_Arg_Types["ctx_print_buffer"]["1"] = "int";
		
		Function_Arg_DataTypes["ctx_print_buffer"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["ctx_print_buffer"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["ctx_print_buffer"].push_back("0");
		Function_Arg_Names["ctx_print_buffer"].push_back("1");
		
		Function_Arg_Types["scope_struct_print"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["scope_struct_print"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["scope_struct_print"].push_back("0");
		
		Function_Arg_Types["scope_struct_Sweep"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["scope_struct_Sweep"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["scope_struct_Sweep"].push_back("0");
		
		Function_Arg_Types["scope_struct_Delete"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["scope_struct_Delete"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["scope_struct_Delete"].push_back("0");
	
		
		Function_Arg_Types["allocate_void"]["0"] = "Scope_Struct";
		Function_Arg_Types["allocate_void"]["1"] = "int";
		Function_Arg_Types["allocate_void"]["2"] = "const";
		Function_Arg_Types["allocate_void"]["3"] = "type";
		Function_Arg_Types["allocate_void"]["4"] = "type";
		Function_Arg_Types["allocate_void"]["5"] = "auto";
		Function_Arg_Types["allocate_void"]["6"] = "it";
		Function_Arg_Types["allocate_void"]["7"] = "data_name_to_type";
		Function_Arg_Types["allocate_void"]["8"] = "data_name_to_type";
		Function_Arg_Types["allocate_void"]["9"] = "type";
		Function_Arg_Types["allocate_void"]["10"] = "type";
		Function_Arg_Types["allocate_void"]["11"] = "if";
		Function_Arg_Types["allocate_void"]["12"] = "it";
		Function_Arg_Types["allocate_void"]["13"] = "it";
		Function_Arg_Types["allocate_void"]["14"] = "data_name_to_type";
		Function_Arg_Types["allocate_void"]["15"] = "data_name_to_type";
		
		Function_Arg_DataTypes["allocate_void"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["allocate_void"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["allocate_void"]["2"] = Data_Tree("const");
		Function_Arg_DataTypes["allocate_void"]["3"] = Data_Tree("type");
		Function_Arg_DataTypes["allocate_void"]["4"] = Data_Tree("type");
		Function_Arg_DataTypes["allocate_void"]["5"] = Data_Tree("auto");
		Function_Arg_DataTypes["allocate_void"]["6"] = Data_Tree("it");
		Function_Arg_DataTypes["allocate_void"]["7"] = Data_Tree("data_name_to_type");
		Function_Arg_DataTypes["allocate_void"]["8"] = Data_Tree("data_name_to_type");
		Function_Arg_DataTypes["allocate_void"]["9"] = Data_Tree("type");
		Function_Arg_DataTypes["allocate_void"]["10"] = Data_Tree("type");
		Function_Arg_DataTypes["allocate_void"]["11"] = Data_Tree("if");
		Function_Arg_DataTypes["allocate_void"]["12"] = Data_Tree("it");
		Function_Arg_DataTypes["allocate_void"]["13"] = Data_Tree("it");
		Function_Arg_DataTypes["allocate_void"]["14"] = Data_Tree("data_name_to_type");
		Function_Arg_DataTypes["allocate_void"]["15"] = Data_Tree("data_name_to_type");
		
		Function_Arg_Names["allocate_void"].push_back("0");
		Function_Arg_Names["allocate_void"].push_back("1");
		Function_Arg_Names["allocate_void"].push_back("2");
		Function_Arg_Names["allocate_void"].push_back("3");
		Function_Arg_Names["allocate_void"].push_back("4");
		Function_Arg_Names["allocate_void"].push_back("5");
		Function_Arg_Names["allocate_void"].push_back("6");
		Function_Arg_Names["allocate_void"].push_back("7");
		Function_Arg_Names["allocate_void"].push_back("8");
		Function_Arg_Names["allocate_void"].push_back("9");
		Function_Arg_Names["allocate_void"].push_back("10");
		Function_Arg_Names["allocate_void"].push_back("11");
		Function_Arg_Names["allocate_void"].push_back("12");
		Function_Arg_Names["allocate_void"].push_back("13");
		Function_Arg_Names["allocate_void"].push_back("14");
		Function_Arg_Names["allocate_void"].push_back("15");
		
		Function_Arg_Types["allocate_pool"]["0"] = "Scope_Struct";
		Function_Arg_Types["allocate_pool"]["1"] = "int";
		Function_Arg_Types["allocate_pool"]["2"] = "uint16_t";
		
		Function_Arg_DataTypes["allocate_pool"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["allocate_pool"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["allocate_pool"]["2"] = Data_Tree("uint16_t");
		
		Function_Arg_Names["allocate_pool"].push_back("0");
		Function_Arg_Names["allocate_pool"].push_back("1");
		Function_Arg_Names["allocate_pool"].push_back("2");
	
		
		Function_Arg_Types["channel_Create"]["0"] = "Scope_Struct";
		Function_Arg_Types["channel_Create"]["1"] = "uint16_t";
		Function_Arg_Types["channel_Create"]["2"] = "int";
		
		Function_Arg_DataTypes["channel_Create"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["channel_Create"]["1"] = Data_Tree("uint16_t");
		Function_Arg_DataTypes["channel_Create"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["channel_Create"].push_back("0");
		Function_Arg_Names["channel_Create"].push_back("1");
		Function_Arg_Names["channel_Create"].push_back("2");
		
		Function_Arg_Types["str_channel_terminate"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_channel_terminate"]["1"] = "Channel";
		
		Function_Arg_DataTypes["str_channel_terminate"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_channel_terminate"]["1"] = Data_Tree("Channel");
		
		Function_Arg_Names["str_channel_terminate"].push_back("0");
		Function_Arg_Names["str_channel_terminate"].push_back("1");
		
		Function_Arg_Types["str_channel_alive"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_channel_alive"]["1"] = "Channel";
		
		Function_Arg_DataTypes["str_channel_alive"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_channel_alive"]["1"] = Data_Tree("Channel");
		
		Function_Arg_Names["str_channel_alive"].push_back("0");
		Function_Arg_Names["str_channel_alive"].push_back("1");
		
		Function_Arg_Types["float_channel_terminate"]["0"] = "Scope_Struct";
		Function_Arg_Types["float_channel_terminate"]["1"] = "Channel";
		
		Function_Arg_DataTypes["float_channel_terminate"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["float_channel_terminate"]["1"] = Data_Tree("Channel");
		
		Function_Arg_Names["float_channel_terminate"].push_back("0");
		Function_Arg_Names["float_channel_terminate"].push_back("1");
		
		Function_Arg_Types["float_channel_alive"]["0"] = "Scope_Struct";
		Function_Arg_Types["float_channel_alive"]["1"] = "Channel";
		
		Function_Arg_DataTypes["float_channel_alive"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["float_channel_alive"]["1"] = Data_Tree("Channel");
		
		Function_Arg_Names["float_channel_alive"].push_back("0");
		Function_Arg_Names["float_channel_alive"].push_back("1");
		
		Function_Arg_Types["int_channel_message"]["0"] = "Scope_Struct";
		Function_Arg_Types["int_channel_message"]["1"] = "void";
		Function_Arg_Types["int_channel_message"]["2"] = "Channel";
		
		Function_Arg_DataTypes["int_channel_message"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["int_channel_message"]["1"] = Data_Tree("void");
		Function_Arg_DataTypes["int_channel_message"]["2"] = Data_Tree("Channel");
		
		Function_Arg_Names["int_channel_message"].push_back("0");
		Function_Arg_Names["int_channel_message"].push_back("1");
		Function_Arg_Names["int_channel_message"].push_back("2");
		
		Function_Arg_Types["channel_int_message"]["0"] = "Scope_Struct";
		Function_Arg_Types["channel_int_message"]["1"] = "Channel";
		Function_Arg_Types["channel_int_message"]["2"] = "int";
		
		Function_Arg_DataTypes["channel_int_message"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["channel_int_message"]["1"] = Data_Tree("Channel");
		Function_Arg_DataTypes["channel_int_message"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["channel_int_message"].push_back("0");
		Function_Arg_Names["channel_int_message"].push_back("1");
		Function_Arg_Names["channel_int_message"].push_back("2");
		
		Function_Arg_Types["int_channel_sum"]["0"] = "Scope_Struct";
		Function_Arg_Types["int_channel_sum"]["1"] = "Channel";
		
		Function_Arg_DataTypes["int_channel_sum"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["int_channel_sum"]["1"] = Data_Tree("Channel");
		
		Function_Arg_Names["int_channel_sum"].push_back("0");
		Function_Arg_Names["int_channel_sum"].push_back("1");
		
		Function_Arg_Types["int_channel_terminate"]["0"] = "Scope_Struct";
		Function_Arg_Types["int_channel_terminate"]["1"] = "Channel";
		
		Function_Arg_DataTypes["int_channel_terminate"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["int_channel_terminate"]["1"] = Data_Tree("Channel");
		
		Function_Arg_Names["int_channel_terminate"].push_back("0");
		Function_Arg_Names["int_channel_terminate"].push_back("1");
		
		Function_Arg_Types["int_channel_alive"]["0"] = "Scope_Struct";
		Function_Arg_Types["int_channel_alive"]["1"] = "Channel";
		
		Function_Arg_DataTypes["int_channel_alive"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["int_channel_alive"]["1"] = Data_Tree("Channel");
		
		Function_Arg_Names["int_channel_alive"].push_back("0");
		Function_Arg_Names["int_channel_alive"].push_back("1");
	
		
		Function_Arg_Types["map_Create"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_Create"]["1"] = "Data_Tree";
		
		Function_Arg_DataTypes["map_Create"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_Create"]["1"] = Data_Tree("Data_Tree");
		
		Function_Arg_Names["map_Create"].push_back("0");
		Function_Arg_Names["map_Create"].push_back("1");
		
		Function_Arg_Types["map_node_reclaim"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_node_reclaim"]["1"] = "map";
		Function_Arg_Types["map_node_reclaim"]["2"] = "map_node";
		
		Function_Arg_DataTypes["map_node_reclaim"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_node_reclaim"]["1"] = Data_Tree("map");
		Function_Arg_DataTypes["map_node_reclaim"]["2"] = Data_Tree("map_node");
		
		Function_Arg_Names["map_node_reclaim"].push_back("0");
		Function_Arg_Names["map_node_reclaim"].push_back("1");
		Function_Arg_Names["map_node_reclaim"].push_back("2");
		
		Function_Arg_Types["map_size"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_size"]["1"] = "map";
		
		Function_Arg_DataTypes["map_size"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_size"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_size"].push_back("0");
		Function_Arg_Names["map_size"].push_back("1");
		
		Function_Arg_Types["map_expand"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_expand"]["1"] = "map";
		
		Function_Arg_DataTypes["map_expand"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_expand"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_expand"].push_back("0");
		Function_Arg_Names["map_expand"].push_back("1");
		
		Function_Arg_Types["map_has_str"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_has_str"]["1"] = "map";
		Function_Arg_Types["map_has_str"]["2"] = "str";
		
		Function_Arg_DataTypes["map_has_str"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_has_str"]["1"] = Data_Tree("map");
		Function_Arg_DataTypes["map_has_str"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["map_has_str"].push_back("0");
		Function_Arg_Names["map_has_str"].push_back("1");
		Function_Arg_Names["map_has_str"].push_back("2");
		
		Function_Arg_Types["map_has_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_has_int"]["1"] = "map";
		Function_Arg_Types["map_has_int"]["2"] = "int";
		
		Function_Arg_DataTypes["map_has_int"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_has_int"]["1"] = Data_Tree("map");
		Function_Arg_DataTypes["map_has_int"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["map_has_int"].push_back("0");
		Function_Arg_Names["map_has_int"].push_back("1");
		Function_Arg_Names["map_has_int"].push_back("2");
		
		Function_Arg_Types["map_has_i64"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_has_i64"]["1"] = "map";
		Function_Arg_Types["map_has_i64"]["2"] = "i64";
		
		Function_Arg_DataTypes["map_has_i64"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_has_i64"]["1"] = Data_Tree("map");
		Function_Arg_DataTypes["map_has_i64"]["2"] = Data_Tree("i64");
		
		Function_Arg_Names["map_has_i64"].push_back("0");
		Function_Arg_Names["map_has_i64"].push_back("1");
		Function_Arg_Names["map_has_i64"].push_back("2");
		
		Function_Arg_Types["map_has_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_has_float"]["1"] = "map";
		Function_Arg_Types["map_has_float"]["2"] = "float";
		
		Function_Arg_DataTypes["map_has_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_has_float"]["1"] = Data_Tree("map");
		Function_Arg_DataTypes["map_has_float"]["2"] = Data_Tree("float");
		
		Function_Arg_Names["map_has_float"].push_back("0");
		Function_Arg_Names["map_has_float"].push_back("1");
		Function_Arg_Names["map_has_float"].push_back("2");
		
		Function_Arg_Types["map_has_char"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_has_char"]["1"] = "map";
		Function_Arg_Types["map_has_char"]["2"] = "int";
		
		Function_Arg_DataTypes["map_has_char"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_has_char"]["1"] = Data_Tree("map");
		Function_Arg_DataTypes["map_has_char"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["map_has_char"].push_back("0");
		Function_Arg_Names["map_has_char"].push_back("1");
		Function_Arg_Names["map_has_char"].push_back("2");
		
		Function_Arg_Types["map_print"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_print"]["1"] = "map";
		
		Function_Arg_DataTypes["map_print"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_print"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_print"].push_back("0");
		Function_Arg_Names["map_print"].push_back("1");
		
		Function_Arg_Types["map_node_set_bucket"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_node_set_bucket"]["1"] = "map";
		Function_Arg_Types["map_node_set_bucket"]["2"] = "map_node";
		Function_Arg_Types["map_node_set_bucket"]["3"] = "int";
		
		Function_Arg_DataTypes["map_node_set_bucket"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_node_set_bucket"]["1"] = Data_Tree("map");
		Function_Arg_DataTypes["map_node_set_bucket"]["2"] = Data_Tree("map_node");
		Function_Arg_DataTypes["map_node_set_bucket"]["3"] = Data_Tree("int");
		
		Function_Arg_Names["map_node_set_bucket"].push_back("0");
		Function_Arg_Names["map_node_set_bucket"].push_back("1");
		Function_Arg_Names["map_node_set_bucket"].push_back("2");
		Function_Arg_Names["map_node_set_bucket"].push_back("3");
		
		Function_Arg_Types["map_node_set_next"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_node_set_next"]["1"] = "map";
		Function_Arg_Types["map_node_set_next"]["2"] = "map_node";
		Function_Arg_Types["map_node_set_next"]["3"] = "map_node";
		
		Function_Arg_DataTypes["map_node_set_next"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_node_set_next"]["1"] = Data_Tree("map");
		Function_Arg_DataTypes["map_node_set_next"]["2"] = Data_Tree("map_node");
		Function_Arg_DataTypes["map_node_set_next"]["3"] = Data_Tree("map_node");
		
		Function_Arg_Names["map_node_set_next"].push_back("0");
		Function_Arg_Names["map_node_set_next"].push_back("1");
		Function_Arg_Names["map_node_set_next"].push_back("2");
		Function_Arg_Names["map_node_set_next"].push_back("3");
		
		Function_Arg_Types["map_node_overwrite_bucket"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_node_overwrite_bucket"]["1"] = "map_node";
		Function_Arg_Types["map_node_overwrite_bucket"]["2"] = "map";
		Function_Arg_Types["map_node_overwrite_bucket"]["3"] = "int";
		
		Function_Arg_DataTypes["map_node_overwrite_bucket"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_node_overwrite_bucket"]["1"] = Data_Tree("map_node");
		Function_Arg_DataTypes["map_node_overwrite_bucket"]["2"] = Data_Tree("map");
		Function_Arg_DataTypes["map_node_overwrite_bucket"]["3"] = Data_Tree("int");
		
		Function_Arg_Names["map_node_overwrite_bucket"].push_back("0");
		Function_Arg_Names["map_node_overwrite_bucket"].push_back("1");
		Function_Arg_Names["map_node_overwrite_bucket"].push_back("2");
		Function_Arg_Names["map_node_overwrite_bucket"].push_back("3");
		
		Function_Arg_Types["map_node_overwrite"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_node_overwrite"]["1"] = "map";
		Function_Arg_Types["map_node_overwrite"]["2"] = "map_node";
		Function_Arg_Types["map_node_overwrite"]["3"] = "map_node";
		Function_Arg_Types["map_node_overwrite"]["4"] = "map_node";
		
		Function_Arg_DataTypes["map_node_overwrite"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_node_overwrite"]["1"] = Data_Tree("map");
		Function_Arg_DataTypes["map_node_overwrite"]["2"] = Data_Tree("map_node");
		Function_Arg_DataTypes["map_node_overwrite"]["3"] = Data_Tree("map_node");
		Function_Arg_DataTypes["map_node_overwrite"]["4"] = Data_Tree("map_node");
		
		Function_Arg_Names["map_node_overwrite"].push_back("0");
		Function_Arg_Names["map_node_overwrite"].push_back("1");
		Function_Arg_Names["map_node_overwrite"].push_back("2");
		Function_Arg_Names["map_node_overwrite"].push_back("3");
		Function_Arg_Names["map_node_overwrite"].push_back("4");
		
		Function_Arg_Types["map_keys_str"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_keys_str"]["1"] = "map";
		
		Function_Arg_DataTypes["map_keys_str"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_keys_str"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_keys_str"].push_back("0");
		Function_Arg_Names["map_keys_str"].push_back("1");
		
		Function_Arg_Types["map_keys_array"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_keys_array"]["1"] = "map";
		
		Function_Arg_DataTypes["map_keys_array"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_keys_array"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_keys_array"].push_back("0");
		Function_Arg_Names["map_keys_array"].push_back("1");
		
		Function_Arg_Types["map_keys_i64"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_keys_i64"]["1"] = "map";
		
		Function_Arg_DataTypes["map_keys_i64"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_keys_i64"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_keys_i64"].push_back("0");
		Function_Arg_Names["map_keys_i64"].push_back("1");
		
		Function_Arg_Types["map_keys"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_keys"]["1"] = "map";
		
		Function_Arg_DataTypes["map_keys"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_keys"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_keys"].push_back("0");
		Function_Arg_Names["map_keys"].push_back("1");
		
		Function_Arg_Types["map_values"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_values"]["1"] = "map";
		
		Function_Arg_DataTypes["map_values"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_values"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_values"].push_back("0");
		Function_Arg_Names["map_values"].push_back("1");
		
		Function_Arg_Types["map_values_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_values_int"]["1"] = "map";
		
		Function_Arg_DataTypes["map_values_int"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_values_int"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_values_int"].push_back("0");
		Function_Arg_Names["map_values_int"].push_back("1");
		
		Function_Arg_Types["map_bad_key_str"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_bad_key_str"]["1"] = "str";
		
		Function_Arg_DataTypes["map_bad_key_str"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_bad_key_str"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["map_bad_key_str"].push_back("0");
		Function_Arg_Names["map_bad_key_str"].push_back("1");
		
		Function_Arg_Types["map_bad_key_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_bad_key_int"]["1"] = "int";
		
		Function_Arg_DataTypes["map_bad_key_int"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_bad_key_int"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["map_bad_key_int"].push_back("0");
		Function_Arg_Names["map_bad_key_int"].push_back("1");
		
		Function_Arg_Types["map_bad_key_i64"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_bad_key_i64"]["1"] = "i64";
		
		Function_Arg_DataTypes["map_bad_key_i64"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_bad_key_i64"]["1"] = Data_Tree("i64");
		
		Function_Arg_Names["map_bad_key_i64"].push_back("0");
		Function_Arg_Names["map_bad_key_i64"].push_back("1");
		
		Function_Arg_Types["map_bad_key_array"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_bad_key_array"]["1"] = "array";
		
		Function_Arg_DataTypes["map_bad_key_array"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree map_bad_key_array_1 = Data_Tree("array");
		map_bad_key_array_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["map_bad_key_array"]["1"] = map_bad_key_array_1;
		
		Function_Arg_Names["map_bad_key_array"].push_back("0");
		Function_Arg_Names["map_bad_key_array"].push_back("1");
		
		Function_Arg_Types["map_bad_key_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_bad_key_float"]["1"] = "float";
		
		Function_Arg_DataTypes["map_bad_key_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_bad_key_float"]["1"] = Data_Tree("float");
		
		Function_Arg_Names["map_bad_key_float"].push_back("0");
		Function_Arg_Names["map_bad_key_float"].push_back("1");
		
		Function_Arg_Types["map_clear"]["0"] = "Scope_Struct";
		Function_Arg_Types["map_clear"]["1"] = "map";
		
		Function_Arg_DataTypes["map_clear"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["map_clear"]["1"] = Data_Tree("map");
		
		Function_Arg_Names["map_clear"].push_back("0");
		Function_Arg_Names["map_clear"].push_back("1");
	
		
		Function_Arg_Types["bool_to_str_buffer"]["0"] = "Scope_Struct";
		Function_Arg_Types["bool_to_str_buffer"]["1"] = "bool";
		Function_Arg_Types["bool_to_str_buffer"]["2"] = "str";
		
		Function_Arg_DataTypes["bool_to_str_buffer"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["bool_to_str_buffer"]["1"] = Data_Tree("bool");
		Function_Arg_DataTypes["bool_to_str_buffer"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["bool_to_str_buffer"].push_back("0");
		Function_Arg_Names["bool_to_str_buffer"].push_back("1");
		Function_Arg_Names["bool_to_str_buffer"].push_back("2");
	
		
		Function_Arg_Types["array_Create"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_Create"]["1"] = "i16";
		
		Function_Arg_DataTypes["array_Create"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["array_Create"]["1"] = Data_Tree("i16");
		
		Function_Arg_Names["array_Create"].push_back("0");
		Function_Arg_Names["array_Create"].push_back("1");
		
		Function_Arg_Types["array_clone"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_clone"]["1"] = "array";
		
		Function_Arg_DataTypes["array_clone"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_clone_1 = Data_Tree("array");
		array_clone_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_clone"]["1"] = array_clone_1;
		
		Function_Arg_Names["array_clone"].push_back("0");
		Function_Arg_Names["array_clone"].push_back("1");
		
		Function_Arg_Types["array_pop"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_pop"]["1"] = "array";
		
		Function_Arg_DataTypes["array_pop"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_pop_1 = Data_Tree("array");
		array_pop_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_pop"]["1"] = array_pop_1;
		
		Function_Arg_Names["array_pop"].push_back("0");
		Function_Arg_Names["array_pop"].push_back("1");
		
		Function_Arg_Types["array_size"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_size"]["1"] = "array";
		
		Function_Arg_DataTypes["array_size"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_size_1 = Data_Tree("array");
		array_size_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_size"]["1"] = array_size_1;
		
		Function_Arg_Names["array_size"].push_back("0");
		Function_Arg_Names["array_size"].push_back("1");
		
		Function_Arg_Types["array_double_size"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_double_size"]["1"] = "array";
		
		Function_Arg_DataTypes["array_double_size"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_double_size_1 = Data_Tree("array");
		array_double_size_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_double_size"]["1"] = array_double_size_1;
		
		Function_Arg_Names["array_double_size"].push_back("0");
		Function_Arg_Names["array_double_size"].push_back("1");
		
		Function_Arg_Types["array_clear"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_clear"]["1"] = "array";
		
		Function_Arg_DataTypes["array_clear"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_clear_1 = Data_Tree("array");
		array_clear_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_clear"]["1"] = array_clear_1;
		
		Function_Arg_Names["array_clear"].push_back("0");
		Function_Arg_Names["array_clear"].push_back("1");
		
		Function_Arg_Types["array_int_NewVec"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_int_NewVec"]["1"] = "int";
		Function_Arg_Types["array_int_NewVec"]["2"] = "int";
		Function_Arg_Types["array_int_NewVec"]["3"] = "int";
		Function_Arg_Types["array_int_NewVec"]["4"] = "int";
		Function_Arg_Types["array_int_NewVec"]["5"] = "int";
		Function_Arg_Types["array_int_NewVec"]["6"] = "int";
		Function_Arg_Types["array_int_NewVec"]["7"] = "int";
		Function_Arg_Types["array_int_NewVec"]["8"] = "int";
		Function_Arg_Types["array_int_NewVec"]["9"] = "int";
		Function_Arg_Types["array_int_NewVec"]["10"] = "int";
		Function_Arg_Types["array_int_NewVec"]["11"] = "int";
		
		Function_Arg_DataTypes["array_int_NewVec"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["array_int_NewVec"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["2"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["3"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["4"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["5"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["6"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["7"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["8"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["9"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["10"] = Data_Tree("int");
		Function_Arg_DataTypes["array_int_NewVec"]["11"] = Data_Tree("int");
		
		Function_Arg_Names["array_int_NewVec"].push_back("0");
		Function_Arg_Names["array_int_NewVec"].push_back("1");
		Function_Arg_Names["array_int_NewVec"].push_back("2");
		Function_Arg_Names["array_int_NewVec"].push_back("3");
		Function_Arg_Names["array_int_NewVec"].push_back("4");
		Function_Arg_Names["array_int_NewVec"].push_back("5");
		Function_Arg_Names["array_int_NewVec"].push_back("6");
		Function_Arg_Names["array_int_NewVec"].push_back("7");
		Function_Arg_Names["array_int_NewVec"].push_back("8");
		Function_Arg_Names["array_int_NewVec"].push_back("9");
		Function_Arg_Names["array_int_NewVec"].push_back("10");
		Function_Arg_Names["array_int_NewVec"].push_back("11");
		
		Function_Arg_Types["array_void_NewVec"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_void_NewVec"]["1"] = "void";
		Function_Arg_Types["array_void_NewVec"]["2"] = "void";
		Function_Arg_Types["array_void_NewVec"]["3"] = "void";
		Function_Arg_Types["array_void_NewVec"]["4"] = "void";
		Function_Arg_Types["array_void_NewVec"]["5"] = "void";
		Function_Arg_Types["array_void_NewVec"]["6"] = "void";
		Function_Arg_Types["array_void_NewVec"]["7"] = "void";
		Function_Arg_Types["array_void_NewVec"]["8"] = "void";
		Function_Arg_Types["array_void_NewVec"]["9"] = "void";
		Function_Arg_Types["array_void_NewVec"]["10"] = "void";
		Function_Arg_Types["array_void_NewVec"]["11"] = "void";
		
		Function_Arg_DataTypes["array_void_NewVec"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["array_void_NewVec"]["1"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["2"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["3"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["4"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["5"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["6"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["7"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["8"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["9"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["10"] = Data_Tree("void");
		Function_Arg_DataTypes["array_void_NewVec"]["11"] = Data_Tree("void");
		
		Function_Arg_Names["array_void_NewVec"].push_back("0");
		Function_Arg_Names["array_void_NewVec"].push_back("1");
		Function_Arg_Names["array_void_NewVec"].push_back("2");
		Function_Arg_Names["array_void_NewVec"].push_back("3");
		Function_Arg_Names["array_void_NewVec"].push_back("4");
		Function_Arg_Names["array_void_NewVec"].push_back("5");
		Function_Arg_Names["array_void_NewVec"].push_back("6");
		Function_Arg_Names["array_void_NewVec"].push_back("7");
		Function_Arg_Names["array_void_NewVec"].push_back("8");
		Function_Arg_Names["array_void_NewVec"].push_back("9");
		Function_Arg_Names["array_void_NewVec"].push_back("10");
		Function_Arg_Names["array_void_NewVec"].push_back("11");
		
		Function_Arg_Types["array_print_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_print_int"]["1"] = "array";
		
		Function_Arg_DataTypes["array_print_int"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_print_int_1 = Data_Tree("array");
		array_print_int_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_print_int"]["1"] = array_print_int_1;
		
		Function_Arg_Names["array_print_int"].push_back("0");
		Function_Arg_Names["array_print_int"].push_back("1");
		
		Function_Arg_Types["array_print_char"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_print_char"]["1"] = "array";
		
		Function_Arg_DataTypes["array_print_char"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_print_char_1 = Data_Tree("array");
		array_print_char_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_print_char"]["1"] = array_print_char_1;
		
		Function_Arg_Names["array_print_char"].push_back("0");
		Function_Arg_Names["array_print_char"].push_back("1");
		
		Function_Arg_Types["arange_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["arange_int"]["1"] = "int";
		Function_Arg_Types["arange_int"]["2"] = "int";
		
		Function_Arg_DataTypes["arange_int"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["arange_int"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["arange_int"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["arange_int"].push_back("0");
		Function_Arg_Names["arange_int"].push_back("1");
		Function_Arg_Names["arange_int"].push_back("2");
		
		Function_Arg_Types["zeros_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["zeros_int"]["1"] = "int";
		
		Function_Arg_DataTypes["zeros_int"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["zeros_int"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["zeros_int"].push_back("0");
		Function_Arg_Names["zeros_int"].push_back("1");
		
		Function_Arg_Types["randint_array"]["0"] = "Scope_Struct";
		Function_Arg_Types["randint_array"]["1"] = "int";
		Function_Arg_Types["randint_array"]["2"] = "int";
		Function_Arg_Types["randint_array"]["3"] = "int";
		
		Function_Arg_DataTypes["randint_array"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["randint_array"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["randint_array"]["2"] = Data_Tree("int");
		Function_Arg_DataTypes["randint_array"]["3"] = Data_Tree("int");
		
		Function_Arg_Names["randint_array"].push_back("0");
		Function_Arg_Names["randint_array"].push_back("1");
		Function_Arg_Names["randint_array"].push_back("2");
		Function_Arg_Names["randint_array"].push_back("3");
		
		Function_Arg_Types["ones_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["ones_int"]["1"] = "int";
		
		Function_Arg_DataTypes["ones_int"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["ones_int"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["ones_int"].push_back("0");
		Function_Arg_Names["ones_int"].push_back("1");
		
		Function_Arg_Types["array_int_add"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_int_add"]["1"] = "array";
		Function_Arg_Types["array_int_add"]["2"] = "int";
		
		Function_Arg_DataTypes["array_int_add"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_int_add_1 = Data_Tree("array");
		array_int_add_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_int_add"]["1"] = array_int_add_1;
		Function_Arg_DataTypes["array_int_add"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["array_int_add"].push_back("0");
		Function_Arg_Names["array_int_add"].push_back("1");
		Function_Arg_Names["array_int_add"].push_back("2");
		
		Function_Arg_Types["randfloat_array"]["0"] = "Scope_Struct";
		Function_Arg_Types["randfloat_array"]["1"] = "int";
		Function_Arg_Types["randfloat_array"]["2"] = "float";
		Function_Arg_Types["randfloat_array"]["3"] = "float";
		
		Function_Arg_DataTypes["randfloat_array"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["randfloat_array"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["randfloat_array"]["2"] = Data_Tree("float");
		Function_Arg_DataTypes["randfloat_array"]["3"] = Data_Tree("float");
		
		Function_Arg_Names["randfloat_array"].push_back("0");
		Function_Arg_Names["randfloat_array"].push_back("1");
		Function_Arg_Names["randfloat_array"].push_back("2");
		Function_Arg_Names["randfloat_array"].push_back("3");
		
		Function_Arg_Types["array_print_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_print_float"]["1"] = "array";
		
		Function_Arg_DataTypes["array_print_float"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_print_float_1 = Data_Tree("array");
		array_print_float_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_print_float"]["1"] = array_print_float_1;
		
		Function_Arg_Names["array_print_float"].push_back("0");
		Function_Arg_Names["array_print_float"].push_back("1");
		
		Function_Arg_Types["arange_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["arange_float"]["1"] = "float";
		Function_Arg_Types["arange_float"]["2"] = "float";
		
		Function_Arg_DataTypes["arange_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["arange_float"]["1"] = Data_Tree("float");
		Function_Arg_DataTypes["arange_float"]["2"] = Data_Tree("float");
		
		Function_Arg_Names["arange_float"].push_back("0");
		Function_Arg_Names["arange_float"].push_back("1");
		Function_Arg_Names["arange_float"].push_back("2");
		
		Function_Arg_Types["zeros_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["zeros_float"]["1"] = "int";
		
		Function_Arg_DataTypes["zeros_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["zeros_float"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["zeros_float"].push_back("0");
		Function_Arg_Names["zeros_float"].push_back("1");
		
		Function_Arg_Types["ones_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["ones_float"]["1"] = "int";
		
		Function_Arg_DataTypes["ones_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["ones_float"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["ones_float"].push_back("0");
		Function_Arg_Names["ones_float"].push_back("1");
		
		Function_Arg_Types["array_sum_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_sum_int"]["1"] = "array";
		
		Function_Arg_DataTypes["array_sum_int"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_sum_int_1 = Data_Tree("array");
		array_sum_int_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_sum_int"]["1"] = array_sum_int_1;
		
		Function_Arg_Names["array_sum_int"].push_back("0");
		Function_Arg_Names["array_sum_int"].push_back("1");
		
		Function_Arg_Types["array_prod_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_prod_int"]["1"] = "array";
		
		Function_Arg_DataTypes["array_prod_int"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_prod_int_1 = Data_Tree("array");
		array_prod_int_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_prod_int"]["1"] = array_prod_int_1;
		
		Function_Arg_Names["array_prod_int"].push_back("0");
		Function_Arg_Names["array_prod_int"].push_back("1");
		
		Function_Arg_Types["array_Split_Parallel"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_Split_Parallel"]["1"] = "array";
		
		Function_Arg_DataTypes["array_Split_Parallel"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_Split_Parallel_1 = Data_Tree("array");
		array_Split_Parallel_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_Split_Parallel"]["1"] = array_Split_Parallel_1;
		
		Function_Arg_Names["array_Split_Parallel"].push_back("0");
		Function_Arg_Names["array_Split_Parallel"].push_back("1");
		
		Function_Arg_Types["array_print_str"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_print_str"]["1"] = "array";
		
		Function_Arg_DataTypes["array_print_str"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_print_str_1 = Data_Tree("array");
		array_print_str_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_print_str"]["1"] = array_print_str_1;
		
		Function_Arg_Names["array_print_str"].push_back("0");
		Function_Arg_Names["array_print_str"].push_back("1");
		
		Function_Arg_Types["array_shuffle_str"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_shuffle_str"]["1"] = "array";
		
		Function_Arg_DataTypes["array_shuffle_str"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_shuffle_str_1 = Data_Tree("array");
		array_shuffle_str_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_shuffle_str"]["1"] = array_shuffle_str_1;
		
		Function_Arg_Names["array_shuffle_str"].push_back("0");
		Function_Arg_Names["array_shuffle_str"].push_back("1");
		
		Function_Arg_Types["hash_array_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["hash_array_int"]["1"] = "array";
		
		Function_Arg_DataTypes["hash_array_int"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree hash_array_int_1 = Data_Tree("array");
		hash_array_int_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["hash_array_int"]["1"] = hash_array_int_1;
		
		Function_Arg_Names["hash_array_int"].push_back("0");
		Function_Arg_Names["hash_array_int"].push_back("1");
		
		Function_Arg_Types["array_eq_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["array_eq_int"]["1"] = "array";
		Function_Arg_Types["array_eq_int"]["2"] = "array";
		
		Function_Arg_DataTypes["array_eq_int"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree array_eq_int_1 = Data_Tree("array");
		array_eq_int_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_eq_int"]["1"] = array_eq_int_1;
		Data_Tree array_eq_int_2 = Data_Tree("array");
		array_eq_int_2.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["array_eq_int"]["2"] = array_eq_int_2;
		
		Function_Arg_Names["array_eq_int"].push_back("0");
		Function_Arg_Names["array_eq_int"].push_back("1");
		Function_Arg_Names["array_eq_int"].push_back("2");
	
		
		Function_Arg_Types["get_tid"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["get_tid"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["get_tid"].push_back("0");
	
		
		Function_Arg_Types["__slee_p_"]["0"] = "Scope_Struct";
		Function_Arg_Types["__slee_p_"]["1"] = "int";
		
		Function_Arg_DataTypes["__slee_p_"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["__slee_p_"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["__slee_p_"].push_back("0");
		Function_Arg_Names["__slee_p_"].push_back("1");
		
		Function_Arg_Types["random_sleep"]["0"] = "Scope_Struct";
		Function_Arg_Types["random_sleep"]["1"] = "int";
		Function_Arg_Types["random_sleep"]["2"] = "int";
		
		Function_Arg_DataTypes["random_sleep"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["random_sleep"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["random_sleep"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["random_sleep"].push_back("0");
		Function_Arg_Names["random_sleep"].push_back("1");
		Function_Arg_Names["random_sleep"].push_back("2");
		
		Function_Arg_Types["silent_sleep"]["0"] = "Scope_Struct";
		Function_Arg_Types["silent_sleep"]["1"] = "int";
		
		Function_Arg_DataTypes["silent_sleep"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["silent_sleep"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["silent_sleep"].push_back("0");
		Function_Arg_Names["silent_sleep"].push_back("1");
		
		Function_Arg_Types["start_timer"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["start_timer"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["start_timer"].push_back("0");
		
		Function_Arg_Types["end_timer"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["end_timer"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["end_timer"].push_back("0");
	
		
		
		
	
		
		Function_Arg_Types["read_float"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["read_float"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["read_float"].push_back("0");
		
		Function_Arg_Types["float_ptr_print"]["0"] = "Scope_Struct";
		Function_Arg_Types["float_ptr_print"]["1"] = "float";
		Function_Arg_Types["float_ptr_print"]["2"] = "int";
		
		Function_Arg_DataTypes["float_ptr_print"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["float_ptr_print"]["1"] = Data_Tree("float");
		Function_Arg_DataTypes["float_ptr_print"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["float_ptr_print"].push_back("0");
		Function_Arg_Names["float_ptr_print"].push_back("1");
		Function_Arg_Names["float_ptr_print"].push_back("2");
		
		Function_Arg_Types["float_to_str"]["0"] = "Scope_Struct";
		Function_Arg_Types["float_to_str"]["1"] = "float";
		
		Function_Arg_DataTypes["float_to_str"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["float_to_str"]["1"] = Data_Tree("float");
		
		Function_Arg_Names["float_to_str"].push_back("0");
		Function_Arg_Names["float_to_str"].push_back("1");
		
		Function_Arg_Types["float_to_str_buffer"]["0"] = "Scope_Struct";
		Function_Arg_Types["float_to_str_buffer"]["1"] = "float";
		Function_Arg_Types["float_to_str_buffer"]["2"] = "str";
		
		Function_Arg_DataTypes["float_to_str_buffer"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["float_to_str_buffer"]["1"] = Data_Tree("float");
		Function_Arg_DataTypes["float_to_str_buffer"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["float_to_str_buffer"].push_back("0");
		Function_Arg_Names["float_to_str_buffer"].push_back("1");
		Function_Arg_Names["float_to_str_buffer"].push_back("2");
		
		Function_Arg_Types["nsk_pow"]["0"] = "Scope_Struct";
		Function_Arg_Types["nsk_pow"]["1"] = "float";
		Function_Arg_Types["nsk_pow"]["2"] = "float";
		
		Function_Arg_DataTypes["nsk_pow"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["nsk_pow"]["1"] = Data_Tree("float");
		Function_Arg_DataTypes["nsk_pow"]["2"] = Data_Tree("float");
		
		Function_Arg_Names["nsk_pow"].push_back("0");
		Function_Arg_Names["nsk_pow"].push_back("1");
		Function_Arg_Names["nsk_pow"].push_back("2");
		
		Function_Arg_Types["nsk_sqrt"]["0"] = "Scope_Struct";
		Function_Arg_Types["nsk_sqrt"]["1"] = "float";
		
		Function_Arg_DataTypes["nsk_sqrt"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["nsk_sqrt"]["1"] = Data_Tree("float");
		
		Function_Arg_Names["nsk_sqrt"].push_back("0");
		Function_Arg_Names["nsk_sqrt"].push_back("1");
	
		
		Function_Arg_Types["object_Attr_on_Offset_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["object_Attr_on_Offset_float"]["1"] = "float";
		Function_Arg_Types["object_Attr_on_Offset_float"]["2"] = "int";
		
		Function_Arg_DataTypes["object_Attr_on_Offset_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["object_Attr_on_Offset_float"]["1"] = Data_Tree("float");
		Function_Arg_DataTypes["object_Attr_on_Offset_float"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["object_Attr_on_Offset_float"].push_back("0");
		Function_Arg_Names["object_Attr_on_Offset_float"].push_back("1");
		Function_Arg_Names["object_Attr_on_Offset_float"].push_back("2");
		
		Function_Arg_Types["object_Attr_on_Offset_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["object_Attr_on_Offset_int"]["1"] = "int";
		Function_Arg_Types["object_Attr_on_Offset_int"]["2"] = "int";
		
		Function_Arg_DataTypes["object_Attr_on_Offset_int"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["object_Attr_on_Offset_int"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["object_Attr_on_Offset_int"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["object_Attr_on_Offset_int"].push_back("0");
		Function_Arg_Names["object_Attr_on_Offset_int"].push_back("1");
		Function_Arg_Names["object_Attr_on_Offset_int"].push_back("2");
		
		Function_Arg_Types["object_Attr_on_Offset"]["0"] = "Scope_Struct";
		Function_Arg_Types["object_Attr_on_Offset"]["1"] = "void";
		Function_Arg_Types["object_Attr_on_Offset"]["2"] = "int";
		
		Function_Arg_DataTypes["object_Attr_on_Offset"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["object_Attr_on_Offset"]["1"] = Data_Tree("void");
		Function_Arg_DataTypes["object_Attr_on_Offset"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["object_Attr_on_Offset"].push_back("0");
		Function_Arg_Names["object_Attr_on_Offset"].push_back("1");
		Function_Arg_Names["object_Attr_on_Offset"].push_back("2");
		
		Function_Arg_Types["object_Load_on_Offset_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["object_Load_on_Offset_float"]["1"] = "int";
		
		Function_Arg_DataTypes["object_Load_on_Offset_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["object_Load_on_Offset_float"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["object_Load_on_Offset_float"].push_back("0");
		Function_Arg_Names["object_Load_on_Offset_float"].push_back("1");
		
		Function_Arg_Types["object_Load_on_Offset_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["object_Load_on_Offset_int"]["1"] = "int";
		
		Function_Arg_DataTypes["object_Load_on_Offset_int"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["object_Load_on_Offset_int"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["object_Load_on_Offset_int"].push_back("0");
		Function_Arg_Names["object_Load_on_Offset_int"].push_back("1");
		
		Function_Arg_Types["object_Load_on_Offset"]["0"] = "Scope_Struct";
		Function_Arg_Types["object_Load_on_Offset"]["1"] = "int";
		
		Function_Arg_DataTypes["object_Load_on_Offset"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["object_Load_on_Offset"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["object_Load_on_Offset"].push_back("0");
		Function_Arg_Names["object_Load_on_Offset"].push_back("1");
	
		
		Function_Arg_Types["psweep"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["psweep"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["psweep"].push_back("0");
		
		Function_Arg_Types["join_gc"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["join_gc"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["join_gc"].push_back("0");
		
		Function_Arg_Types["sweep"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["sweep"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["sweep"].push_back("0");
		
		Function_Arg_Types["scope_struct_Join_GC"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["scope_struct_Join_GC"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["scope_struct_Join_GC"].push_back("0");
		
		Function_Arg_Types["scope_struct_Alloc_GC"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["scope_struct_Alloc_GC"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["scope_struct_Alloc_GC"].push_back("0");
		
		Function_Arg_Types["GC_print"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["GC_print"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["GC_print"].push_back("0");
	
		
		Function_Arg_Types["read_int"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["read_int"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["read_int"].push_back("0");
		
		Function_Arg_Types["int_to_str"]["0"] = "Scope_Struct";
		Function_Arg_Types["int_to_str"]["1"] = "int";
		
		Function_Arg_DataTypes["int_to_str"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["int_to_str"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["int_to_str"].push_back("0");
		Function_Arg_Names["int_to_str"].push_back("1");
		
		Function_Arg_Types["i64_to_str_buffer"]["0"] = "Scope_Struct";
		Function_Arg_Types["i64_to_str_buffer"]["1"] = "i64";
		Function_Arg_Types["i64_to_str_buffer"]["2"] = "str";
		
		Function_Arg_DataTypes["i64_to_str_buffer"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["i64_to_str_buffer"]["1"] = Data_Tree("i64");
		Function_Arg_DataTypes["i64_to_str_buffer"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["i64_to_str_buffer"].push_back("0");
		Function_Arg_Names["i64_to_str_buffer"].push_back("1");
		Function_Arg_Names["i64_to_str_buffer"].push_back("2");
		
		Function_Arg_Types["int_to_str_buffer"]["0"] = "Scope_Struct";
		Function_Arg_Types["int_to_str_buffer"]["1"] = "int";
		Function_Arg_Types["int_to_str_buffer"]["2"] = "str";
		
		Function_Arg_DataTypes["int_to_str_buffer"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["int_to_str_buffer"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["int_to_str_buffer"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["int_to_str_buffer"].push_back("0");
		Function_Arg_Names["int_to_str_buffer"].push_back("1");
		Function_Arg_Names["int_to_str_buffer"].push_back("2");
		
		Function_Arg_Types["int_print_bits"]["0"] = "Scope_Struct";
		Function_Arg_Types["int_print_bits"]["1"] = "int";
		
		Function_Arg_DataTypes["int_print_bits"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["int_print_bits"]["1"] = Data_Tree("int");
		
		Function_Arg_Names["int_print_bits"].push_back("0");
		Function_Arg_Names["int_print_bits"].push_back("1");
	
		
		Function_Arg_Types["list_New"]["0"] = "Scope_Struct";
		Function_Arg_Types["list_New"]["1"] = "str";
		Function_Arg_Types["list_New"]["2"] = "str";
		Function_Arg_Types["list_New"]["3"] = "str";
		Function_Arg_Types["list_New"]["4"] = "str";
		Function_Arg_Types["list_New"]["5"] = "str";
		Function_Arg_Types["list_New"]["6"] = "str";
		Function_Arg_Types["list_New"]["7"] = "str";
		Function_Arg_Types["list_New"]["8"] = "str";
		Function_Arg_Types["list_New"]["9"] = "str";
		Function_Arg_Types["list_New"]["10"] = "str";
		Function_Arg_Types["list_New"]["11"] = "str";
		
		Function_Arg_DataTypes["list_New"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["list_New"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["2"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["3"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["4"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["5"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["6"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["7"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["8"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["9"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["10"] = Data_Tree("str");
		Function_Arg_DataTypes["list_New"]["11"] = Data_Tree("str");
		
		Function_Arg_Names["list_New"].push_back("0");
		Function_Arg_Names["list_New"].push_back("1");
		Function_Arg_Names["list_New"].push_back("2");
		Function_Arg_Names["list_New"].push_back("3");
		Function_Arg_Names["list_New"].push_back("4");
		Function_Arg_Names["list_New"].push_back("5");
		Function_Arg_Names["list_New"].push_back("6");
		Function_Arg_Names["list_New"].push_back("7");
		Function_Arg_Names["list_New"].push_back("8");
		Function_Arg_Names["list_New"].push_back("9");
		Function_Arg_Names["list_New"].push_back("10");
		Function_Arg_Names["list_New"].push_back("11");
		
		Function_Arg_Types["list_append_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["list_append_int"]["1"] = "list";
		Function_Arg_Types["list_append_int"]["2"] = "int";
		
		Function_Arg_DataTypes["list_append_int"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree list_append_int_1 = Data_Tree("list");
		list_append_int_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["list_append_int"]["1"] = list_append_int_1;
		Function_Arg_DataTypes["list_append_int"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["list_append_int"].push_back("0");
		Function_Arg_Names["list_append_int"].push_back("1");
		Function_Arg_Names["list_append_int"].push_back("2");
		
		Function_Arg_Types["list_append_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["list_append_float"]["1"] = "list";
		Function_Arg_Types["list_append_float"]["2"] = "float";
		
		Function_Arg_DataTypes["list_append_float"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree list_append_float_1 = Data_Tree("list");
		list_append_float_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["list_append_float"]["1"] = list_append_float_1;
		Function_Arg_DataTypes["list_append_float"]["2"] = Data_Tree("float");
		
		Function_Arg_Names["list_append_float"].push_back("0");
		Function_Arg_Names["list_append_float"].push_back("1");
		Function_Arg_Names["list_append_float"].push_back("2");
		
		Function_Arg_Types["list_append_bool"]["0"] = "Scope_Struct";
		Function_Arg_Types["list_append_bool"]["1"] = "list";
		Function_Arg_Types["list_append_bool"]["2"] = "bool";
		
		Function_Arg_DataTypes["list_append_bool"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree list_append_bool_1 = Data_Tree("list");
		list_append_bool_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["list_append_bool"]["1"] = list_append_bool_1;
		Function_Arg_DataTypes["list_append_bool"]["2"] = Data_Tree("bool");
		
		Function_Arg_Names["list_append_bool"].push_back("0");
		Function_Arg_Names["list_append_bool"].push_back("1");
		Function_Arg_Names["list_append_bool"].push_back("2");
		
		Function_Arg_Types["list_append"]["0"] = "Scope_Struct";
		Function_Arg_Types["list_append"]["1"] = "list";
		Function_Arg_Types["list_append"]["2"] = "void";
		Function_Arg_Types["list_append"]["3"] = "str";
		
		Function_Arg_DataTypes["list_append"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree list_append_1 = Data_Tree("list");
		list_append_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["list_append"]["1"] = list_append_1;
		Function_Arg_DataTypes["list_append"]["2"] = Data_Tree("void");
		Function_Arg_DataTypes["list_append"]["3"] = Data_Tree("str");
		
		Function_Arg_Names["list_append"].push_back("0");
		Function_Arg_Names["list_append"].push_back("1");
		Function_Arg_Names["list_append"].push_back("2");
		Function_Arg_Names["list_append"].push_back("3");
		
		Function_Arg_Types["list_print"]["0"] = "Scope_Struct";
		Function_Arg_Types["list_print"]["1"] = "list";
		
		Function_Arg_DataTypes["list_print"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree list_print_1 = Data_Tree("list");
		list_print_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["list_print"]["1"] = list_print_1;
		
		Function_Arg_Names["list_print"].push_back("0");
		Function_Arg_Names["list_print"].push_back("1");
		
		Function_Arg_Types["tuple_print"]["0"] = "Scope_Struct";
		Function_Arg_Types["tuple_print"]["1"] = "list";
		
		Function_Arg_DataTypes["tuple_print"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree tuple_print_1 = Data_Tree("list");
		tuple_print_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["tuple_print"]["1"] = tuple_print_1;
		
		Function_Arg_Names["tuple_print"].push_back("0");
		Function_Arg_Names["tuple_print"].push_back("1");
		
		Function_Arg_Types["list_Create"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["list_Create"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["list_Create"].push_back("0");
		
		Function_Arg_Types["list_shuffle"]["0"] = "Scope_Struct";
		Function_Arg_Types["list_shuffle"]["1"] = "list";
		
		Function_Arg_DataTypes["list_shuffle"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree list_shuffle_1 = Data_Tree("list");
		list_shuffle_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["list_shuffle"]["1"] = list_shuffle_1;
		
		Function_Arg_Names["list_shuffle"].push_back("0");
		Function_Arg_Names["list_shuffle"].push_back("1");
		
		Function_Arg_Types["list_size"]["0"] = "Scope_Struct";
		Function_Arg_Types["list_size"]["1"] = "list";
		
		Function_Arg_DataTypes["list_size"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree list_size_1 = Data_Tree("list");
		list_size_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["list_size"]["1"] = list_size_1;
		
		Function_Arg_Names["list_size"].push_back("0");
		Function_Arg_Names["list_size"].push_back("1");
		
		Function_Arg_Types["to_int"]["0"] = "Scope_Struct";
		Function_Arg_Types["to_int"]["1"] = "void";
		
		Function_Arg_DataTypes["to_int"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["to_int"]["1"] = Data_Tree("void");
		
		Function_Arg_Names["to_int"].push_back("0");
		Function_Arg_Names["to_int"].push_back("1");
		
		Function_Arg_Types["to_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["to_float"]["1"] = "void";
		
		Function_Arg_DataTypes["to_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["to_float"]["1"] = Data_Tree("void");
		
		Function_Arg_Names["to_float"].push_back("0");
		Function_Arg_Names["to_float"].push_back("1");
		
		Function_Arg_Types["to_bool"]["0"] = "Scope_Struct";
		Function_Arg_Types["to_bool"]["1"] = "void";
		
		Function_Arg_DataTypes["to_bool"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["to_bool"]["1"] = Data_Tree("void");
		
		Function_Arg_Names["to_bool"].push_back("0");
		Function_Arg_Names["to_bool"].push_back("1");
		
		Function_Arg_Types["zip"]["0"] = "Scope_Struct";
		Function_Arg_Types["zip"]["1"] = "list";
		Function_Arg_Types["zip"]["2"] = "list";
		Function_Arg_Types["zip"]["3"] = "list";
		Function_Arg_Types["zip"]["4"] = "list";
		Function_Arg_Types["zip"]["5"] = "list";
		Function_Arg_Types["zip"]["6"] = "list";
		Function_Arg_Types["zip"]["7"] = "list";
		Function_Arg_Types["zip"]["8"] = "list";
		Function_Arg_Types["zip"]["9"] = "list";
		Function_Arg_Types["zip"]["10"] = "list";
		Function_Arg_Types["zip"]["11"] = "list";
		
		Function_Arg_DataTypes["zip"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree zip_1 = Data_Tree("list");
		zip_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["1"] = zip_1;
		Data_Tree zip_2 = Data_Tree("list");
		zip_2.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["2"] = zip_2;
		Data_Tree zip_3 = Data_Tree("list");
		zip_3.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["3"] = zip_3;
		Data_Tree zip_4 = Data_Tree("list");
		zip_4.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["4"] = zip_4;
		Data_Tree zip_5 = Data_Tree("list");
		zip_5.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["5"] = zip_5;
		Data_Tree zip_6 = Data_Tree("list");
		zip_6.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["6"] = zip_6;
		Data_Tree zip_7 = Data_Tree("list");
		zip_7.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["7"] = zip_7;
		Data_Tree zip_8 = Data_Tree("list");
		zip_8.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["8"] = zip_8;
		Data_Tree zip_9 = Data_Tree("list");
		zip_9.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["9"] = zip_9;
		Data_Tree zip_10 = Data_Tree("list");
		zip_10.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["10"] = zip_10;
		Data_Tree zip_11 = Data_Tree("list");
		zip_11.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["zip"]["11"] = zip_11;
		
		Function_Arg_Names["zip"].push_back("0");
		Function_Arg_Names["zip"].push_back("1");
		Function_Arg_Names["zip"].push_back("2");
		Function_Arg_Names["zip"].push_back("3");
		Function_Arg_Names["zip"].push_back("4");
		Function_Arg_Names["zip"].push_back("5");
		Function_Arg_Names["zip"].push_back("6");
		Function_Arg_Names["zip"].push_back("7");
		Function_Arg_Names["zip"].push_back("8");
		Function_Arg_Names["zip"].push_back("9");
		Function_Arg_Names["zip"].push_back("10");
		Function_Arg_Names["zip"].push_back("11");
		
		Function_Arg_Types["list_Idx"]["0"] = "Scope_Struct";
		Function_Arg_Types["list_Idx"]["1"] = "list";
		Function_Arg_Types["list_Idx"]["2"] = "int";
		
		Function_Arg_DataTypes["list_Idx"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree list_Idx_1 = Data_Tree("list");
		list_Idx_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["list_Idx"]["1"] = list_Idx_1;
		Function_Arg_DataTypes["list_Idx"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["list_Idx"].push_back("0");
		Function_Arg_Names["list_Idx"].push_back("1");
		Function_Arg_Names["list_Idx"].push_back("2");
		
		Function_Arg_Types["tuple_Idx"]["0"] = "Scope_Struct";
		Function_Arg_Types["tuple_Idx"]["1"] = "list";
		Function_Arg_Types["tuple_Idx"]["2"] = "int";
		
		Function_Arg_DataTypes["tuple_Idx"]["0"] = Data_Tree("Scope_Struct");
		Data_Tree tuple_Idx_1 = Data_Tree("list");
		tuple_Idx_1.Nested_Data.push_back(Data_Tree("any"));
		Function_Arg_DataTypes["tuple_Idx"]["1"] = tuple_Idx_1;
		Function_Arg_DataTypes["tuple_Idx"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["tuple_Idx"].push_back("0");
		Function_Arg_Names["tuple_Idx"].push_back("1");
		Function_Arg_Names["tuple_Idx"].push_back("2");
	
		
		Function_Arg_Types["charv_print"]["0"] = "Scope_Struct";
		Function_Arg_Types["charv_print"]["1"] = "str";
		Function_Arg_Types["charv_print"]["2"] = "int";
		
		Function_Arg_DataTypes["charv_print"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["charv_print"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["charv_print"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["charv_print"].push_back("0");
		Function_Arg_Names["charv_print"].push_back("1");
		Function_Arg_Names["charv_print"].push_back("2");
	
		
		Function_Arg_Types["randint"]["0"] = "Scope_Struct";
		Function_Arg_Types["randint"]["1"] = "int";
		Function_Arg_Types["randint"]["2"] = "int";
		
		Function_Arg_DataTypes["randint"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["randint"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["randint"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["randint"].push_back("0");
		Function_Arg_Names["randint"].push_back("1");
		Function_Arg_Names["randint"].push_back("2");
	
		
		Function_Arg_Types["str_Copy"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_Copy"]["1"] = "str";
		
		Function_Arg_DataTypes["str_Copy"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_Copy"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["str_Copy"].push_back("0");
		Function_Arg_Names["str_Copy"].push_back("1");
		
		Function_Arg_Types["str_eq"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_eq"]["1"] = "str";
		Function_Arg_Types["str_eq"]["2"] = "str";
		Function_Arg_Types["str_eq"]["3"] = "int";
		Function_Arg_Types["str_eq"]["4"] = "int";
		
		Function_Arg_DataTypes["str_eq"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_eq"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["str_eq"]["2"] = Data_Tree("str");
		Function_Arg_DataTypes["str_eq"]["3"] = Data_Tree("int");
		Function_Arg_DataTypes["str_eq"]["4"] = Data_Tree("int");
		
		Function_Arg_Names["str_eq"].push_back("0");
		Function_Arg_Names["str_eq"].push_back("1");
		Function_Arg_Names["str_eq"].push_back("2");
		Function_Arg_Names["str_eq"].push_back("3");
		Function_Arg_Names["str_eq"].push_back("4");
		
		Function_Arg_Types["str_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_float"]["1"] = "str";
		
		Function_Arg_DataTypes["str_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_float"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["str_float"].push_back("0");
		Function_Arg_Names["str_float"].push_back("1");
		
		Function_Arg_Types["str_int_add"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_int_add"]["1"] = "str";
		Function_Arg_Types["str_int_add"]["2"] = "int";
		
		Function_Arg_DataTypes["str_int_add"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_int_add"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["str_int_add"]["2"] = Data_Tree("int");
		
		Function_Arg_Names["str_int_add"].push_back("0");
		Function_Arg_Names["str_int_add"].push_back("1");
		Function_Arg_Names["str_int_add"].push_back("2");
		
		Function_Arg_Types["str_float_add"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_float_add"]["1"] = "str";
		Function_Arg_Types["str_float_add"]["2"] = "float";
		
		Function_Arg_DataTypes["str_float_add"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_float_add"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["str_float_add"]["2"] = Data_Tree("float");
		
		Function_Arg_Names["str_float_add"].push_back("0");
		Function_Arg_Names["str_float_add"].push_back("1");
		Function_Arg_Names["str_float_add"].push_back("2");
		
		Function_Arg_Types["int_str_add"]["0"] = "Scope_Struct";
		Function_Arg_Types["int_str_add"]["1"] = "int";
		Function_Arg_Types["int_str_add"]["2"] = "str";
		
		Function_Arg_DataTypes["int_str_add"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["int_str_add"]["1"] = Data_Tree("int");
		Function_Arg_DataTypes["int_str_add"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["int_str_add"].push_back("0");
		Function_Arg_Names["int_str_add"].push_back("1");
		Function_Arg_Names["int_str_add"].push_back("2");
		
		Function_Arg_Types["float_str_add"]["0"] = "Scope_Struct";
		Function_Arg_Types["float_str_add"]["1"] = "float";
		Function_Arg_Types["float_str_add"]["2"] = "str";
		
		Function_Arg_DataTypes["float_str_add"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["float_str_add"]["1"] = Data_Tree("float");
		Function_Arg_DataTypes["float_str_add"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["float_str_add"].push_back("0");
		Function_Arg_Names["float_str_add"].push_back("1");
		Function_Arg_Names["float_str_add"].push_back("2");
		
		Function_Arg_Types["str_bool_add"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_bool_add"]["1"] = "str";
		Function_Arg_Types["str_bool_add"]["2"] = "bool";
		
		Function_Arg_DataTypes["str_bool_add"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_bool_add"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["str_bool_add"]["2"] = Data_Tree("bool");
		
		Function_Arg_Names["str_bool_add"].push_back("0");
		Function_Arg_Names["str_bool_add"].push_back("1");
		Function_Arg_Names["str_bool_add"].push_back("2");
		
		Function_Arg_Types["bool_str_add"]["0"] = "Scope_Struct";
		Function_Arg_Types["bool_str_add"]["1"] = "bool";
		Function_Arg_Types["bool_str_add"]["2"] = "str";
		
		Function_Arg_DataTypes["bool_str_add"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["bool_str_add"]["1"] = Data_Tree("bool");
		Function_Arg_DataTypes["bool_str_add"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["bool_str_add"].push_back("0");
		Function_Arg_Names["bool_str_add"].push_back("1");
		Function_Arg_Names["bool_str_add"].push_back("2");
		
		Function_Arg_Types["str_split_idx"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_split_idx"]["1"] = "str";
		Function_Arg_Types["str_split_idx"]["2"] = "str";
		Function_Arg_Types["str_split_idx"]["3"] = "int";
		
		Function_Arg_DataTypes["str_split_idx"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_split_idx"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["str_split_idx"]["2"] = Data_Tree("str");
		Function_Arg_DataTypes["str_split_idx"]["3"] = Data_Tree("int");
		
		Function_Arg_Names["str_split_idx"].push_back("0");
		Function_Arg_Names["str_split_idx"].push_back("1");
		Function_Arg_Names["str_split_idx"].push_back("2");
		Function_Arg_Names["str_split_idx"].push_back("3");
		
		Function_Arg_Types["can_convert_to_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["can_convert_to_float"]["1"] = "str";
		
		Function_Arg_DataTypes["can_convert_to_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["can_convert_to_float"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["can_convert_to_float"].push_back("0");
		Function_Arg_Names["can_convert_to_float"].push_back("1");
		
		Function_Arg_Types["str_to_float"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_to_float"]["1"] = "str";
		
		Function_Arg_DataTypes["str_to_float"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_to_float"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["str_to_float"].push_back("0");
		Function_Arg_Names["str_to_float"].push_back("1");
		
		Function_Arg_Types["str_str_different"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_str_different"]["1"] = "str";
		Function_Arg_Types["str_str_different"]["2"] = "str";
		
		Function_Arg_DataTypes["str_str_different"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_str_different"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["str_str_different"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["str_str_different"].push_back("0");
		Function_Arg_Names["str_str_different"].push_back("1");
		Function_Arg_Names["str_str_different"].push_back("2");
		
		Function_Arg_Types["str_str_equal"]["0"] = "Scope_Struct";
		Function_Arg_Types["str_str_equal"]["1"] = "str";
		Function_Arg_Types["str_str_equal"]["2"] = "str";
		
		Function_Arg_DataTypes["str_str_equal"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["str_str_equal"]["1"] = Data_Tree("str");
		Function_Arg_DataTypes["str_str_equal"]["2"] = Data_Tree("str");
		
		Function_Arg_Names["str_str_equal"].push_back("0");
		Function_Arg_Names["str_str_equal"].push_back("1");
		Function_Arg_Names["str_str_equal"].push_back("2");
		
		Function_Arg_Types["readline"]["0"] = "Scope_Struct";
		
		Function_Arg_DataTypes["readline"]["0"] = Data_Tree("Scope_Struct");
		
		Function_Arg_Names["readline"].push_back("0");
		
		Function_Arg_Types["_glob_b_"]["0"] = "Scope_Struct";
		Function_Arg_Types["_glob_b_"]["1"] = "str";
		
		Function_Arg_DataTypes["_glob_b_"]["0"] = Data_Tree("Scope_Struct");
		Function_Arg_DataTypes["_glob_b_"]["1"] = Data_Tree("str");
		
		Function_Arg_Names["_glob_b_"].push_back("0");
		Function_Arg_Names["_glob_b_"].push_back("1");

}