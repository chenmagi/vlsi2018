#include "parser.hpp"
#include <boost/assert.hpp>
#include <string.h>
#include "algo.hpp"
#include <boost/foreach.hpp>
#include "globalvar.hpp"
#include "sa.hpp"
#include "clock.hpp"
#include <fstream>

namespace fs = boost::filesystem;
#if (0)
int main3(int argc, char **argv){
    boost::shared_ptr<parser_t> parser(new parser_t(argv[1]));
    std::vector<net_t> array;
    global_var_t *global_var = global_var_t::get_ref();
    int net_count=0;
    int count=parser->do_net_file_parse(array);
    
    BOOST_FOREACH(auto e, array){
        std::cout <<e.toString()<< std::endl;
    }
    std::cout<<"net count: "<<array.size()<<std::endl;
    return 0;
}

int main2(int argc, char **argv){
    boost::shared_ptr<parser_t> parser(new parser_t(argv[1]));
    std::vector<terminal_t> array;
    global_var_t *global_var = global_var_t::get_ref();
    int terminal_count=0;
    int count=parser->do_pl_file_parse(array);
    std::cout<<"terminal count"<<count<<std::endl;
    BOOST_FOREACH(auto e, array){
        std::cout <<"p"<<e.id<<"("<<e.coord.x<<", "<<e.coord.y<<")" << std::endl;
    }
    std::cout<<"terminal_array.size()="<<array.size()<<std::endl;
    return 0;
}

int main1(int argc, char **argv){
  boost::shared_ptr<parser_t> parser(new parser_t(argv[1]));
  std::vector<module_t> module_array;
  global_var_t *global_var = global_var_t::get_ref();
  int terminal_count=0;
  int lines=parser->do_block_file_parse(module_array, &terminal_count);
  std::cout<<"lines="<<lines<<std::endl;
  

  shape_t die_shape = calc_for_die_shape(module_array, 0.1);
  std::cout<<"die_shape="<<die_shape.w<<" x "<<die_shape.h<<std::endl;
  global_var->set_die_shape(die_shape);
  global_var->set_placement(PLACEMENT_HARD);


  std::vector<tiny_module_t> sorted_array;
  int rc = build_sorted_module_array(module_array, sorted_array);
  //BOOST_FOREACH(auto e, sorted_array){
  //  std::cout << e.toString() << std::endl;
  //}
  //std::cout<<"sorted_array.size()="<<sorted_array.size()<<std::endl;

  boost::shared_ptr<b_node_t> root(new b_node_t(0));
  build_b_tree(module_array, sorted_array, root);

  std::vector<unsigned int> h_contour, v_contour;
  shape_t result=b_node_t::pack2(root, module_array);


  std::cout<<" first pack result: die shape="<<result.w<<" x "<<result.h<<std::endl;
  bool is_overlap = b_node_t::verify(root, module_array);
  std::cout<<"overlap check result="<<is_overlap<<std::endl;


  int num_of_nodes=0;
  std::ostringstream ostream;
  build_graphviz(root, ostream, &num_of_nodes);
  
  std::cout<<" num of nodes in b-tree: "<< num_of_nodes << std::endl;
  std::cout << ostream.str() <<std::endl;
  for(int i=0;i<module_array.size();++i){
    std::cout<<"sb"<<module_array[i].id<<" = ("<<module_array[i].origin.x <<", "<<module_array[i].origin.y<<")"
    <<std::endl;
  }

  
  
  boost::shared_ptr<b_node_t> copy_root = b_node_t::tree_copy(root);
  num_of_nodes = 0;
  ostream.str("");
  ostream.clear();
  build_graphviz(copy_root, ostream, &num_of_nodes);
  std::cout<<" num of nodes in copy version of b-tree: "<< num_of_nodes << std::endl;
  std::cout << ostream.str() <<std::endl;

  solution_t sol;
  sol.build_from_b_tree(copy_root, module_array.size());
  ostream.str("");
  ostream.clear();
  num_of_nodes=0;
  build_graphviz(sol.tree_root, ostream, &num_of_nodes);
  std::cout<<" num of nodes in solution version of b-tree: "<< num_of_nodes << std::endl;
  std::cout << ostream.str() <<std::endl;

  root->lchild->rotate();
  std::cout<<"Is original version of root roated?" <<root->lchild->rotated<<std::endl;
  std::cout<<"Is copy version of root roated?" <<copy_root->lchild->rotated<<std::endl;


  return 0;

}
#endif

void usage(const char *exec_name){
  std::cout<<"Usage: "<<exec_name<<" .hardblocks .nets .pl .floorplan" <<std::endl;
}
#if (1)
void do_seed_init(const char *dataset){
  boost::filesystem::path p(dataset);
  if(strcmp(p.filename().c_str(), "n100.hardblocks")==0){
    random_t::init(15753);
  }
  else if(strcmp(p.filename().c_str(), "n200.hardblocks")==0)
    random_t::init(26362);
  else if(strcmp(p.filename().c_str(), "n300.hardblocks")==0)
    random_t::init(27991);
  else random_t::init(791233);  
}
#else 
void do_seed_init(const char *value){
  random_t::init(atoi(value));
}
#endif

int main(int argc, char **argv){
  simple_timer_t::get_ref().reset();
  if(argc!=5){
    usage(argv[0]);
    return 0;
  }
  do_seed_init(argv[1]);
  boost::shared_ptr<parser_t> block_parser(new parser_t(argv[1]));
  std::vector<module_t> module_array;
  
  boost::shared_ptr<parser_t> net_parser(new parser_t(argv[2]));
  std::vector<net_t> net_array;

  boost::shared_ptr<parser_t> pin_parser(new parser_t(argv[3]));
  std::vector<terminal_t> pin_array;
  int terminal_count;
  global_var_t *global_var = global_var_t::get_ref();
  shape_t target_die_shape;

  int lines=block_parser->do_block_file_parse(module_array, &terminal_count);
  lines = net_parser->do_net_file_parse(net_array);
  lines = pin_parser->do_pl_file_parse(pin_array);


  target_die_shape = calc_for_die_shape(module_array, 0.1);
  std::cout<<"target die shape="<<target_die_shape.w<<" x "<<target_die_shape.h<<std::endl;
  global_var->set_die_shape(target_die_shape);
  global_var->set_placement(PLACEMENT_HARD);

  simulated_annealing_t sa;
  sa.run(module_array, net_array, pin_array, 60*19);

  std::cout<<"elapsed="<<simple_timer_t::get_ref().elapsed()<<std::endl;
  std::cout<<"final result[0]="<<sa.fit_sol.toString()<<std::endl;
  std::cout<<"final result[1]="<<sa.best_sol.toString()<<std::endl;
  std::cout<<"final result[2]="<<sa.cur_sol.toString()<<std::endl;
  
  std::ofstream file;
  file.open (argv[4]);
  file << "Wirelength " << sa.fit_sol.wirelength <<"\n";
  file << "Blocks\n";
  for(int k=0;k<module_array.size();++k){
    file <<"sb"<<k<<" "<<module_array[k].origin.x <<" "<<module_array[k].origin.y;
    file <<" "<<module_array[k].shape.w <<" "<<module_array[k].shape.h;
    file <<" "<<sa.fit_sol.lookup_tbl[k]->rotated<<"\n";
  }
  file.close();


  return 0;

}

