//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.
#include <cstdlib>
#include <cassert>
#include <vector>
#include <unordered_set>

#include "pass_dfg.hpp"
#include "cfg_node_data.hpp"
#include "lgedge.hpp"
#include "lgedgeiter.hpp"


using std::unordered_map;

unsigned int Pass_dfg::temp_counter = 0;

Pass_dfg::Pass_dfg(const std::string &key, const std::string &value) : Pass() { opack.set(key,value); }

LGraph * Pass_dfg::generate_dfg() {
  assert(!opack.src.empty());

  //LGraph *cfg = new LGraph(opack.path, opack.src, false);
  const LGraph *cfg = new LGraph(opack.path, opack.src, false);
  regen(cfg);
  delete cfg;

  LGraph *dfg = new LGraph(opack.path, opack.name, false);
  return dfg;
}

LGraph *Pass_dfg::regen(const LGraph *cfg) {
  assert(!opack.name.empty());
  LGraph *dfg = new LGraph(opack.path, opack.name, false); // true? should it clear the DFG?

  cfg_2_dfg(dfg, cfg);
  dfg->sync();
  delete dfg;
}

void  Pass_dfg::optimize() {
  assert(!opack.src.empty());
  LGraph *dfg = new LGraph(opack.path, opack.src, false);
  trans(dfg);
}

void Pass_dfg::trans(LGraph *dfg) {
  LGraph* sub_graph = nullptr;
  //resolve pending graph
  for(auto idx : dfg->fast()) {
    if(dfg->node_type_get(idx).op == DfgPendingGraph_Op){
      if((sub_graph = LGraph::find_lgraph(dfg->get_path(), ((std::string)(dfg->get_node_wirename(idx))+"_dfg")))){
        dfg->node_subgraph_set(idx, (uint32_t)sub_graph->lg_id());
        fmt::print("resolve pending subgraph! nid:{}, sub_graph name:{}, sub_graph_id:{}\n", idx, dfg->get_node_wirename(idx), sub_graph->lg_id());
      }
      else{
        fmt::print("cannot resolve pending subgraph!!\n");
        assert(0);
      }
    }
  }

  //bits inference
  for(auto idx : dfg->fast()) {
    for(const auto &out : dfg->out_edges(idx)) {
      Index_ID src_nid      = idx;
      Index_ID dst_nid      = out.get_idx();
      Port_ID  src_pid      = out.get_out_pin().get_pid();
      Port_ID  dst_pid      = out.get_inp_pin().get_pid();
      uint16_t src_nid_size = dfg->get_bits(src_nid);
      uint16_t dst_nid_size = dfg->get_bits(dst_nid);

      if(dfg->node_type_get(idx).op == SubGraph_Op){
        LGraph *subgraph = dfg->get_library()->get_graph(dfg->subgraph_id_get(idx));
        const char *out_name = subgraph->get_graph_output_name_from_pid(src_pid);
        uint16_t    out_size = subgraph->get_bits(subgraph->get_graph_output(out_name).get_nid());
        dfg->set_bits(dst_nid,out_size);
      }
      else{
        if(src_nid_size > dst_nid_size)
          dfg->set_bits(dst_nid, src_nid_size);
      }
    }
  }

  dfg->sync();
}


void Pass_dfg::cfg_2_dfg(LGraph *dfg, const LGraph *cfg) {
  Index_ID    itr = find_cfg_root(cfg);
  CF2DF_State state(dfg);

  process_cfg(dfg, cfg, &state, itr);
  attach_outputs(dfg, &state);

  fmt::print("calling sync\n");
}

Index_ID Pass_dfg::process_cfg(LGraph *dfg, const LGraph *cfg, CF2DF_State *state, Index_ID top_node) {
  Index_ID itr = top_node;
  Index_ID last_itr = 0;

  while (itr != 0) {
    last_itr = itr;
    itr = process_node(dfg, cfg, state, itr);
    fmt::print("cfg nid:{} process finished!!\n\n", last_itr);
  }

  state->print_aux();
  return last_itr;
}

