#include "sa.hpp"
#include <queue>
#include <boost/foreach.hpp>
#include "algo.hpp"
#include <iostream>
#include "globalvar.hpp"
#include "view.hpp"
#include <string>
#include <fstream>
double random_t::deviation = 4.0;
double random_t::mean = 1.0;
double simulated_annealing_t::tc_start = 10000.0;
double simulated_annealing_t::tc_end = 25.0;
//double simulated_annealing_t::cooling_factor = 0.99995;
double simulated_annealing_t::cooling_factor = 0.00005;
double solution_t::alpha[] = {100000, 100000};
unsigned random_t::seed=1;


solution_t simulated_annealing_t::get_next_solution(solution_t &cur_sol){
    
    solution_t result;
    result = cur_sol;
    
    random_t::dice_t op=random_t::get_ref().rolling();
    int num_of_modules = cur_sol.modules.size();
    int random_id0 = (rand()% (num_of_modules));
    int random_id1;
    int tries = 4;
    do{
        random_id1 = (rand()% (num_of_modules));
    }while(random_id0!=random_id1 && tries-->0);
    if(random_id0==random_id1){
        random_id1=(random_id0+3)%num_of_modules;
    }

    boost::shared_ptr<b_node_t> node1 = result.lookup_tbl[random_id0];   
    boost::shared_ptr<b_node_t> node2 = result.lookup_tbl[random_id1];   

    
    
    switch(op){
        case random_t::FIRST_DEVIATION: ///< rotate
            //std::cout<<"do swap "<<node1->module_id << " <->" << node2->module_id << std::endl;
            b_node_t::swap(node1, node2);
            result.lookup_tbl[random_id0]=node2;   
            result.lookup_tbl[random_id1]=node1;
        break;
        case random_t::SECOND_DEVIATION: ///< swap
            //std::cout<<"do rotate "<<node1->module_id << std::endl;
            node1->rotate();
        break;
        
        case random_t::THIRD_DEVIATION: ///< move
            if(node1->num_of_children()==1)
                b_node_t::move(node1, node2);
            else {
                b_node_t::swap(node2, node1);
                result.lookup_tbl[random_id0]=node2;   
                result.lookup_tbl[random_id1]=node1;
                node2->rotate();
            }
            
        break;
        
        
    }    
    int ret_count=0;
    
    /*
    int num_of_nodes = 0;
    std::ostringstream ostream;
    build_graphviz(result.tree_root, ostream, &num_of_nodes);
    std::cout<<" num of nodes in copy version of b-tree: "<< num_of_nodes << std::endl;
    std::cout << ostream.str() <<std::endl;
    */
#if defined(MYDEBUG)
    b_node_t::dfs_visit(result.tree_root, &ret_count);
    BOOST_ASSERT(cur_sol.modules.size()==ret_count);
#endif    

    return result;
}
#define REJECTION_LIMIT 400
void simulated_annealing_t::run(std::vector<module_t> & module_array, std::vector<net_t> &net_array, std::vector<terminal_t> &pin_array,int timeout){
    double tc_current = this->tc_start;
    int node_count=0;
    std::ostringstream ostream;
    std::vector<tiny_module_t> sorted_array;
    int rc = build_sorted_module_array(module_array, sorted_array);

    boost::shared_ptr<b_node_t> init_tree(new b_node_t(0));
    build_b_tree(module_array, sorted_array, init_tree);

    //std::vector<unsigned int> h_contour, v_contour;
    shape_t init_shape=b_node_t::pack2(init_tree, module_array);//, h_contour, v_contour);
#if defined(MYDEBUG)    
    std::cout << "init shape="<<init_shape.w <<" x "<<init_shape.h <<std::endl;
#endif    
    
    cur_sol.build_from_b_tree(init_tree, module_array.size());
    cur_sol.modules.assign(module_array.begin(), module_array.end());
    cur_sol.update_cost(net_array, pin_array, 0);
    best_sol = cur_sol;
#if defined(MYDEBUG)    
    std::cout << "initial solution: "<<cur_sol.toString()<<std::endl;
#endif
    global_var_t *global_var = global_var_t::get_ref();
    shape_t target_die_shape = global_var->get_die_shape();
    double target_wirelength = global_var->get_target_wirelength();
    int iteration=0;
    //double level=0.8;
    double level=0.3;
    int sa_loop=0;
    bool found=false;
    fit_sol.wirelength= std::numeric_limits<unsigned int>::max();
    
    do {
        double c_fac=1.0-this->cooling_factor/pow(2,sa_loop);
        tc_current = tc_start/pow(1.414, sa_loop);
        int continuous_reject=0, global_reject=0, op_count=0;
        while(tc_current > this->tc_end && simple_timer_t::get_ref().elapsed() < timeout){
            double old_cost = cur_sol.cost;
            solution_t new_solution = get_next_solution(cur_sol);
            new_solution.update_cost(net_array, pin_array, 0);
            op_count++;
            if(new_solution.die_shape.w <= target_die_shape.w && new_solution.die_shape.h <= target_die_shape.h){
               

#if defined(MYDEBUG)
                 bool legal = new_solution.verify(true);            
                if(legal==false){
                    int num_of_nodes=0;
                    build_graphviz(cur_sol.tree_root, ostream, &num_of_nodes);
                    num_of_nodes=0;
                    build_graphviz(new_solution.tree_root, ostream, &num_of_nodes);
                    std::cout<<ostream.str()<<std::endl;
                    show_result("current", cur_sol, NULL);
                    show_result("new", new_solution, NULL);

                }
                BOOST_ASSERT(legal==true);
#endif
                
                if(fit_sol.wirelength > new_solution.wirelength){
                    found=true;
                    fit_sol = new_solution;
#if defined(MYDEBUG)                    
                    legal = fit_sol.verify(true);
                    if(legal==false){
                        int num_of_nodes=0;
                        build_graphviz(new_solution.tree_root, ostream, &num_of_nodes);
                        num_of_nodes=0;
                        build_graphviz(fit_sol.tree_root, ostream, &num_of_nodes);
                        std::cout<<ostream.str()<<std::endl;
                        std::cout<<"sb16 in fit="<<fit_sol.modules[16].origin.x<<" x "<<fit_sol.modules[16].origin.y<<std::endl;
                        std::cout<<"sb16 in new="<<new_solution.modules[16].origin.x<<" x "<<new_solution.modules[16].origin.y<<std::endl;
                        show_result("before copy", new_solution, NULL);
                        show_result("after copy", fit_sol, NULL);
                    }
                    BOOST_ASSERT(legal==true);

#endif                    
                    if(global_var->timing_limit) break;
                }
            }
            double prob = acceptance(new_solution.cost, old_cost, tc_current);
            tc_current *= c_fac;
            if(prob >= level){
                cur_sol = new_solution;
                if(new_solution.cost < best_sol.cost)
                    best_sol = new_solution;
                continuous_reject=0;
#if defined(MYDEBUG)
                std::cout<<"["<<iteration++<<"]"<<cur_sol.toString() <<", eslaped="<<
                    simple_timer_t::get_ref().elapsed()<<", fit=" << fit_sol.die_shape.w<<" x " <<
                    fit_sol.die_shape.h << ", tc="<<(double)tc_current<<std::endl;
#endif
            }
            else {
                iteration++;
                continuous_reject++;
                global_reject++;
            }
            if(continuous_reject> REJECTION_LIMIT && (double)rand()/(double)RAND_MAX >0.1){
                ///< perturbe again
                continuous_reject=0;
                cur_sol = best_sol;
            }

        }
        sa_loop++;
        if(found==true && ((double)global_reject)/((double)op_count) > 0.7){
            break;
        }
        if(fit_sol.wirelength<=target_wirelength)
            break;
        if(global_var->timing_limit==true && found==true)
            break;
        cur_sol = best_sol;

    }while(simple_timer_t::get_ref().elapsed()<timeout);

    return;


}



