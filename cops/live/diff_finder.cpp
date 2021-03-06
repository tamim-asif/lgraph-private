//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "diff_finder.hpp"
#include "lgedgeiter.hpp"

#include "graph_library.hpp"
#include "live_common.hpp"
#include "stdlib.h"

using namespace Live;

auto Diff_finder::go_up(const Graph_Node &boundary) {

  Graph_Node bound;
  bound.module             = nullptr;
  std::string current_inst = boundary.instance;

  for(auto &parentID : boundaries->hierarchy_tree[Invariant_boundaries::get_graphID(boundary.module)]) {
    LGraph *parent = Invariant_boundaries::get_graph(parentID, boundary.module->get_path());
    if(!parent) {
      parent = Invariant_boundaries::get_graph(parentID, original->get_path());
    }

    assert(parent);

    for(auto &parent_instance : boundaries->instance_collection[parentID]) {
      //FIXME: can we avoid the string comparison here?
      if(current_inst.substr(0, parent_instance.size()) != parent_instance)
        continue;

      std::string sub_instance;
      if(parent_instance != "")
        sub_instance = current_inst.substr(parent_instance.size() + hier_sep.size());
      else
        sub_instance = current_inst;
#ifdef DEBUG
      assert(parent->has_instance_name(sub_instance.c_str()));
#endif

      Index_ID parent_node = parent->get_node_from_instance_name(sub_instance.c_str());

      assert(bound.module == nullptr);
      bound.module   = parent;
      bound.idx      = parent_node;
      bound.bit      = boundary.bit;
      bound.instance = parent_instance;
      bound.pid      = boundary.module->get_graph_pid_from_nid(boundary.idx);
      assert(boundary.module->is_graph_input(boundary.idx) || boundary.module->is_graph_output(boundary.idx));
    }
  }
  assert(bound.module != nullptr);
  return bound;
}

auto Diff_finder::go_down(const Graph_Node &boundary, bool output) {
  Graph_Node bound;

  LGraph * current = boundary.module;
  Index_ID idx     = boundary.idx;
  Port_ID  pid     = boundary.pid;

  std::string subgraph_name = current->get_library()->get_name(current->subgraph_id_get(idx));
  LGraph *    child         = LGraph::find_graph(subgraph_name, current->get_path());

  if(!child) {
    child = LGraph::find_graph(subgraph_name, original->get_path());
  }

  assert(child);

  bound.module = child;
  if(boundary.instance != "")
    bound.instance = boundary.instance + hier_sep + current->get_node_instancename(boundary.idx);
  else
    bound.instance = current->get_node_instancename(boundary.idx);

  Index_ID nid = 0;
  if(output)
    nid = child->get_graph_output_nid_from_pid(pid);
  else
    nid = child->get_graph_input_nid_from_pid(pid);

  assert(nid); // pid provided not valid
  bound.idx = nid;
  bound.bit = boundary.bit;
  bound.pid = 0; //IO pids always 0

  return bound;
}

void Diff_finder::find_fwd_boundaries(Graph_Node &          start_boundary,
                                      std::set<Graph_Node> &discovered, bool went_up) {

  if(fwd_visited.find(start_boundary) != fwd_visited.end())
    return;

  fwd_visited.insert(start_boundary);

  fmt::print("fwd bound {} {} {} {} {}\n", start_boundary.module->get_name(), start_boundary.idx, start_boundary.pid, start_boundary.bit, start_boundary.instance);

  stack.insert(start_boundary);

  Index_ID idx     = start_boundary.idx;
  LGraph * current = start_boundary.module;
  Port_ID  pid     = start_boundary.pid;

  if(current->node_type_get(idx).op == SubGraph_Op && !went_up) {
    Graph_Node child = go_down(start_boundary, false);

    if(stack.find(child) == stack.end()) {
      find_fwd_boundaries(child, discovered);
      stack.erase(start_boundary);
      return;
    }
  } else if(current->is_graph_output(idx)) {
    if(start_boundary.instance == "") {
      //global output, we're done.
      stack.erase(start_boundary);
      return;
    }
    Graph_Node parent = go_up(start_boundary);

    if(stack.find(parent) == stack.end()) {
      find_fwd_boundaries(parent, discovered, true);
      stack.erase(start_boundary);
      return;
    }
  }

  for(auto &out : current->out_edges(idx)) {
    //FIXME: what other cases do we need to take into account for propagation?
    if(current->node_type_get(idx).op == SubGraph_Op && out.get_out_pin().get_pid() != pid)
      continue;
    Index_ID next = out.get_inp_pin().get_nid();

    std::set<uint32_t> relevant_bits;
    int                found = resolve_bit_fwd(current, next, start_boundary.bit, out.get_inp_pin().get_pid(), relevant_bits);
    if(found < 0)
      continue;

    for(uint32_t bit : relevant_bits) {
      Graph_Node bound(current, next, bit, start_boundary.instance, out.get_inp_pin().get_pid());
      if(is_invariant(bound)) {
        discovered.insert(bound);
      } else {
        if(stack.find(bound) == stack.end())
          find_fwd_boundaries(bound, discovered);
      }
    }
  }

  stack.erase(start_boundary);
}

