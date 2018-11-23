#include "parser.hpp"
#include <boost/assert.hpp>
#include <string.h>
#include "algo.hpp"
#include <boost/foreach.hpp>
#include "globalvar.hpp"

namespace fs = boost::filesystem;
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

int main(int argc, char **argv){
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
  shape_t result=b_node_t::pack(root, module_array, h_contour, v_contour);


  std::cout<<" first pack result: die shape="<<result.w<<" x "<<result.h<<std::endl;
  bool is_overlap = b_node_t::verify(root, module_array);
  std::cout<<"overlap check result="<<is_overlap<<std::endl;

  std::cout<<"roate module id 0~29"<<std::endl;
  for(int i=0;i<30;++i){
    if(i%4) continue;
    module_array[20+i].rotate();
  }
  result=b_node_t::pack(root, module_array, h_contour, v_contour);
  std::cout<<" 2nd pack result: die shape="<<result.w<<" x "<<result.h<<std::endl;

  int num_of_nodes=0;
  std::ostringstream ostream;
  build_graphviz(root, ostream, &num_of_nodes);
  std::cout<<" num of nodes in b-tree: "<< num_of_nodes << std::endl;
  std::cout << ostream.str();


  return 0;

}

