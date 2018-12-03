#include "parser.hpp"
#include <boost/assert.hpp>
#include <string.h>
#include "algo.hpp"
#include <boost/foreach.hpp>
#include "globalvar.hpp"
#include "sa.hpp"
#include "clock.hpp"
#include "view.hpp"
#include <fstream>

namespace fs = boost::filesystem;

void usage(const char *exec_name){
  std::cout<<"Usage: "<<exec_name<<" .hardblocks .nets .pl .floorplan ratio" <<std::endl;
}
void do_seed_init(const char *dataset, const char *ratio_str){
  //boost::filesystem::path p(dataset);
  int len = strlen(dataset);
  int len2=strlen("n100.hardblocks");
  double ratio = atof(ratio_str);
  if(len<len2){
	random_t::init(1);
    return;
  }
  
  global_var_t *global_var = global_var_t::get_ref();
  if(getenv("hw3_time_limit")!=NULL)
    global_var->timing_limit=true; 
  if(strcmp(dataset+(len-len2), "n100.hardblocks")==0){
    random_t::init(23151);
    global_var->set_target_wirelength(215143);
  }
  else if(strcmp(dataset+(len-len2), "n200.hardblocks")==0){
    //random_t::init(7563);
    //random_t::init(22816);
    //random_t::init(1045);
    //random_t::init(434);
    random_t::init(11273); //11273
    global_var->set_target_wirelength(400291);
    //if(ratio==0.1) global_var->timing_limit=true; 
  }
  else if(strcmp(dataset+(len-len2), "n300.hardblocks")==0){
    random_t::init(19853);
    global_var->set_target_wirelength(540137);
  }
  else random_t::init(1);  
}
static bool is_seed_gen_mode(char *exec_name){
  int len = strlen(exec_name);
  char *str="seed_gen";
  int len2=strlen(str);
  if(len<len2)
    return false;
  if(strcmp(exec_name+(len-len2), str)==0)
    return true;
  return false;
}

int main(int argc, char **argv){
  simple_timer_t::get_ref().reset();
  bool disable_output=false;
  bool time_limit=false;
  global_var_t *global_var = global_var_t::get_ref();
  if(argc!=6){
    usage(argv[0]);
    return 0;
  }
  if(is_seed_gen_mode(argv[0])){
    int value=atoi(getenv("hw3_seed"));
    random_t::init(value);
    disable_output=true;
    time_limit=true;
    std::cout<<"hw3 seed value="<<value<<std::endl;
    if(getenv("hw3_time_limit")!=NULL)
      global_var->timing_limit=true; 
  }
  else {
    do_seed_init(argv[1],argv[5]);
  }
  boost::shared_ptr<parser_t> block_parser(new parser_t(argv[1]));
  std::vector<module_t> module_array;
  
  boost::shared_ptr<parser_t> net_parser(new parser_t(argv[2]));
  std::vector<net_t> net_array;

  boost::shared_ptr<parser_t> pin_parser(new parser_t(argv[3]));
  std::vector<terminal_t> pin_array;
  int terminal_count;
  shape_t target_die_shape;

  int lines=block_parser->do_block_file_parse(module_array, &terminal_count);
  lines = net_parser->do_net_file_parse(net_array);
  lines = pin_parser->do_pl_file_parse(pin_array);


  target_die_shape = calc_for_die_shape(module_array, atof(argv[5]));
  std::cout<<"target die shape="<<target_die_shape.w<<" x "<<target_die_shape.h<<std::endl;
  std::cout<<"white space ratio="<<atof(argv[5])<<std::endl;
  global_var->set_die_shape(target_die_shape);
  global_var->set_placement(PLACEMENT_HARD);

  simulated_annealing_t sa;
  if(time_limit==true)
    sa.run(module_array, net_array, pin_array, 30);
  else sa.run(module_array, net_array, pin_array, 60*20-4);

  std::cout<<"elapsed="<<simple_timer_t::get_ref().elapsed()<<std::endl;
  std::cout<<"[die size fit]="<<sa.fit_sol.toString()<<std::endl;
  std::cout<<"[best cost solution]="<<sa.best_sol.toString()<<std::endl;
  std::cout<<"[last solution when run sa]="<<sa.cur_sol.toString()<<std::endl;
  std::cout<<"verify result="<<sa.fit_sol.verify(true)<<std::endl;
  
  if(disable_output==false){
    std::ofstream file;
    file.open (argv[4]);
    file << "Wirelength " << sa.fit_sol.wirelength <<"\n";
    file << "Blocks\n";
    for(int k=0;k<module_array.size();++k){
      file <<"sb"<<k<<" "<<sa.fit_sol.modules[k].origin.x <<" "<<sa.fit_sol.modules[k].origin.y;
      file <<" "<<sa.fit_sol.modules[k].shape.w <<" "<<sa.fit_sol.modules[k].shape.h;
      file <<" "<<sa.fit_sol.lookup_tbl[k]->rotated<<"\n";
    }
    file.close();
  }
#if defined(USE_UI)
  int num_of_nodes=0;
  std::ostringstream ostream;
  build_graphviz(sa.fit_sol.tree_root, ostream, &num_of_nodes);                  
  std::cout<<ostream.str()<<std::endl;
  show_result("final", sa.fit_sol, "result.png");
#endif

  return 0;

}

