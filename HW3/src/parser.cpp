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
static int parse_line_for_net(std::string &line, net_t &obj){
  const char *buf=line.c_str();
  int len = line.length(); ///< there is no 0x0d/0x0a in line which retrieve by getline()
  if(len==0 || (buf[0]!='p' && buf[0]!='s'))
    return -1;
  int i=0;
  if(buf[0]=='p' && len>=2){
    int value=0;
    for(i=1;i<len;++i){
      value*=10; 
      value+=buf[i]-'0';
    }
    obj.pin_count++;
    obj.pin_id = value;
    BOOST_ASSERT(obj.pin_count==1);
    return 0;
  }
  else if(buf[0]=='s' && len>=3 && buf[1]=='b'){
    int value=0;
    for(i=2;i<len;++i){
      value*=10; 
      value+=buf[i]-'0';
    }
    obj.module_ids.push_back((unsigned int)value);
    return 0;
  }
  return -1;

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

static bool find_duplicate_net(std::vector<net_t> &vec, net_t &obj){
  BOOST_FOREACH(auto e, vec){
    if(e.equal(obj)==true)
      return true;
  }
  return false;
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
/**
 * parser routine for .hardblocks/.softblocks file
 * @return number of lines proceeded
 */
int parser_t::do_block_file_parse(std::vector<module_t> &vec,  int *ptr_terminal_count){
  std::string line;
  int line_idx=0;
  int block_count=0;
  int terminal_count=0;
  int rc;
  module_t obj;
  ///< parse header information first
  while(std::getline(*infile, line)){
    ///< TODO refer to settings in global_var, and implement parser for .softblocks file
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


/**
 * parser routine for .pl file
 * @return number of lines proceeded
 */
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
  /*< NOTE: terminal id starts from 1, 
  insert extra terminal_t in front of vector to easy further access
  */
  if(terminal_count && vec[0].id==1){
    obj.id = 0;
    vec.insert(vec.begin(), obj);
  }
  return line_idx;
}

/**
 * parser routine for .net file
 * @return number of lines proceeded
 */
int parser_t::do_net_file_parse(std::vector<net_t> &vec){
  std::string line;
  int line_idx=0;
  int net_count=0;
  int pin_count=0;
  int degree=0;
  int remains=0;
  int rc;
  net_t obj;
  ///< parse header information first
  while(std::getline(*infile, line)){
    if(line_idx==0){
      rc = parse_line_by_key(line,"NumNets", &net_count);
      BOOST_ASSERT_MSG(rc==0, line.c_str());
    }
    else if(line_idx==1){
      rc = parse_line_by_key(line,"NumPins", &pin_count);
      BOOST_ASSERT_MSG(rc==0, line.c_str());
    }
    else {
      if(remains==0){
        rc = parse_line_by_key(line, "NetDegree", &degree);
        if(rc==0) {
          obj.reset(); ///< start as new
          remains=degree;
          obj.degree = degree;
        }
        else obj.reset();
      }
      else if(remains >0){
        rc = parse_line_for_net(line, obj);
        if(rc==0) remains--;
        if(remains==0){
#ifdef REMOVE_DUP_NET
          if(find_duplicate_net(vec, obj)==false)
#endif
            vec.push_back(obj);
        }

      }

    }
    line_idx++;
    //std::cout<<"line #: "<<line_idx<<std::endl;
  }
#ifndef REMOVE_DUP_NET
  BOOST_ASSERT(vec.size()==net_count);
#endif

  ///< assign id to net
  for(int i=0;i<vec.size();++i){
    vec[i].id=i;
  }


  //std::cout<<"NumHardRectilinearBlocks:"<<block_count<<std::endl;
  //std::cout<<"NumTerminals:"<<terminal_count<<std::endl;
  return line_idx;


}