Index_ID Pass_dfg::process_node(LGraph *dfg, const LGraph *cfg, CF2DF_State *state, Index_ID node) {
  CFG_Node_Data data(cfg, node);

  //sh dbg
  fmt::print("Processing CFG node:{}\n", node);
  fmt::print("target:[{}], operator:[{}], ", data.get_target(), data.get_operator());
  fmt::print("operands:[");
  for(const auto i:data.get_operands())
    fmt::print("{}, ", i);
  fmt::print("]");
  fmt::print("\n");


  switch (cfg->node_type_get(node).op) {
  case CfgAssign_Op:
    process_assign(dfg, state, data);
    return get_cfg_child(cfg, node);
  case CfgFunctionCall_Op:
    process_func_call(dfg, cfg, state, data);
    return get_cfg_child(cfg, node);
  case CfgIf_Op:
    return process_if(dfg, cfg, state, data, node);
  case CfgIfMerge_Op:
    return 0;
  default:
    fmt::print("\n\n*************Unrecognized node type[n={}]: {}\n", node, cfg->node_type_get(node).get_name());
    return get_cfg_child(cfg, node);
  }
}

void Pass_dfg::process_func_call(LGraph *dfg, const LGraph *cfg, CF2DF_State *state, const CFG_Node_Data &data) {
  //for func_call, all the node should be created before, you just connect them.
  //no need to create node for target
  fmt::print("process_func_call\n");
  const auto &target = data.get_target();
  const auto &oprds  = data.get_operands();
  const auto &oprd_ids = process_operands_bk(dfg, state, data); // all the operands should be created before, just get back oprd_ids
  LGraph* sub_graph = nullptr;
  Index_ID subg_root_nid = state->get_alias(oprds[0]);

  if((sub_graph = LGraph::find_lgraph(cfg->get_path(), ((std::string)(dfg->get_node_wirename(subg_root_nid))+"_dfg")))){
    dfg->node_subgraph_set(subg_root_nid, (uint32_t)sub_graph->lg_id());
    fmt::print("set subgraph on nid:{}, sub_graph name:{}, sub_graph_id:{}\n", subg_root_nid, dfg->get_node_wirename(subg_root_nid), sub_graph->lg_id());
  }else{
    dfg->node_type_set(subg_root_nid, DfgPendingGraph_Op);
    fmt::print("set pending graph on nid:{}, sub_graph name should be:{}\n", subg_root_nid, dfg->get_node_wirename(subg_root_nid));
  }

  state->set_alias(target, oprd_ids[0]);

  //connect 1st operand with [2nd,3rd,...] operands
  std::vector<Index_ID> subg_input_ids(oprd_ids.begin()+1, oprd_ids.end());
  process_connections(dfg, subg_input_ids, subg_root_nid);
}


void Pass_dfg::process_assign(LGraph *dfg, CF2DF_State *state, const CFG_Node_Data &data){
  fmt::print("process_assign\n");
  const auto                     &target = data.get_target();
  const std::vector<std::string> &oprds  = data.get_operands();
  const std::string              &op     = data.get_operator();
  Index_ID                       oprd_id0;
  Index_ID                       oprd_id1;
  if(is_pure_assign_op(op)){
    if(is_output(target)) {
      oprd_id0 = process_operand(dfg, state, oprds[0]);
      auto bits = dfg->get_bits(oprd_id0);
      std::vector<Index_ID> oprd_ids;
      oprd_ids.push_back(oprd_id0);
      Index_ID target_id = create_output(dfg, state, target, bits);
      fmt::print("create node for output target:{}, nid:{}\n", target, target_id);
      process_connections(dfg, oprd_ids, target_id);
    }else{
      oprd_id0 = process_operand(dfg, state, oprds[0]);
      state->set_alias(target, oprd_id0);
    }
  }
  else if(is_label_op(op)){
    if(oprds[0] == "__bits"){
      //Index_ID floating_id = resolve_constant(dfg, state, oprds[1]);
      Index_ID floating_id = process_operand(dfg, state, oprds[1]);
      state->set_alias(target, floating_id);
    }else if(oprds[0] == "__fluid"){
      ;//balabala
    }else{//function argument assign
      oprd_id1 = process_operand(dfg, state, oprds[1]);
      state->set_alias(target, oprd_id1);
    }
  }
  else if(is_as_op(op)){
    oprd_id0 = process_operand(dfg, state, oprds[0]);
    //process target
    if(is_input(target)){
      assert(dfg->node_value_get(oprd_id0));
      auto bits = dfg->node_value_get(oprd_id0);
      Index_ID target_id = create_input(dfg, state, target, bits);
      fmt::print("create node for input target:{}, nid:{}\n", target, target_id);
    }else if(is_output(target)){
      assert(dfg->node_value_get(oprd_id0));
      auto bits = dfg->node_value_get(oprd_id0);
      Index_ID target_id = create_output(dfg, state, target, bits);
      fmt::print("create node for output target:{}, nid:{}\n", target, target_id);
    }else
      state->set_alias(target, oprd_id0);
  }
  else if(is_unary_op(op)){
    oprd_id0 = process_operand(dfg, state, oprds[0]);
    state->set_alias(target, oprd_id0);
  }
  else if(is_binary_op(op)){
    std::vector<Index_ID> oprd_ids;
    oprd_ids.push_back(process_operand(dfg, state, oprds[0]));
    oprd_ids.push_back(process_operand(dfg, state, oprds[1]));
    Index_ID target_id = create_node(dfg, state, target);
    fmt::print("create node for internal target:{}, nid:{}\n", target, target_id);
    dfg->node_type_set(target_id, node_type_from_text(op));
    auto max_bits = std::max(dfg->get_bits(oprd_ids[0]),dfg->get_bits(oprd_ids[1])) + 1;
    dfg->set_bits(target_id,max_bits);
    process_connections(dfg, oprd_ids, target_id);
  }
}