bool Diff_finder::is_user_def(LGraph *current, Index_ID idx, Port_ID pid) const {
  Index_ID port_id = current->get_idx_from_pid(idx, pid);
  return current->is_graph_output(idx) || current->is_graph_input(idx) || (current->get_wid(port_id) != 0);
}

bool Diff_finder::is_invariant(Graph_Node node) {
  LGraph *    current  = node.module;
  Index_ID    idx      = node.idx;
  uint32_t    bit      = node.bit;
  std::string instance = node.instance;
  std::string net_name;
  if(current->is_graph_output(idx)) {
    net_name = current->get_graph_output_name(idx);
  } else if(current->is_graph_input(idx)) {
    net_name = current->get_graph_input_name(idx);
  } else if(current->get_wid(idx) != 0) {
    net_name = current->get_node_wirename(idx);
  } else {
    return false;
  }
  std::string hierarchical_name = "";
  if(instance != "")
    hierarchical_name = instance + hier_sep + net_name;
  else
    hierarchical_name = net_name;

  Index_ID    synth_idx;
  WireName_ID wire_id;
  if(synth->has_name(hierarchical_name.c_str())) {
    synth_idx = synth->get_node_id(hierarchical_name.c_str());
    wire_id   = synth->get_wid(synth_idx);
  } else {
    return false;
  }
  Net_ID id = std::make_pair(wire_id, bit);

  assert(synth_map.find(node) == synth_map.end() || synth_map[node].first == 0 || synth_map[node] == id);
  synth_map[node] = id;
  bound2net[node] = net_name;

  return boundaries->is_invariant_boundary(id);
}

