//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.
#include "lgwirenames.hpp"

LGraph_WireNames::LGraph_WireNames(const std::string &path, const std::string &name) noexcept
    : Lgraph_base_core(path, name)
    , LGraph_Base(path, name)
    , names(path, name + "_wnames")
    , wires(path + "/" + name + "_wid")
    , offsets(path + "/" + name + "_offsets") {
}

void LGraph_WireNames::clear() {
  names.clear();
  wires.clear();
  offsets.clear();
}

void LGraph_WireNames::reload() {
  names.reload();
  wires.reload();
  offsets.reload();
}

void LGraph_WireNames::sync() {
  names.sync();
  wires.sync();
  offsets.sync();
}

void LGraph_WireNames::emplace_back() {
  wires.emplace_back();
  wires[wires.size() - 1] = 0;

  offsets.emplace_back();
  offsets[offsets.size() - 1] = 0;
}

/*WireName_ID LGraph_WireNames::get_wirename_id(const char *wirename) {
  //checks before creating
  return names.create_id(wirename);
}*/

const char *LGraph_WireNames::get_wirename(WireName_ID wid) const {
  return names.get_char(wid);
}

WireName_ID LGraph_WireNames::get_wid(Index_ID nid) const {
  assert(nid < wires.size());
  assert(node_internal[nid].is_node_state());
  assert(node_internal[nid].is_root());

  return wires[nid];
}

void LGraph_WireNames::set_node_wirename(Index_ID nid, WireName_ID wid) {
  assert(nid < wires.size());
  assert(node_internal[nid].is_node_state());
  assert(node_internal[nid].is_root());
  // assert(node_internal[nid].get_nid() == nid);

  wires[nid] = wid;
}

bool LGraph_WireNames::has_name(const char *name) const {
  return names.include(name);
}

Index_ID LGraph_WireNames::get_node_id(const char *name) const {
  return names.get_field(names.get_id(name));
}
