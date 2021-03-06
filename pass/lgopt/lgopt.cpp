//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <iostream>
#include <random>

#include "lgbench.hpp"
#include "lgraph.hpp"

#include "inou.hpp"

#include "pass/sample/pass_topological.hpp"
#include "pass/sample/pass_fast.hpp"

#include "pass/cse/pass_cse.hpp"
#include "pass/vectorize/pass_vectorize.hpp"

class Pass_options_pack : public Options_pack {
  std::set<std::string> enabled;

public:
  std::vector<std::string> passes;

  Pass_options_pack();
};

Pass_options_pack::Pass_options_pack() {
  assert(Options::get_cargc() != 0); // Options::setup(argc,argv) must be called before setup() is called

  Options::get_desc()->add_options()("passes", boost::program_options::value(&passes), "Sequence of passes enabled");

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::command_line_parser(Options::get_cargc(), Options::get_cargv()).options(*Options::get_desc()).allow_unregistered().run(), vm);

  if(vm.count("passes")) {
    passes = vm["passes"].as<std::vector<std::string>>();
  } else {
    passes.push_back("cse");
    passes.push_back("vectorize");
  }
}

void terminate(std::vector<LGraph *> lgs) {
  for(auto g : lgs) {
    g->sync();
  }
}

int main(int argc, const char **argv) {
  LGBench b;

  Options::setup(argc, argv);

  Pass_options_pack opack;

  Inou_trivial inou;
  Options::setup_lock();

  std::vector<LGraph *> lgs = inou.generate();
  b.sample("setup");

  for(auto g : lgs) {
    console->info("processing {}\n", g->get_name());
    for(const auto pass_name : opack.passes) {
      fmt::print("starting {} pass...\n", pass_name);
      if(pass_name == "cse") {
        Pass_cse cse;
        cse.transform(g);
      } else if(pass_name == "vectorize") {
        Pass_vectorize vectorize;
        vectorize.transform(g);
      } else if(pass_name == "fast") {
        Pass_fast fast;
        for(int i = 0; i < 2000; i++) {
        fast.transform(g);
        }
      b.sample(pass_name + " pass1");
        fast.transform(g);
      b.sample(pass_name + " pass2");
      } else if(pass_name == "topological") {
        Pass_topo topo;
        for(int i = 0; i < 50; i++) {
        topo.transform(g);
        }
      b.sample(pass_name + " pass1");
        topo.transform(g);
      b.sample(pass_name + " pass2");
      } else {
        fmt::print("ERROR: unknown {} pass\n", pass_name);
        terminate(lgs);
      }
      b.sample(pass_name + " pass");
    }
  }

  terminate(lgs);

  return 0;
}