bool Diff_finder::compare_cone(const Graph_Node &start_boundary, const Graph_Node &original_boundary, bool went_up) {

  std::string instance = start_boundary.instance;

  if(different.find(start_boundary) != different.end()) {
    assert(cones.find(start_boundary) != cones.end());
    assert(endpoints.find(start_boundary) != endpoints.end());
    return different[start_boundary];
  }

  assert(cones.find(start_boundary) == cones.end());
  assert(endpoints.find(start_boundary) == endpoints.end());

  endpoints[start_boundary] = std::set<Graph_Node>();
  cones[start_boundary]     = std::set<Graph_Node>();
  different[start_boundary] = false;

  cones[start_boundary].insert(start_boundary);

  Index_ID idx     = start_boundary.idx;
  LGraph * current = start_boundary.module;
  Port_ID  pid     = start_boundary.pid;

  if(current->get_wid(idx)) {
    fmt::print("{} {} {} {} {} \n", start_boundary.module->get_name(), idx, pid, instance, current->get_node_wirename(idx));
  } else {
    fmt::print("{} {} {} {} nowirename \n", start_boundary.module->get_name(), idx, pid, instance);
  }

  LGraph * current_original = original_boundary.module;
  Index_ID original_idx     = original_boundary.idx;

  stack.insert(start_boundary);

  assert(original_idx != -1);

  assert(current_original->node_type_get(original_idx).op != TechMap_Op);
  assert(current->node_type_get(idx).op != TechMap_Op);

  if(current_original->node_type_get(original_idx).op !=
     current->node_type_get(idx).op)
    different[start_boundary] = true;

  if(!went_up && current->node_type_get(idx).op == SubGraph_Op) {
    Graph_Node child = go_down(start_boundary, true);
    Graph_Node child_original;

    if(current_original->node_type_get(original_idx).op != SubGraph_Op ||
       current->subgraph_id_get(idx) != current_original->subgraph_id_get(original_idx)) {
      different[start_boundary] = true;
      child_original            = original_boundary;
    } else {
      Graph_Node this_original(current_original, original_idx, start_boundary.bit, instance, pid);
      child_original = go_down(this_original, true);
    }

    cones[start_boundary].insert(child);

    bool diff = compare_cone(child, child_original);

    if(diff) {
      different[start_boundary] = true;
    }
    cones[start_boundary].insert(cones[child].begin(), cones[child].end());
    endpoints[start_boundary].insert(endpoints[child].begin(), endpoints[child].end());

    stack.erase(start_boundary);
    return different[start_boundary];
  } else if(current->is_graph_input(idx) && boundaries->hierarchy_tree[Invariant_boundaries::get_graphID(start_boundary.module)].size() > 0) {

    Graph_Node parent = go_up(start_boundary);
    Graph_Node parent_original;

    if(!current_original->is_graph_input(original_idx) ||
       std::string(current->get_graph_input_name(idx)) != std::string(current_original->get_graph_input_name(original_idx))) {
      different[start_boundary] = true;
      parent_original           = original_boundary;
    } else {
      //all pids on IO nodes is 0
      Graph_Node this_original(current_original, original_idx, start_boundary.bit, instance, 0);
      parent_original = go_up(this_original);
      //what about cases where module IOs change?
      assert(parent.pid == parent_original.pid);
    }

    cones[start_boundary].insert(parent);
    bool diff = compare_cone(parent, parent_original, true);
    if(diff) {
      different[start_boundary] = true;
    }
    cones[start_boundary].insert(cones[parent].begin(), cones[parent].end());
    endpoints[start_boundary].insert(endpoints[parent].begin(), endpoints[parent].end());

    stack.erase(start_boundary);
    return different[start_boundary];
  }

  for(auto &inp : current->inp_edges(idx)) {
    //in cases like join/pick we only propagate to a specific bit
    std::set<uint32_t> useful_bits;
    int                found_bit = resolve_bit(current, idx, start_boundary.bit, inp.get_inp_pin().get_pid(), useful_bits);
    if(found_bit == -1) // do not propagate through this pid
      continue;
    Index_ID previous = inp.get_out_pin().get_nid();

    //for subgraphs, only propagate through the input we exited through
    if(pid != inp.get_inp_pin().get_pid() &&
       start_boundary.module->node_type_get(start_boundary.idx).op == SubGraph_Op)
      continue;

    Port_ID n_pid = inp.get_inp_pin().get_pid();

    //just assign a valid idx while we look for the right one
    Index_ID previous_original = original_idx;
    bool     found             = false;

    if(!different[start_boundary]) {
      //if the cone is already different, don't bother comparing it anymore
      //just propagate over the new graph
      for(auto &cinp : current_original->inp_edges(original_idx)) {
        if(cinp.get_inp_pin().get_pid() == n_pid) {
          if(!found) {
            previous_original = (cinp.get_out_pin().get_nid());
          } else {
            //there is more than one edge to the same pid, like Sum
            //let's try to figure out which one we need to follow
            if(previous == previous_original)
              continue;
            else if(previous == (cinp.get_out_pin().get_nid())) {
              previous_original = (cinp.get_out_pin().get_nid());
            } else if(std::abs(static_cast<int64_t>(previous) - static_cast<int64_t>(previous_original)) < std::abs(static_cast<int64_t>(previous) - static_cast<int64_t>(cinp.get_out_pin().get_nid()))) {
              previous_original = (cinp.get_out_pin().get_nid());
            } else if(std::abs(static_cast<int64_t>(previous) - static_cast<int64_t>(previous_original)) > std::abs(static_cast<int64_t>(previous) - static_cast<int64_t>(cinp.get_out_pin().get_nid()))) {
              continue;
            } else {
              //is there any way to distinguish?
              //at this point any bet is as good as any.
              //Taking the wrong fan in may cause more mismatches than there
              //are, but it should be functionally correct
              continue;
            }
          }
          found = true;
        }
      }
      if(!found) {
        different[start_boundary] = true;
      }
    }

    for(uint32_t bit : useful_bits) {
      //some operators like Sum and Mult may have inputs with different
      //bitwidths
      if(current->get_bits_pid(previous, inp.get_out_pin().get_pid()) < bit)
        continue;

      Graph_Node bound(current, previous, bit, instance, inp.get_out_pin().get_pid());
      Graph_Node orig(current_original, previous_original, bit, instance, inp.get_out_pin().get_pid());

      if(is_invariant(bound)) {
        endpoints[start_boundary].insert(bound);
        //cones[start_boundary].insert(bound);
      } else {
        cones[start_boundary].insert(bound);

        if(stack.find(bound) == stack.end()) {
          bool diff = compare_cone(bound, orig);
          if(diff) {
            different[start_boundary] = true;
          }
          cones[start_boundary].insert(cones[bound].begin(), cones[bound].end());
          endpoints[start_boundary].insert(endpoints[bound].begin(), endpoints[bound].end());
        }
      }
    }
  }

  stack.erase(start_boundary);
  return different[start_boundary];
}