void Pass_dfg::process_connections(LGraph *dfg, const std::vector<Index_ID> &src_nids, const Index_ID &dst_nid){
  for (unsigned i = 0; i<src_nids.size();i++){
    Index_ID src_nid =  src_nids.at(i);
    fmt::print("src_nid:{}\n", src_nid);
    fmt::print("src_nids size:{}\n", src_nids.size());
    //Port_ID  src_pid = (dfg->node_type_get(src_nid).op == Or_Op) ? (uint16_t)1 : (uint16_t)0; // output pid=1 for reduced Or_Op
    Port_ID  src_pid = 0;
    Port_ID  dst_pid = (dfg->node_type_get(dst_nid).op == Sum_Op            )? (uint16_t)1 :
                       (dfg->node_type_get(dst_nid).op == DfgPendingGraph_Op)? (uint16_t)i :
                       (dfg->node_type_get(dst_nid).op == SubGraph_Op       )? (uint16_t)i : (uint16_t)0;

    dfg->add_edge(Node_Pin(src_nid, src_pid, false), Node_Pin(dst_nid, dst_pid, true));
    //fmt::print("create edge ({}->{})\n", dfg->get_node_wirename(src_nid), dfg->get_node_wirename(dst_nid));
  }
}


Index_ID Pass_dfg::process_operand(LGraph *dfg, CF2DF_State *state, const std::string oprd) {
  Index_ID oprd_id;
  if (state->has_alias(oprd)){
    oprd_id = state->get_alias(oprd);
    fmt::print("operand:{} has an alias:{}\n", oprd, oprd_id);
  }
  else {
    if (is_constant(oprd)){
      oprd_id = resolve_constant(dfg, state, oprd);
      state->set_alias(oprd, oprd_id);
      fmt::print("create node for constant operand:{}, nid:{}\n", oprd, oprd_id);
    }
    else if (is_register(oprd)){
      oprd_id = create_register(dfg, state, oprd);
      state->set_alias(oprd, oprd_id);
      fmt::print("create node for register operand:{}, nid:{}\n", oprd, oprd_id);
    }
    else if (is_input(oprd)){
      oprd_id = create_input(dfg, state, oprd);
      state->set_alias(oprd, oprd_id);
      fmt::print("create node for input operand:{}, nid:{}\n", oprd, oprd_id);
    }
    else if (is_output(oprd)){
      oprd_id = create_output(dfg, state, oprd);
      state->set_alias(oprd, oprd_id);
      fmt::print("create node for output operand:{}, nid:{}\n", oprd, oprd_id);
    }
    else if (is_reference(oprd)){
      oprd_id = create_reference(dfg, state, oprd);
      state->set_alias(oprd, oprd_id);
      fmt::print("create node for reference operand:{}, nid:{}\n", oprd, oprd_id);
    }
    else{
      oprd_id = create_private(dfg, state, oprd);
      state->set_alias(oprd, oprd_id);
      fmt::print("create node for private operand:{}, nid:{}\n", oprd, oprd_id);
    }
  }

  if (state->fluid_df() && is_input(oprd))
    add_read_marker(dfg, state, oprd);
  return oprd_id;
}


