#pragma once

#include "../include.h"

void OwnedClearComposite(Value *scope_struct, Data_Tree &dt, Value *ptr,
            bool from_obj); 


void Disown(Value *scope_struct, Data_Tree &dt, Value *ptr, bool is_view); 

