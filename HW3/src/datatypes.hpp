#ifndef _DATATYPES_HPP_
#define _DATATYPES_HPP_
#include <vector>
#include <boost/smart_ptr.hpp>
#include <sstream>

struct coordinate_t {
    unsigned int x;
    unsigned int y;    
    coordinate_t(){
        x=y=0;
    }
    coordinate_t(unsigned int _x, unsigned int _y){
        x=_x;
        y=_y;
    }
    inline coordinate_t& operator=(const coordinate_t& other){
        x = other.x;
        y = other.y;
        return *this;
    }
};

struct net_t {
    unsigned int id; ///< id of net
    std::vector<unsigned int> module_ids; ///< id list of connected modules
    int pin_id; ///< terminal id, -1 when net is not connected to any output pin
    unsigned int degree; ///< number of connections
};

struct terminal_t {
    unsigned int id; ///< id of this terminal pin
    struct coordinate_t coord; ///< xy coordinate
    inline terminal_t& operator=(const terminal_t& other){
        coord = other.coord;
        id = other.id;
        return *this;
    }
};





struct shape_t {
    unsigned int w;
    unsigned int h;
    double ratio; ///< height devide by width
    bool rotated;
    shape_t(){
        w=h=0;
        ratio=0.1;
        rotated=false;
    }
    shape_t(unsigned int _w, unsigned int _h){
        w=_w;
        h=_h;
        rotated=false;
    }
    inline shape_t & operator=(const shape_t& other){
        w = other.w;
        h = other.h;
        rotated = other.rotated;
        return *this;
    }
    inline shape_t& rotate(){
        unsigned int tmp;
        tmp = w;
        w = h;
        h = tmp;
        rotated=rotated==true?false:true; 
        return *this;
    }
    inline unsigned int area(){
        return w*h;
    }
    static shape_t build_from_string(const char *str);
};


struct b_node_t {
    boost::shared_ptr<b_node_t> lchild;
    boost::shared_ptr<b_node_t> rchild;
    boost::shared_ptr<b_node_t> parent;
    unsigned int module_id;

    b_node_t(unsigned int v){
        lchild=rchild=parent=NULL;
        module_id=v;
    }
    inline bool is_root(void){
        return parent==NULL?true:false;
    }
    inline bool is_lchild(void){
        return lchild!=NULL?true:false;
    }

    inline bool is_rchild(void){
        return rchild!=NULL?true:false;
    }

    static void swap(b_node_t *a, b_node_t *b);
    static void move(b_node_t *a, b_node_t *dst_root);
    static void rotate(b_node_t *a);

    static int dfs_visit(boost::shared_ptr<b_node_t> root, int *ret_count);

};


enum module_type {
    SOFT_MODULE=0,
    HARD_MODULE=1,
    UNKNOWN_MODULE=2
};

struct module_t {
    enum module_type type;
    unsigned int id;  ///< id of module
    coordinate_t origin; ///< lower left point
    shape_t shape; 
    
    double aspect_lower; ///< reserved for soft module
    double aspect_upper; ///< reserved for soft module

    boost::shared_ptr<b_node_t> ptr_node;

    module_t(){
        type = UNKNOWN_MODULE;
        id=0;
        origin.x=origin.y=0;
    }

    inline module_t& operator=(const module_t& other){
        type = other.type;
        id = other.id;
        shape = other.shape;
        aspect_lower = other.aspect_lower;
        aspect_upper = other.aspect_upper;
        return *this;
    }

    inline module_t & roate(){
        shape.rotate();
        return *this;
    }

    std::string toString() const {
        std::ostringstream stream;
        stream<<"id: "<<id <<", ";
        stream<<"type: "<<type <<", ";
        stream<<"origin: "<<"("<<origin.x<<", "<<origin.y<<"), ";
        stream<<"shape: "<<"("<<shape.w<<", "<<shape.h<<"), " ;
        stream<<"rotate: "<<shape.rotated;
        return stream.str();
    }

};







#endif