void solution_t::build_from_b_tree(boost::shared_ptr<b_node_t> src_root, int num_of_nodes){
    this->tree_root = boost::shared_ptr<b_node_t>(new b_node_t(src_root->module_id));
    std::queue<boost::shared_ptr<b_node_t> > bfs_queue;
    std::queue<boost::shared_ptr<b_node_t> > construct_queue;

    lookup_tbl.clear();
    for(int i=0;i<num_of_nodes;++i){
        boost::shared_ptr<b_node_t> ptr;
        lookup_tbl.push_back(ptr);
    }
    
    bfs_queue.push(src_root);    
    
    construct_queue.push(tree_root);
    tree_root->rotated = src_root->rotated;

    while(!bfs_queue.empty()){
        boost::shared_ptr<b_node_t> sub_root = bfs_queue.front();
        bfs_queue.pop();
        boost::shared_ptr<b_node_t> ptr_current = construct_queue.front();
        construct_queue.pop();
        lookup_tbl[sub_root->module_id]=ptr_current;
        
        boost::shared_ptr<b_node_t> lchild = sub_root->lchild;
        boost::shared_ptr<b_node_t> rchild = sub_root->rchild;
        if(lchild!=NULL){
            bfs_queue.push(lchild);
            ptr_current->lchild = boost::shared_ptr<b_node_t> (new b_node_t(lchild->module_id));
            ptr_current->lchild->parent = ptr_current;
            ptr_current->lchild->rotated = lchild->rotated;
            construct_queue.push(ptr_current->lchild);
        }
        else ptr_current->lchild.reset();
        if(rchild!=NULL){
            bfs_queue.push(rchild);
            ptr_current->rchild = boost::shared_ptr<b_node_t> (new b_node_t(rchild->module_id));
            ptr_current->rchild->parent = ptr_current;
            ptr_current->rchild->rotated = rchild->rotated;
            construct_queue.push(ptr_current->rchild);
        }
        else ptr_current->rchild.reset();
    }    

    BOOST_ASSERT(construct_queue.empty()==true);

    return;
}

