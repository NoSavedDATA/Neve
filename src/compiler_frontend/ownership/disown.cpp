#include "disown.h"


void OwnedHolder::RegisterOwned(std::string fn, Data_Tree dt, int memid, Value *v) {
    OwnedToClear[fn].push_back({dt, memid, v});
}



void OwnedClearComposite(Value *scope_struct, Data_Tree &dt, Value *ptr,
            bool from_obj) {
    if (dt.Type=="array") {
        ArrayClearOwned(scope_struct,dt,ptr,from_obj);
        return;
    }
    if (dt.Type=="map") {
        MapClear(scope_struct,ptr,dt,false,from_obj);
        return;
    }
    if (dt.Type=="channel") {
        //
        return;
    }
    

    // p2t("composite: " + dt.Type);
    // call("print_void_ptr",{ptr});

    std::string class_name = dt.Type;
    int attr_idx=-1;
    for (auto &attr : ClassAttrsName[class_name]) {
        attr_idx++;
        Data_Tree attr_dt = data_typeVars[class_name][attr];

        if (!attr_dt.IsTraversable())
            continue;

        // p2t("  traverse: " + attr_dt.Type + ", " + std::to_string(attr_idx));
        // if (attr_dt.Nested_Data.size()>0)
            // p2t("    inner: " + attr_dt.Nested_Data[0].Type);


        Value *attr_ptr = Builder->CreateLoad(int8PtrTy,
                    Builder->CreateStructGEP(
                        struct_types["class_"+class_name],
                        ptr, attr_idx)
                    );
        // p2t("attribute " + dt.Type + " | " + attr_dt.Type);
        // call("print_void_ptr",{attr_ptr});

        // if (dt.Type=="BackNode")
        //     p2t("BACKNODE&&&&&&&&&&&&&&&&&&&&&&&&&");


        // if (dt.Type=="BackNode") {

            // std::cout << "-------------->traverse " << attr_dt.Type << " - " << attr_idx << "\n";
            // p2t("   traverse " + attr_dt.Type + " | " + std::to_string(attr_idx));
            // p2t("   traverse " + attr_dt.Type + " | " + std::to_string(attr_dt.is_view));
            if (!attr_dt.is_view)
                Disown(scope_struct,attr_dt,
                        attr_ptr);
            // std::cout << "\n\n\n\n\n\n";
        // }
    }

}



void Disown(Value *scope_struct, Data_Tree &dt, Value *ptr, bool is_view) {
    if (!is_view&&dt.IsTraversable())
        OwnedClearComposite(scope_struct, dt, ptr, false);

    std::string disown_method = dt.Type+"_disown";
    if (fn_ret_dt.count(disown_method)>0) {
        set_scope_obj(scope_struct, ptr);
        call(disown_method, {scope_struct});
    }
}

void OwnedHolder::ClearOwned(Value *scope_struct, std::string fn,
                             std::string base_fn) {
    if (OwnedToClear.size()==0)
        return;
    // p2t("clear owneds: " + std::to_string(OwnedToClear.size()));
    // std::cout << "\n\n------\n";
    // if (auto *op_stmt = dynamic_cast<BinaryExprAST*>(this)) {
    //     // std::cout << "\n\t\033[32mPASS: " << fn << "\033[0m\n\n";
    //     p2t("BinOp " + fn + ": " + op_stmt->Op +  " CLEAR OWNED ");
    // }
    // if (auto *stmt = dynamic_cast<NameableCall*>(this)) {
    //     std::cout << "CALL CLEAR OWNED " << "\n";
    //     p2t("Call clear owned " + fn + " | " +  stmt->Callee);
    // }
    // if (dynamic_cast<IfExprAST*>(this))
    //     p2t("IF CLEAR OWNED ");
    // if (dynamic_cast<ForExprAST*>(this))
    //     p2t("FOR CLEAR OWNED ");
    // if (dynamic_cast<WhileExprAST*>(this))
    //     p2t("WHILE CLEAR OWNED ");

    Value *previous_obj = get_scope_obj(scope_struct);
    for(auto &[dt, memid, ptr] : OwnedToClear[fn]) {

        OwnedsCleared[fn].push_back(ptr);

        bool is_conditional = ctakens.count(fn)>0&&ctakens[fn].count(memid)>0; 
        BasicBlock *AfterBB, *DisownBB, *created_okBB;
        if (is_conditional) {
            // p2t("------------------------->conditional " + fn + " -- " + std::to_string(memid));
            // call("print_bool", {Builder->CreateLoad(boolTy, ctakens[fn][memid])}); 
            Function *TheFunction = Builder->GetInsertBlock()->getParent();
            AfterBB  = BasicBlock::Create(*TheContext, "disown.after",
                                TheFunction);
            DisownBB = BasicBlock::Create(*TheContext, "disown.disown",
                                TheFunction);


            if (ctakens_maybe[fn].count(memid)>0) {
                created_okBB = BasicBlock::Create(*TheContext, "disown.disown",
                                    TheFunction);
                Value *was_created = Builder->CreateLoad(boolTy, ctakens_maybe[fn][memid]);
                Builder->CreateCondBr(
                            was_created, created_okBB, AfterBB);
                Builder->SetInsertPoint(created_okBB);
            }

            Builder->CreateCondBr(
                        Builder->CreateLoad(boolTy, ctakens[fn][memid]),
                        AfterBB, DisownBB);
            Builder->SetInsertPoint(DisownBB);
        }



        // if (dt.Type=="BackNode") {
        //     p2t("cond free in : " + fn);
        //     // call("print_void_ptr", {ptr});
        // }

        // p2t("Disown " + dt.Type);
        Disown(scope_struct, dt, ptr, in_vec(memid, fn_views[fn]));

        if (is_conditional) {
            call("free", {ptr});
            if (ctakens_maybe[fn].count(memid)>0) {
                Builder->CreateStore(const_bool(false), ctakens_maybe[fn][memid]);
            }
            block_values[DisownBB] = function_values[fn];
            Builder->CreateBr(AfterBB);
            Builder->SetInsertPoint(AfterBB);
        }
    }
    set_scope_obj(scope_struct, previous_obj);
}




