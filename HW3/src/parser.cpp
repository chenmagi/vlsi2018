#include "parser.hpp"
#include <boost/assert.hpp>
#include <string.h>
#include "algo.hpp"
#include <boost/foreach.hpp>
#include "globalvar.hpp"
namespace fs = boost::filesystem;


parser_t::parser_t(const char *filename){
  valid = false;
  std::string s;
  fs::path p(filename);

 /*  TODO check file's exist 
  if(op::exists(p)==false)
    return;
 */
  
  infile = boost::shared_ptr<std::ifstream>(new std::ifstream());
  infile->open(filename);
  valid=true;
}
parser_t::~parser_t(){
  if(infile && valid){
    infile->close();
  }
}

static int parse_line_by_key(std::string &line, const char *key, int *ret_value){
  const char *buf = line.c_str();
  int len = line.length();
  int pat_len=key==NULL?0:strlen(key);
  const char colon=':';
  int i=0;
  bool found=false;

  int value=0;

  if(pat_len+1 >= len)
    return -1;

  if(memcmp(buf, key, pat_len)!=0)
    return -1;
  
  i=pat_len;
  while(i<len){
    char ch = buf[i++];
    if(ch==' '|| ch==':'){
      value=0;
      continue;
    }
    if(ch<='9' && ch>='0'){
      value*=10; value+=ch-'0';
    }
    else {
      return -1;
    }
  }
  *ret_value = value;
  return 0;
}

static int parse_line_for_block(std::string &line, module_t &module){
  const char *buf=line.c_str();
  int len=line.length();
  int i;
  int id=0;
  shape_t *ptr_shape;
  const char *key1="hardrectilinear";
  const char *key2="softrectilinear";
  const int key1_len=15;
  const int key2_len=15;
  const int st_prefix=0;
  const int st_type=1;
  const int st_tail=2;
  int state=-1;
  module_type type = UNKNOWN_MODULE;
  if(line.length()<=3) 
    return -1;
  i=0;

  state=st_prefix;
  while(i<len){
    char ch=state!=st_tail?buf[i++]:' ';
    switch(state){
      case st_prefix:
        if(ch=='s'||ch=='b'){
          id=0; continue;
        }
        else if(ch>='0'&&ch<='9'){
          id*=10; id+=ch-'0';
          continue;
        }
        else if(ch==' '){
          state=st_type;
          continue;
        }
        else if(ch=='p'){
          return -1;
        }
      break;
      case st_type:
        if(len-i+1>=key1_len){
          int cmp=memcmp(&buf[i-1], key1, key1_len);
          if(cmp==0 || memcmp(&buf[i-1], key2, key2_len)==0){
            type = cmp==0?HARD_MODULE:SOFT_MODULE;  
            i+=key1_len-1;
            state=st_tail;
          }
        }
        else {
          BOOST_ASSERT_MSG(len-i+1>=key1_len, buf);
        }
      break;
      case st_tail:
        if(type==HARD_MODULE){
          shape_t new_shape=shape_t::build_from_string(&buf[i]);
          //std::cout<<"new_shape="<<new_shape.w<<", "<<new_shape.h<<std::endl;
          module.shape=new_shape;
          module.type=type;
          module.id=id;
          //std::cout<<"add "<<module.toString()<<std::endl;
          i=len; ///< exit line-parser
        }
        else if(type==SOFT_MODULE){
          ///< TODO implement softmodule parser
          BOOST_ASSERT_MSG(type==HARD_MODULE, buf);
        }
      break;
    }
    
  }
  return 0;
}
int parser_t::do_block_file_parse(std::vector<module_t> &vec,  int *ptr_terminal_count){
  std::string line;
  int line_idx=0;
  int block_count=0;
  int terminal_count=0;
  int rc;
  module_t obj;
  ///< parse header information first
  while(std::getline(*infile, line)){
    if(line_idx==0){
      rc = parse_line_by_key(line,"NumHardRectilinearBlocks", &block_count);
      BOOST_ASSERT_MSG(rc==0, line.c_str());
    }
    else if(line_idx==1){
      rc = parse_line_by_key(line,"NumTerminals", &terminal_count);
      BOOST_ASSERT_MSG(rc==0, line.c_str());
    }
    else {
      rc = parse_line_for_block(line, obj);
      if(rc==0){
        vec.push_back(obj);
      }
    }
    line_idx++;
  }
  BOOST_ASSERT(vec.size()==block_count);
  //std::cout<<"NumHardRectilinearBlocks:"<<block_count<<std::endl;
  //std::cout<<"NumTerminals:"<<terminal_count<<std::endl;
  return line_idx;
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

  int num_of_nodes=0;
  b_node_t::dfs_visit(root, &num_of_nodes);
  std::cout<<" num of nodes in b-tree: "<< num_of_nodes << std::endl;


  return 0;

}