Index_ID Pass_dfg::process_if(LGraph *dfg, const LGraph *cfg, CF2DF_State *state, const CFG_Node_Data &data, Index_ID node) {
  fmt::print("process_if\n");
  Index_ID cond = state->get_alias(data.get_target());
  const auto &operands = data.get_operands();

  Index_ID tbranch = std::stol(operands[0]);
  CF2DF_State tstate = state->copy();
  Index_ID tb_next = get_cfg_child(cfg, process_cfg(dfg, cfg, &tstate, tbranch));

  Index_ID fbranch = std::stol(operands[1]);

  if (fbranch != node) {                                // there is an 'else' clause
    CF2DF_State fstate = state->copy();
    Index_ID fb_next = get_cfg_child(cfg, process_cfg(dfg, cfg, &fstate, fbranch));
    assert(tb_next == fb_next);
    add_phis(dfg, cfg, state, &tstate, &fstate, cond);
  } else {
    add_phis(dfg, cfg, state, &tstate, state, cond);    // if there's no else, the 'state' of the 'else' branch is the same as the parent
  }

  return tb_next;
}

void Pass_dfg::add_phis(LGraph *dfg, const LGraph *cfg, CF2DF_State *parent, CF2DF_State *tstate, CF2DF_State *fstate, Index_ID condition) {
  std::unordered_set<std::string> phis_added;

  for (const auto &pair : tstate->get_auxtab()) {
    if (reference_changed(parent, tstate, pair.first)) {
      add_phi(dfg, parent, tstate, fstate, condition, pair.first);
      phis_added.insert(pair.first);
    }
  }

  for (const auto &pair : fstate->get_auxtab()) {
    if (phis_added.find(pair.first) == phis_added.end() && reference_changed(parent, fstate, pair.first))
      add_phi(dfg, parent, tstate, fstate, condition, pair.first);
  }
}

void Pass_dfg::add_phi(LGraph *dfg, CF2DF_State *parent, CF2DF_State *tstate, CF2DF_State *fstate, Index_ID condition, const std::string &variable) {
  Index_ID tid = resolve_phi_branch(dfg, parent, tstate, variable);
  Index_ID fid = resolve_phi_branch(dfg, parent, fstate, variable);

  Index_ID phi = dfg->create_node().get_nid();
  dfg->node_type_set(phi, Mux_Op);
  auto tp = dfg->node_type_get(phi);

  Port_ID tin = tp.get_input_match("B");
  Port_ID fin = tp.get_input_match("A");
  Port_ID cin = tp.get_input_match("S");

  dfg->add_edge(Node_Pin(tid, 0, false), Node_Pin(phi, tin, true));
  dfg->add_edge(Node_Pin(fid, 0, false), Node_Pin(phi, fin, true));
  dfg->add_edge(Node_Pin(condition, 0, false), Node_Pin(phi, cin, true));

  parent->set_alias(variable, phi);
}

Index_ID Pass_dfg::resolve_phi_branch(LGraph *dfg, CF2DF_State *parent, CF2DF_State *branch, const std::string &variable) {
  if (branch->has_alias(variable))
    return branch->get_alias(variable);
  else if (parent->has_alias(variable))
    return parent->get_alias(variable);
  else if (is_register(variable))
    return create_register(dfg, parent, variable);
  else if (is_input(variable))
    return create_input(dfg, parent, variable);
  else if (is_output(variable))
    return create_output(dfg, parent, variable);
  else
    return create_default_const(dfg, parent);
}


void Pass_dfg::assign_to_true(LGraph *dfg, CF2DF_State *state, const std::string &v) {
  Index_ID node = create_node(dfg, state, v);
  fmt::print("create node nid:{}\n", node);
  dfg->node_type_set(node, Or_Op);

  dfg->add_edge(Node_Pin(create_true_const(dfg, state), 0, false), Node_Pin(node, 0, true));
  dfg->add_edge(Node_Pin(create_true_const(dfg, state), 0, false), Node_Pin(node, 0, true));
}

void Pass_dfg::attach_outputs(LGraph *dfg, CF2DF_State *state) {
  for (const auto &pair : state->copy().get_auxtab()) {
    const auto &var = pair.first;

    if (is_register(var) || is_output(var)) {
      Index_ID lref = pair.second;

      Index_ID oid = create_output(dfg, state, var);
      dfg->add_edge(Node_Pin(lref, 0, false), Node_Pin(oid, 0, true));
    }
  }

  if (state->fluid_df())
    add_fluid_behavior(dfg, state);
}

void Pass_dfg::add_fluid_behavior(LGraph *dfg, CF2DF_State *state) {
  std::vector<Index_ID> inputs, outputs;
  add_fluid_ports(dfg, state, inputs, outputs);

}

