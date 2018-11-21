#ifndef _PARSER_HPP_
#define _PARSER_HPP_
#include <boost/smart_ptr.hpp>
#include <boost/filesystem/fstream.hpp>
#include "datatypes.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>


struct parser_t {
  parser_t(const char *filename);
  ~parser_t();
  bool valid;

  int do_net_file_parse(std::vector<net_t> &vec);
  int do_pl_file_parse(std::vector<terminal_t> &vec);
  int do_block_file_parse(std::vector<module_t> &vec, int *ptr_terminal_count);
  int build_net_ids_to_module(std::vector<module_t> &module_array, std::vector<net_t> &net_array);  

private:
  boost::shared_ptr<std::ifstream> infile;
  boost::shared_ptr<std::string> fname;
  unsigned int num_of_lines;

};



#endif
