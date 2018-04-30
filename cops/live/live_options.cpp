

#include "live_options.hpp"
#include "core/lglog.hpp"

Live_pass_pack::Live_pass_pack(int argc, const char ** argv) {

  Options::setup(argc, argv);

  Options::get_desc()->add_options()
    ("original_lgdb,o", boost::program_options::value(&original_lgdb)->required(), "lgdb path of the original netlist")
    ("modified_lgdb,m", boost::program_options::value(&modified_lgdb)->required(), "lgdb path of the modified netlist")
    ("synth_lgdb,s", boost::program_options::value(&synth_lgdb)->required(), "lgdb path of the synthesized netlist")
    ("delta_lgdb,s", boost::program_options::value(&delta_lgdb), "lgdb path for the generated delta lgraph (default olgdb)")
    ("boundaries,b", boost::program_options::value(&boundaries_name)->required(), "invariant boundaries files generated by lginvariant")
    ("diff,d", boost::program_options::value(&diff_file), "file containing different boundaries (default=diff)")
    ("help,h", "print usage message")
    ;

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::command_line_parser(Options::get_cargc(), Options::get_cargv()).options(*Options::get_desc()).allow_unregistered().run(), vm);

  if(vm.count("help"))
    return;

  if(vm.count("original_lgdb")) {
    original_lgdb  = vm["original_lgdb"].as<std::string>();
  } else {
    console->error("original_lgdb is required\n");
    exit(-1);
  }
  if(vm.count("modified_lgdb")) {
    modified_lgdb  = vm["modified_lgdb"].as<std::string>();
  } else {
    console->error("modified_lgdb is required\n");
    exit(-1);
  }
  if(vm.count("synth_lgdb")) {
    synth_lgdb = vm["synth_lgdb"].as<std::string>();
  } else {
    console->error("synth_lgdb is required\n");
    exit(-1);
  }
  if(vm.count("delta_lgdb")) {
    delta_lgdb = vm["delta_lgdb"].as<std::string>();
  } else {
    delta_lgdb = "olgdb";
  }
  if(vm.count("boundaries")) {
    boundaries_name = vm["boundaries"].as<std::string>();
  } else {
    console->error("boundaries is required\n");
    exit(-1);
  }
  if(vm.count("diff")) {
    diff_file = vm["diff"].as<std::string>();
  } else {
    diff_file = "diff";
  }

  Options::setup_lock();
}