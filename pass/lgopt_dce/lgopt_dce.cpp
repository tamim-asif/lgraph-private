//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <iostream>
#include <random>

#include "lgbench.hpp"
#include "lgraph.hpp"

#include "inou.hpp"

#include "pass/dce/pass_dce.hpp"

int main(int argc, const char **argv) {
  LGBench b;

  Options::setup(argc, argv);

  Pass_dce     dce;
  Inou_trivial inou;

  Options::setup_lock();

  std::vector<LGraph *> lgs = inou.generate();
  b.sample("setup");

  for(auto g : lgs) {
    console->info("processing {}\n", g->get_name());
    dce.transform(g);
  }

  b.sample("dce_pass");

  for(auto g : lgs) {
    g->sync();
  }
}
