
#include <iostream>
#include <random>

#include "lgraph.hpp"
#include "lgbench.hpp"

#include "inou.hpp"

#include "pass/fluid/pass_fluid.hpp"

int main(int argc, const char **argv) {
  LGBench b;

  Options::setup(argc, argv);

  Pass_fluid fluid;
  Inou_trivial inou;

  Options::setup_lock();

  std::vector<LGraph *> lgs = inou.generate();
  b.sample("setup");

  for(auto g:lgs) {
    console->info("processing {}\n",g->get_name());
    fluid.transform(g);
  }

  b.sample("fluid_pass");

  for(auto g:lgs) {
    g->sync();
  }
}