void Pass_dfg::add_fluid_ports(LGraph *dfg, CF2DF_State *state, std::vector<Index_ID> &data_inputs, std::vector<Index_ID> &data_outputs) {
  for (const auto &pair : state->outputs()) {
    if (!is_valid_marker(pair.first) && !is_retry_marker(pair.first)) {
      auto valid_output = valid_marker(pair.first);
      auto retry_input = retry_marker(pair.first);
      data_outputs.push_back(state->get_alias(pair.first));

      if (!state->has_alias(valid_output))
        create_output(dfg, state, valid_output);

      if (!state->has_alias(retry_input))
        create_input(dfg, state, retry_input);
    }
  }

  for (const auto &pair : state->inputs()) {
    if (!is_valid_marker(pair.first) && !is_retry_marker(pair.first)) {
      auto valid_input = valid_marker(pair.first);
      auto retry_output = retry_marker(pair.first);
      data_inputs.push_back(state->get_alias(pair.first));

      if (!state->has_alias(valid_input))
        create_input(dfg, state, valid_input);

      if (!state->has_alias(retry_output))
        create_output(dfg, state, retry_output);
    }
  }
}

void Pass_dfg::add_fluid_logic(LGraph *dfg, CF2DF_State *state, const std::vector<Index_ID> &data_inputs, const std::vector<Index_ID> &data_outputs) {
  //Index_ID abort_id = add_abort_logic(dfg, state, data_inputs, data_outputs);


}

void Pass_dfg::add_abort_logic(LGraph *dfg, CF2DF_State *state, const std::vector<Index_ID> &data_inputs, const std::vector<Index_ID> &data_outputs) {

}

Index_ID Pass_dfg::find_cfg_root(const LGraph *cfg) {
  for(auto idx : cfg->fast()) {
    if(cfg->is_root(idx))
      return idx;
  }

  assert(false);
}

Index_ID Pass_dfg::get_cfg_child(const LGraph *cfg, Index_ID node) {
  std::vector<Index_ID> children;

  for(const auto &cedge : cfg->out_edges(node))
    children.push_back(cedge.get_inp_pin().get_nid());

  if(children.size() == 1)
    return children[0];
  else if(children.size() == 0)
    return 0;
  else
    assert(false);
}

std::vector<Index_ID> Pass_dfg::process_operands_bk(LGraph *dfg, CF2DF_State *state, const CFG_Node_Data &data) {
  const std::vector<std::string> &oprds = data.get_operands();
  std::vector<Index_ID> oprd_ids(oprds.size());
  const std::string &op = data.get_operator();

  for (size_t i = 0 ; i <oprd_ids.size();i++) {
    if (state->has_alias(oprds[i])){
      oprd_ids[i] = state->get_alias(oprds[i]);
      fmt::print("operand:{} has an alias:{}\n", oprds[i], oprd_ids[i]);
    }
    else {
      if (is_constant(oprds[i])){
        //oprd_ids[i] = create_default_const(dfg, state);
        oprd_ids[i] = resolve_constant(dfg, state, oprds[i]);
        fmt::print("create node for constant operand:{}, nid:{}\n", oprds[i], oprd_ids[i]);
      }
      else if (is_register(oprds[i])){
        oprd_ids[i] = create_register(dfg, state, oprds[i]);
        fmt::print("create node for register operand:{}, nid:{}\n", oprds[i], oprd_ids[i]);
      }
      else if (is_input(oprds[i])){
        oprd_ids[i] = create_input(dfg, state, oprds[i]);
        fmt::print("create node for input operand:{}, nid:{}\n", oprds[i], oprd_ids[i]);
      }
      else if (is_output(oprds[i])){
        oprd_ids[i] = create_output(dfg, state, oprds[i]);
        fmt::print("create node for output operand:{}, nid:{}\n", oprds[i], oprd_ids[i]);
      }
      else if (is_reference(oprds[i])){
        oprd_ids[i] = create_reference(dfg, state, oprds[i]);
        fmt::print("create node for reference operand:{}, nid:{}\n", oprds[i], oprd_ids[i]);
      }
      else{
        oprd_ids[i] = create_private(dfg, state, oprds[i]);
        fmt::print("create node for private operand:{}, nid:{}\n", oprds[i], oprd_ids[i]);
      }
    }

    if (state->fluid_df() && is_input(oprds[i]))
      add_read_marker(dfg, state, oprds[i]);
  }

  return oprd_ids;
}





void Pass_dfg_options::set(const std::string &key, const std::string &value) {
  try {
    if (is_opt(key,"src") )
      src = value;
    else
      set_val(key,value);

  } catch (const std::invalid_argument& ia) {
    fmt::print("ERROR: key {} has an invalid argument {}\n",key);
  }

  console->warn("pass_dfg src:{} path:{} name:{}", src, path, name);
}