static bool integer_cmp( int a,  int b){
    return a < b;
}

void solution_t::update_cost(std::vector<net_t> &net_array, std::vector<terminal_t> &pin_array, int mode){
    std::vector<int> x_array;
    std::vector<int> y_array;
    cost =0;
    double length=0;
    //std::cout<<"u100"<<std::endl;
    global_var_t *global_var = global_var_t::get_ref();
    shape_t target_die_shape = global_var->get_die_shape();
    double target_wirelength = global_var->get_target_wirelength();
    //std::cout<<"u110"<<std::endl;
    //std::vector<unsigned int> h_contour, v_contour;
    die_shape=b_node_t::pack2(tree_root, modules);//, h_contour, v_contour);
    //std::cout<<"u120"<<std::endl;
    for(int k=0;k<net_array.size();++k){
        net_t net = net_array[k];
        //std::cout<<"u121"<<std::endl;
        for(int i=0;i<net.module_ids.size();++i){
            unsigned int id = net.module_ids[i];
            unsigned int cx, cy;
            bool is_rotated=this->lookup_tbl[id]->rotated;
            if(is_rotated)
                modules[id].get_rotated_centric(&cx, &cy);
            else modules[id].get_centric(&cx, &cy);
            x_array.push_back((int)cx);
            y_array.push_back((int)cy);
        }
        if(net.pin_count==1){
            unsigned int px, py;
            px = pin_array[net.pin_id].coord.x;
            py = pin_array[net.pin_id].coord.y;
            x_array.push_back((int)px);
            y_array.push_back((int)py);
        }
        sort(x_array.begin(), x_array.end(), integer_cmp);
        sort(y_array.begin(), y_array.end(), integer_cmp);
        int dx=x_array[x_array.size()-1]-x_array[0];
        int dy=y_array[y_array.size()-1]-y_array[0];
        length += dx;
        length += dy;
        x_array.clear(); y_array.clear();
    }    
    wirelength = length;
    double balance=die_shape.w<die_shape.h? (double)die_shape.h/(double)die_shape.w:(double)die_shape.w/(double)die_shape.h;
    int dw = (die_shape.w <=target_die_shape.w )
             ?0:abs((int)target_die_shape.w - (int)die_shape.w);
    int dh = (die_shape.h <=target_die_shape.h )
            ?0:abs((int)target_die_shape.h - (int)die_shape.h);
#if (0)
    length *= this->alpha[mode];
    double balance_0 =sqrt(dw*dw+dh*dh);
    cost = die_shape.area() + length +(int)(balance_0*100000.);
#else
    wirelength = length;
    double balance_0 =sqrt(dw*dw+dh*dh);
    cost = 100000.0*balance_0; ///< hinge loss for die size constraint
    cost += (double)length;
#endif
    //std::cout<<"u140"<<std::endl;
    return;

}
static bool contains(unsigned int rect0[4][2], unsigned int rect1[4][2]){
    for(int i=0;i<4;++i){
        int x=rect0[i][0];
        int y=rect0[i][1];
        if(x>rect1[0][0] && x<rect1[2][0])
            if(y>rect1[0][1] && y < rect1[2][1])
                return true;
    }
    return false;
}
bool solution_t::verify(bool flag){
    unsigned int rect0[4][2]={0};
    unsigned int rect1[4][2]={0};
    int overlap_count=0;
    global_var_t *global_var = global_var_t::get_ref();
    shape_t target_die_shape = global_var->get_die_shape();
    BOOST_ASSERT(modules.size()==lookup_tbl.size());
    for(int i=0;i<this->lookup_tbl.size();++i){
        bool rotated_0 = lookup_tbl[i]->rotated;
        modules[i].get_rect(rect0,rotated_0);
        if(flag){
            if(rect0[2][0]>target_die_shape.w || rect0[2][1]>target_die_shape.h){
                std::cout<<"sb"<<i<<" exceeds die size constraint, rotated="<<rotated_0<<std::endl;
                overlap_count++;
            }
        }      
        for(int k=0;k<modules.size();++k){
            if(k==i) continue;
            bool rotated_1 = lookup_tbl[k]->rotated;
            modules[k].get_rect(rect1,rotated_1);
            if(contains(rect0,rect1) || contains(rect1, rect0)){
                overlap_count++;
#if defined(MYDEBUG)
                std::cout<<"sb"<<i<<" overlaps sb"<<k<<std::endl;
                std::cout<<"sb"<<i<<" "<<modules[i].origin.x <<" "<<modules[i].origin.y;
                std::cout<<" "<<modules[i].shape.w <<" "<<modules[i].shape.h;
                std::cout<<" "<<this->lookup_tbl[i]->rotated<<std::endl;
                std::cout<<"sb"<<k<<" "<<modules[k].origin.x <<" "<<modules[k].origin.y;
                std::cout<<" "<<modules[k].shape.w <<" "<<modules[k].shape.h;
                std::cout<<" "<<this->lookup_tbl[k]->rotated<<std::endl;
#endif

            }
        }
    }
    return overlap_count==0?true:false;
}