void Diff_finder::add_ios_up(LGraph *module, Index_ID nid, std::map<std::string, LGraph *> &name2graph) {
  for(auto &parent : boundaries->hierarchy_tree[Invariant_boundaries::get_graphID(module)]) {

    //not in delta, do nothing
    if(name2graph.find("lgraph_" + parent) == name2graph.end()) {
      continue;
    }
    assert(name2graph.find("lgraph_" + parent) != name2graph.end());
    LGraph *nparent = name2graph["lgraph_" + parent];

    //not in delta, do nothing
    if(!nparent)
      continue;

    //FIXME: inefficient solution, it would be better to be able to get the node
    //from a map or something like that
    std::set<Index_ID> parent_ids;
    std::string        module_name = module->get_name();
    if(std::strncmp(module_name.c_str(), "lgraph_", 7) == 0) {
      module_name = module_name.substr(7);
    }
    for(auto node : nparent->fast()) {
      if(nparent->node_type_get(node).op != SubGraph_Op)
        continue;

      uint32_t subgraph_id = nparent->subgraph_id_get(node);

      if(subgraph_id == nparent->get_library()->get_id_const(module_name))
        parent_ids.insert(node);
    }

    assert(parent_ids.size() > 0);

    Index_ID pnid;
    if(module->is_graph_input(nid)) {

      for(auto idx_in_parent : parent_ids) {
        assert(nparent->get_instance_name_id(idx_in_parent) != 0);
        std::string wire_name = "lgraph_hier_" + std::string(nparent->get_node_instancename(idx_in_parent)) + hier_sep + module->get_graph_input_name(nid);
        if(nparent->is_graph_input(wire_name)) {
          fmt::print("input {} already exists in parent module {}\n", wire_name, nparent->get_name());
          return;
        }
        pnid = nparent->add_graph_input(wire_name.c_str(), 0, module->get_bits(nid), module->get_offset(nid));
        nparent->add_edge(Node_Pin(pnid, 0, false), Node_Pin(idx_in_parent, nid, true));
      }

    } else {
      assert(module->is_graph_output(nid));
      for(auto idx_in_parent : parent_ids) {
        assert(nparent->get_instance_name_id(idx_in_parent) != 0);
        std::string wire_name = "lgraph_hier_" + std::string(nparent->get_node_instancename(idx_in_parent)) + hier_sep + module->get_graph_output_name(nid);
        if(nparent->is_graph_output(wire_name)) {
          fmt::print("output {} already exists in parent module {}\n", wire_name, nparent->get_name());
          return;
        }
        pnid = nparent->add_graph_output(wire_name.c_str(), 0, module->get_bits(nid), module->get_offset(nid));
        nparent->add_edge(Node_Pin(idx_in_parent, nid, false), Node_Pin(pnid, 0, true));
      }
    }
    add_ios_up(nparent, pnid, name2graph);
  }
}

