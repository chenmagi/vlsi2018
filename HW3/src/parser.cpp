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

static int parse_line_for_terminal(std::string &line, terminal_t &obj){
  const char *buf=line.c_str();
  int len = line.length(); ///< there is no 0x0d/0x0a in line which retrieve by getline()
  const int st_hdr=0;
  const int st_xpos=1;
  const int st_ypos=2;
  int state=st_hdr;
  int value;

  if(len==0 || buf[0]!='p')
    return -1;
  int i=0;
  while(i<len){
    char ch = buf[i++];
    switch(state){
      case st_hdr:
        if(ch=='p') { value=0; continue;}
        else if(ch<='9' && ch>='0') { value *=10; value+=ch-'0'; continue;}
        else if(ch==' '||ch=='\t'){obj.id=value; value=0;state=st_xpos; continue;}
      break;
      case st_xpos:
        if(ch<='9' && ch>='0') { value *=10; value+=ch-'0'; continue;}
        else if(ch==' '||ch=='\t'){obj.coord.x=value; value=0;state=st_ypos; continue;}
      break;
      case st_ypos:
        if(ch<='9' && ch>='0') { value *=10; value+=ch-'0'; obj.coord.y=value; continue;}
      break;  
    }
  }
  return 0;

}
int parser_t::do_pl_file_parse(std::vector<terminal_t> &vec){
  std::string line;
  int line_idx=0;
  terminal_t obj;
  
  int terminal_count=0;
  while(std::getline(*infile, line)){
    line_idx++;
    
    int rc = parse_line_for_terminal(line, obj);
    if(rc==0){
      vec.push_back(obj);
      terminal_count++;
    }
  }
  return terminal_count;
}


