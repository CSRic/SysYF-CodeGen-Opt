#include "DeadCodeEli.h"
#include "RDominateTree.h"
#define DEBUG std::cout << "qwq" << std::endl;
BasicBlock* get_irdom(BasicBlock* bb){
    BasicBlock *irdom = nullptr;
    for(auto pdombb:bb->get_rdoms()){
        if(pdombb==bb)
            continue;
        auto curbb = pdombb;
        if(irdom==nullptr){
            irdom = curbb;
        }
        if (curbb->get_rdoms().find(irdom) == curbb->get_rdoms().end()){
            irdom = curbb;
        }
    }
    return irdom;
}

bool DeadCodeEli::is_valid_expr(Instruction* inst){
    return !(inst->is_void());
}

bool DeadCodeEli::is_critical_expr(Instruction* inst){
    return (inst->is_call() || inst->is_ret() || inst->is_store());
}

void DeadCodeEli::execute() {
    // Start from here!
    RDominateTree r_dom_tree(module); // 名字可以随便起
    r_dom_tree.execute();
    
    for (auto fun : module->get_functions())
    {   
        if(fun->get_num_basic_blocks()==0){
            continue;
        }
        initialize_inst_mark(fun);
        mark(fun);
        sweep(fun);
    }
    // std::list<Instruction *> delete_list = {};
    // for(auto fun:module->get_functions()){
    //     //找到所有引用列表为空的指令，插入待删除的列表中
    //     for(auto bb:fun->get_basic_blocks()){
    //         for(auto inst:bb->get_instructions()){
    //             if(inst->get_use_list().empty()){
    //                 if(!is_valid_expr(inst))
    //                     continue;
    //                 delete_list.push_back(inst);
    //             }
    //         }
    //     }
    //     for(auto inst:delete_list){
    //         clean_inst(inst);
    //     }
    // }
    
}

void DeadCodeEli::initialize_inst_mark(Function* fun){
    inst_mark.clear();
    for (auto bb : fun->get_basic_blocks())
    {
        // std::cout << bb->get_name() << " " << bb->get_rdom_frontier().size() << " " << bb->get_rdoms().size() << std::endl;
        // for(auto qwq:bb->get_rdoms()){
        //     std::cout << qwq->get_name()<<" ";
        // }
        // std::cout << std::endl;
        for (auto inst : bb->get_instructions())
        {
            inst_mark.insert({inst, false});
        }
    }
}


void DeadCodeEli::mark(Function* fun){
    std::list<Instruction *> worklist = {};
    for(auto bb:fun->get_basic_blocks()){
        for(auto inst:bb->get_instructions()){
            if(is_critical_expr(inst)){
                inst_mark.find(inst)->second = true;
                worklist.push_back(inst);
            }
        }
        if(bb->get_rdoms().size()==1){
            worklist.push_back(bb->get_instructions().back());
            inst_mark[bb->get_instructions().back()] = true;
        }
    }
    while(!worklist.empty()){
        auto inst = worklist.front();
        worklist.pop_front();
        for(auto operand:inst->get_operands()){
            if(dynamic_cast<Instruction *>(operand)){
                auto curinst = dynamic_cast<Instruction *>(operand);
                if(!inst_mark[curinst]){
                    inst_mark[curinst] = true;
                    worklist.push_back(curinst);
                }
            }
        }
        for(auto bb:inst->get_parent()->get_rdom_frontier()){
            auto br = bb->get_instructions().back();
            if(br->is_br()){
                if(!inst_mark[br]){
                    inst_mark[br] = true;
                    worklist.push_back(br);
                }
            }
        }
    }
}


void DeadCodeEli::sweep(Function* fun){
    std::list<Instruction *> delete_list = {};
    for(auto bb: fun->get_basic_blocks()){
        for(auto inst:bb->get_instructions()){
            if(!inst_mark[inst]){
                delete_list.push_back(inst);
            }
        }
    }
    for(auto inst:delete_list){
        if (is_valid_expr(inst))
        {
            inst->get_parent()->delete_instr(inst);//删除指令
            for(auto operand:inst->get_operands()){//更新使用者列表
                if(dynamic_cast<ConstantInt *>(operand)){
                    continue;//常数不用管
                }
                if(dynamic_cast<Instruction*>(operand)){
                    auto curinst = dynamic_cast<Instruction *>(operand);//操作数对应的指令
                    curinst->remove_use(dynamic_cast<Value *>(inst));
                }
            }
        }
        else if(inst->is_br()){
            auto brinst = dynamic_cast<BranchInst *>(inst);
            // if(!brinst->is_cond_br()){
            //     continue;
            // }
            Instruction *nearest_marked_postdom = inst;
            while(!inst_mark[nearest_marked_postdom]){
                //std::cout << nearest_marked_postdom->get_parent()->get_name() << std::endl;
                nearest_marked_postdom = get_irdom(nearest_marked_postdom->get_parent())->get_instructions().back();
                
            }
            auto newbr = BranchInst::create_br(nearest_marked_postdom->get_parent(), inst->get_parent());
            inst->get_parent()->delete_instr(inst);
            DEBUG
        }
    }
}


void DeadCodeEli::find_nearest_marked_pdombb(BasicBlock*bb){
    
}


void DeadCodeEli::clean_inst(Instruction* inst){
    //将指令inst从其操作数的use列表中移除并删除inst
    for(auto operand:inst->get_operands()){
        if(dynamic_cast<ConstantInt *>(operand)){
            continue;//常数不用管
        }
        if(dynamic_cast<Instruction*>(operand)){
            auto curinst = dynamic_cast<Instruction *>(operand);//操作数对应的指令
            curinst->remove_use(dynamic_cast<Value *>(inst));
            if(curinst->get_use_list().empty()){
                clean_inst(curinst);
            }
        }
    }
    inst->get_parent()->delete_instr(inst);
}