void Diff_finder::generate_modules(std::set<Graph_Node> &different_nodes, const std::string &out_lgdb) {

  std::map<std::string, LGraph *> name2graph;
  std::map<Graph_Node, Index_ID>  node2idx;

  std::map<LGraph *, std::map<Index_ID, Index_ID>> old2newidx;

  std::set<std::pair<LGraph *, Index_ID>> visited_idx;

  for(auto node : different_nodes) {
    LGraph *original = node.module;

    //FIXME: this is happening due to multibit nodes, is there a way to prevent it?
    if(visited_idx.find(std::make_pair(original, node.idx)) != visited_idx.end()) {
      continue;
    }
    visited_idx.insert(std::make_pair(original, node.idx));
    if(name2graph.find(original->get_name()) == name2graph.end()) {
      std::string name = original->get_name();
      fmt::print("creating graph for {}\n", name);
      if(name.substr(0, 6) == "lgraph")
        name = name.substr(7);
      name2graph[original->get_name()] = new LGraph(out_lgdb, name, true);
    }
    LGraph *new_module = name2graph[original->get_name()];

    Index_ID idx;
    if(original->is_graph_input(node.idx)) {
      if(!new_module->is_graph_input(original->get_graph_input_name(node.idx))) {
        idx = new_module->add_graph_input(original->get_graph_input_name(node.idx), 0, original->get_bits(node.idx), original->get_offset(node.idx));
      } else {
        //input already created
        idx = new_module->get_graph_input(original->get_graph_input_name(node.idx)).get_nid();
      }
    } else if(original->is_graph_output(node.idx)) {
      if(!new_module->is_graph_output(original->get_graph_output_name(node.idx))) {
        idx = new_module->add_graph_output(original->get_graph_output_name(node.idx), 0, original->get_bits(node.idx), original->get_offset(node.idx));
      } else {
        //output already created
        idx = new_module->get_graph_output(original->get_graph_output_name(node.idx)).get_nid();
      }
    } else {
      idx = new_module->create_node().get_nid();

      if(original->get_instance_name_id(node.idx) != 0)
        new_module->set_node_instance_name(idx, original->get_node_instancename(node.idx));

      if(original->get_wid(node.idx) != 0)
        new_module->set_node_wirename(idx, original->get_node_wirename(node.idx));

      if(original->node_type_get(node.idx).op < GraphIO_Op || original->node_type_get(node.idx).op == BlackBox_Op) {
        new_module->node_type_set(idx, original->node_type_get(node.idx).op);
        new_module->set_bits(idx, original->get_bits(node.idx));
      } else {
        switch(original->node_type_get(node.idx).op) {
        case SubGraph_Op:
          //assert(name2graph.find(original->get_subgraph_name(node.idx)) != name2graph.end());
          new_module->node_subgraph_set(idx, original->subgraph_id_get(node.idx));
          break;
        case TechMap_Op:
          new_module->node_tmap_set(idx, original->tmap_id_get(node.idx));
          new_module->set_bits(idx, original->get_bits(node.idx));
          break;
        case U32Const_Op:
          new_module->node_u32type_set(idx, original->node_value_get(node.idx));
          break;
        case StrConst_Op:
          new_module->node_const_type_set(idx, original->node_const_value_get(node.idx));
          break;
        default:
          console->error("Node type unrecognized {}\n", original->node_type_get(node.idx).op);
          break;
        }
      }
    }
    node2idx[node]                 = idx;
    old2newidx[original][node.idx] = idx;
  }

  std::set<std::pair<LGraph *, int>> visited;
  std::map<LGraph *, int>            wire_id;
  for(auto &node : different_nodes) {
    LGraph * new_module = name2graph[node.module->get_name()];
    Index_ID idx        = node2idx[node];

    //this skips multiple bits from the same node
    if(visited.find(std::make_pair(node.module, node.idx)) != visited.end())
      continue;

    visited.insert(std::make_pair(node.module, node.idx));

    for(auto &inp : node.module->inp_edges(node.idx)) {
      if(old2newidx[node.module].find(inp.get_out_pin().get_nid()) != old2newidx[node.module].end()) {
        //input included in delta
        Port_ID outpid = inp.get_out_pin().get_pid();
        Port_ID inppid = inp.get_inp_pin().get_pid();
        if(new_module->is_graph_input(old2newidx[node.module][inp.get_out_pin().get_nid()]) ||
           new_module->is_graph_output(old2newidx[node.module][inp.get_out_pin().get_nid()])) {
          outpid = 0;
        }
        if(new_module->is_graph_input(idx) || new_module->is_graph_output(idx)) {
          inppid = 0;
        }
        if(new_module->node_type_get(old2newidx[node.module][inp.get_inp_pin().get_nid()]).op == SubGraph_Op) {
          //edge load is a subgraph
          std::string subgraph_name = new_module->get_subgraph_name(old2newidx[node.module][inp.get_inp_pin().get_nid()]);
          assert(name2graph.find("lgraph_" + subgraph_name) != name2graph.end());
          LGraph * nsubgraph = name2graph["lgraph_" + subgraph_name];
          LGraph * osubgraph = LGraph::find_graph(subgraph_name, node.module->get_path());
          Index_ID inpnid    = osubgraph->get_graph_input_nid_from_pid(inp.get_inp_pin().get_pid());
          assert(inpnid);

          if(nsubgraph->is_graph_input(osubgraph->get_graph_input_name(inpnid))) {
            inppid = nsubgraph->get_graph_input(osubgraph->get_graph_input_name(inpnid)).get_pid();
          } else {
            //input port (load) not present in the delta graph, can safely skip
            continue;
          }
        }
        if(new_module->node_type_get(old2newidx[node.module][inp.get_out_pin().get_nid()]).op == SubGraph_Op) {
          //edge driver is a subgraph
          std::string subgraph_name = new_module->get_subgraph_name(old2newidx[node.module][inp.get_out_pin().get_nid()]);
          assert(name2graph.find("lgraph_" + subgraph_name) != name2graph.end());
          LGraph * nsubgraph = name2graph["lgraph_" + subgraph_name];
          LGraph * osubgraph = LGraph::find_graph(subgraph_name, node.module->get_path());
          Index_ID outnid    = osubgraph->get_graph_output_nid_from_pid(inp.get_out_pin().get_pid());

          assert(outnid);
          if(nsubgraph->is_graph_output(osubgraph->get_graph_output_name(outnid))) {
            outpid = nsubgraph->get_graph_output(osubgraph->get_graph_output_name(outnid)).get_pid();

            //FIXME: refactor to reduce code replication
          } else {
            if(node.module->node_type_get(idx).op == GraphIO_Op)
              continue;

            //driver submodule is present in delta, but driver pin is not,
            //promote load input to module input
            Index_ID name_idx = node.module->get_idx_from_pid(inp.get_out_pin().get_nid(), inp.get_out_pin().get_pid());
            assert(osubgraph->get_graph_output_nid_from_pid(inp.get_out_pin().get_pid()));
            assert(!new_module->is_graph_output(osubgraph->get_graph_output_name_from_pid(inp.get_inp_pin().get_pid())));

            Index_ID inp_id = 0;
            if(new_module->is_graph_input(osubgraph->get_graph_output_name_from_pid(inp.get_out_pin().get_pid()))) {
              //node has previously been promoted
              inp_id = new_module->get_graph_input(osubgraph->get_graph_output_name_from_pid(inp.get_out_pin().get_pid())).get_nid();
            } else {
              inp_id = new_module->add_graph_input(
                  osubgraph->get_graph_output_name_from_pid(inp.get_out_pin().get_pid()), 0,
                  osubgraph->get_bits(osubgraph->get_graph_output_nid_from_pid(inp.get_out_pin().get_pid())), 0);
              add_ios_up(new_module, inp_id, name2graph);
              assert(old2newidx[node.module].find(name_idx) == old2newidx[node.module].end());
              old2newidx[node.module][name_idx] = inp_id;
            }
            new_module->add_edge(
                Node_Pin(inp_id, 0, false),
                Node_Pin(idx, inppid, true));
            continue;
          }
        }
        new_module->add_edge(
            Node_Pin(old2newidx[node.module][inp.get_out_pin().get_nid()], outpid, false),
            Node_Pin(idx, inppid, true));

      } else {

        Port_ID subgraph_inpid = inp.get_inp_pin().get_pid();
        if(node.module->node_type_get(node.idx).op == SubGraph_Op) {
          std::string subgraph_name =
              new_module->get_subgraph_name(old2newidx[node.module][inp.get_inp_pin().get_nid()]);
          assert(name2graph.find("lgraph_" + subgraph_name) != name2graph.end());
          LGraph *osubgraph = LGraph::find_graph(subgraph_name, node.module->get_path());
          LGraph *nsubgraph = name2graph["lgraph_" + subgraph_name];

          Index_ID subgraph_innid = osubgraph->get_graph_input_nid_from_pid(inp.get_inp_pin().get_pid());

          if(old2newidx.find(osubgraph) == old2newidx.end() ||
             old2newidx[osubgraph].find(subgraph_innid) == old2newidx[osubgraph].end())
            //for subgraphs we only want to go over inputs that are in the
            //resynthesis region
            continue;

          subgraph_inpid = nsubgraph->get_graph_input(osubgraph->get_graph_input_name(subgraph_innid)).get_pid();
        }

        //FIXME refactor to reduce code replication
        if(node.module->node_type_get(inp.get_out_pin().get_nid()).op == U32Const_Op) {
          //node not included but simple constant
          Index_ID const_id = new_module->create_node().get_nid();
          new_module->node_u32type_set(const_id, node.module->node_value_get(inp.get_out_pin().get_nid()));
          if(new_module->is_graph_output(idx)) {
            new_module->add_edge(
                Node_Pin(const_id, 0, false),
                Node_Pin(idx, idx, true));
          } else {
            new_module->add_edge(
                Node_Pin(const_id, 0, false),
                Node_Pin(idx, subgraph_inpid, true));
          }
        } else if(node.module->node_type_get(inp.get_out_pin().get_nid()).op == StrConst_Op) {
          //node not included but simple constant
          Index_ID const_id = new_module->create_node().get_nid();
          new_module->node_const_type_set(const_id, node.module->node_const_value_get(inp.get_out_pin().get_nid()));
          if(new_module->is_graph_output(idx)) {
            new_module->add_edge(
                Node_Pin(const_id, 0, false),
                Node_Pin(idx, idx, true));
          } else {
            new_module->add_edge(
                Node_Pin(const_id, 0, false),
                Node_Pin(idx, subgraph_inpid, true));
          }

        } else {
          //input not included in delta, promote this node to input
          //for joins, we can skip remaining wires
          if(node.module->node_type_get(node.idx).op == Join_Op || is_invariant(node)) {
            continue;
          }

          Index_ID name_idx = node.module->get_idx_from_pid(inp.get_out_pin().get_nid(), inp.get_out_pin().get_pid());
          assert(node.module->get_wid(name_idx) != 0);
          Index_ID inp_id = new_module->add_graph_input(node.module->get_node_wirename(name_idx), 0, node.module->get_bits(name_idx), 0);
          add_ios_up(new_module, inp_id, name2graph);
          assert(old2newidx[node.module].find(name_idx) == old2newidx[node.module].end());
          old2newidx[node.module][name_idx] = inp_id;
          if(new_module->is_graph_output(idx)) {
            new_module->add_edge(
                Node_Pin(inp_id, 0, false),
                Node_Pin(idx, idx, true));
          } else {
            new_module->add_edge(
                Node_Pin(inp_id, 0, false),
                Node_Pin(idx, subgraph_inpid, true));
          }
        }
      }
    }

    //if there is at least one output from the delta region to a node outside
    //the delta region, this node needs to be made an output
    for(auto &out : node.module->out_edges(node.idx)) {
      if(node.module->node_type_get(node.idx).op == SubGraph_Op) {
        //if node is a subgraph, I need to first check if the output port is in the delta
        std::string subgraph_name = new_module->get_library()->get_name(new_module->subgraph_id_get(old2newidx[node.module][node.idx]));
        assert(name2graph.find("lgraph_" + subgraph_name) != name2graph.end());
        LGraph *nsubgraph = name2graph["lgraph_" + subgraph_name];
        LGraph *osubgraph = LGraph::find_graph(subgraph_name, node.module->get_path());
        if(!nsubgraph->is_graph_output(osubgraph->get_graph_output_name_from_pid(out.get_out_pin().get_pid()))) {
          continue;
        }
      }
      if(node.module->is_graph_input(node.idx) || node.module->is_graph_output(node.idx)) {
        continue;
      }
      if(old2newidx[node.module].find(out.get_inp_pin().get_nid()) == old2newidx[node.module].end()) {
        fmt::print("module {}, from cell {}, to_cell {}, from_pin {}, to pin {}\n",
                   node.module->get_name(),
                   out.get_out_pin().get_nid(), out.get_inp_pin().get_nid(),
                   out.get_out_pin().get_pid(), out.get_inp_pin().get_pid());

        std::string wirename = "";
        if(node.module->get_wid(node.idx) != 0) {
          // wire has user defined name
          assert(new_module->has_name(node.module->get_node_wirename(node.idx))); // wirename should already be in graph unless we messed up something
          wirename = std::string(node.module->get_node_wirename(node.idx)) + "_output";
        } else {
          // wire does not have user defined name
          wire_id[new_module] = wire_id[new_module] + 1;
          wirename            = "__lgraph__generated_wire_" + new_module->get_name() + "__driver__" + std::to_string(out.get_inp_pin().get_nid()) + "__pin__" + std::to_string(out.get_inp_pin().get_pid()) + "__id__" + std::to_string(wire_id[new_module]);
          assert(!new_module->is_graph_output(wirename));
        }

        Index_ID out_id = new_module->add_graph_output(wirename.c_str(), 0, node.module->get_bits(node.idx), 0);
        add_ios_up(new_module, out_id, name2graph);
        old2newidx[node.module][out.get_out_pin().get_nid()] = out_id;
        if(new_module->is_graph_input(idx) || new_module->is_graph_output(idx)) {
          new_module->add_edge(
              Node_Pin(idx, 0, false),
              Node_Pin(out_id, 0, true));
        } else {
          new_module->add_edge(
              //Node_Pin(idx, out.get_out_pin().get_pid(), false),
              Node_Pin(idx, out.get_out_pin().get_pid(), false),
              Node_Pin(out_id, 0, true));
        }

        //only one output per node is needed
        break;
      }
    }
  }

  //sync all graphs to disk
  for(auto &name_graph : name2graph) {
    fmt::print("sync'ing graph {}\n", name_graph.first);
    name_graph.second->sync();
  }
}

