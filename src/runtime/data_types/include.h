#pragma once

#include <vector>
#include <map>

#include "any_map.h"
#include "array.h"
#include "bool.h"
#include "charv.h"
#include "codegen_notes.h"
#include "data_tree.h"
#include "float.h"
#include "int.h"
#include "list.h"
#include "map.h"
#include "nsk_vector.h"
#include "nullptr.h"
#include "str.h"
#include "str_view.h"
#include "tensor.h"
#include "type_info.h"


extern std::map<std::string, std::vector<char *>> ClassStrVecs;
extern std::map<std::string, float> NamedClassValues;
extern std::map<std::string, int> NamedInts;
extern std::map<std::string, std::string> NamedObjects;
extern std::map<std::string, std::vector<std::pair<std::string, std::string>>> ScopeVarsToClean;
extern std::map<std::string, char *> ScopeNamesToClean;
extern std::map<int, std::map<std::string, std::vector<std::string>>> ThreadedScopeTensorsToClean;


extern std::map<std::string, std::vector<float>>  FloatVecAuxHash;

extern std::map<std::string, std::string> objectVecs;
extern std::map<std::string, int> objectVecsLastId;

