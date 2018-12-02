#ifndef _VIEW_HPP_
#define _VIEW_HPP_
#include "sa.hpp"
#include "datatypes.hpp"
#include <vector>
#if defined(USE_UI)
void show_result(const char *name,solution_t & solution, std::vector<module_t> &module_array,  char *fname=NULL);
#endif
#endif