void Diff_finder::generate_delta(const std::string &modified_lgdb, const std::string &out_lgdb, std::set<Net_ID> &diffs) {

  std::set<LGraph *>   discovered_modules;
  std::set<Graph_Node> discovered_boundaries, visited_boundaries, all_diff;

  Graph_library *modified_library = Graph_library::instance(modified_lgdb);
  for(int id = 0; id < modified_library->lgraph_count(); id++) {
    assert(modified_library->get_graph(id));
    discovered_modules.insert(modified_library->get_graph(id));
    fmt::print("discovered module {} \n", modified_library->get_graph(id)->get_name());
  }

  while(discovered_modules.size() > 0) {
    LGraph *current = *(discovered_modules.begin());
    discovered_modules.erase(current);

    fmt::print("current module {} \n", current->get_name());

    if(current->get_name() == "lgraph_fp_add64")
      fmt::print("foo");

    for(auto &instance : boundaries->instance_collection[Invariant_boundaries::get_graphID(current)]) {
      for(auto &ridx : current->fast()) {
        std::set<Port_ID> out_pids;
        if(current->is_graph_output(ridx)) {
          //graph outputs may not have out_edges
          out_pids.insert(0);
        } else {
          for(auto &cout : current->out_edges(ridx)) {
            out_pids.insert(cout.get_out_pin().get_pid());
          }
        }

        for(Port_ID pid : out_pids) {
          Index_ID idx = current->get_idx_from_pid(ridx, pid);

          if(!is_user_def(current, ridx, pid))
            continue;

          for(int bit = 0; bit < current->get_bits(idx); bit++) {
            Graph_Node bound(current, ridx, bit, instance, pid);

            LGraph *current_original = LGraph::find_graph(current->get_name(), original->get_path());
            if(!current_original) {
              //dealing with "lgraph_" that is appended to graph name
              current_original = LGraph::find_graph(current->get_name().substr(7), original->get_path());
            }
            assert(current_original);

            if(is_invariant(bound)) {
              visited_boundaries.insert(bound);

              Graph_Node orig(current_original, current_original->get_node_id(bound2net[bound]), bit, instance, pid);
              bool       diff = compare_cone(bound, orig);
              assert(stack.size() == 0);
              visited_boundaries.insert(bound);

              if(diff) {
#ifdef DEBUG
                for(auto foo : cones[bound]) {
                  fmt::print("    diff mod {} {}:{}\n", foo.module->get_name(), foo.idx, foo.pid);
                }
#endif
                all_diff.insert(cones[bound].begin(), cones[bound].end());
                diffs.insert(synth_map[bound]);
              }

            } else if(current->is_graph_output(idx)) {
              //propagate fwd from non-boundary outputs to get the next boundary
              find_fwd_boundaries(bound, discovered_boundaries);
            }
          }
        }
      }
    }
  }

  for(auto &bound : discovered_boundaries) {
    if(visited_boundaries.find(bound) == visited_boundaries.end()) {
      visited_boundaries.insert(bound);

      LGraph *current_original = LGraph::find_graph(bound.module->get_name(), original->get_path());
      if(!current_original) {
        //dealing with "lgraph_" that is appended to graph name
        current_original = LGraph::find_graph(bound.module->get_name().substr(7), original->get_path());
      }
      assert(current_original);
      Graph_Node orig(current_original, current_original->get_node_id(bound2net[bound]), bound.bit, bound.instance, bound.pid);

      bool diff = compare_cone(bound, orig);
      assert(stack.size() == 0);
      visited_boundaries.insert(bound);
      if(diff) {
        all_diff.insert(cones[bound].begin(), cones[bound].end());
        diffs.insert(synth_map[bound]);
      }
    }
  }

#ifdef DEBUG
  for(auto foo : all_diff) {
    fmt::print("alldiff mod {}\n", foo.module->get_name());
  }
#endif
  generate_modules(all_diff, out_lgdb);
}
