//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.
#include <sstream>

#include "inou_cfg.hpp"

#include "cfg_node_data.hpp"

using std::string;

CFG_Node_Data::CFG_Node_Data(const LGraph *g, Index_ID node) {
  const char *data_str = g->get_node_wirename(node);

  if (data_str != nullptr) {
    std::istringstream ss(data_str);

    assert(std::getline(ss, operator_txt, ENCODING_DELIM)); // the first token is the operator, we don't need it
    assert(std::getline(ss, target, ENCODING_DELIM)); // the 2nd var in the data_str is the target
                                                      // this read shouldn't fail

    string buffer;
    while (std::getline(ss, buffer, ENCODING_DELIM)) {
      if (!buffer.empty())
        operands.push_back(buffer);
    }
  } else {
    target = EMPTY_MARKER;
    operator_txt = EMPTY_MARKER;
  }
}

CFG_Node_Data::CFG_Node_Data(const string &parser_raw) {
  std::istringstream ss(parser_raw);
  string buffer;

  for (int i = 0; i < CFG_METADATA_COUNT; ) {   // the first few are metadata, and these reads should not fail or
                                                // we have a formatting issue

    assert(ss >> buffer);

    if (!buffer.empty()) // only count non-empty tokens
      i++;
  }

  ss >> operator_txt;
  ss >> target;

  while (ss >> buffer) { // actually save the remaining
    if (!buffer.empty())
      operands.push_back(buffer);
  }
}

string CFG_Node_Data::encode() const {
  string encd = operator_txt + ENCODING_DELIM + target + ENCODING_DELIM;

  for (const auto &op : operands)
    encd += op + ENCODING_DELIM;

  return encd;
}
