#ifndef _DATATYPES_HPP_
#define _DATATYPES_HPP_
#include <vector>
#include <boost/smart_ptr.hpp>
#include <sstream>
#include <math.h>

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
    inline coordinate_t& set(unsigned int _x, unsigned int _y){
        x=_x;
        y=_y;
        return *this;
    }
};

struct net_t {
    unsigned int id; ///< id of net
    std::vector<unsigned int> module_ids; ///< id list of connected modules
    int pin_id; ///< terminal id, -1 when net is not connected to any output pin
    int pin_count;
    unsigned int degree; ///< number of connections
    static const int INVALID_PIN=-1;

    net_t(){
        id = 0;
        pin_id=INVALID_PIN;
        pin_count=0;
    }
    inline net_t& operator=(const net_t& other){
        id = other.id;
        pin_id = other.pin_id;
        pin_count= other.pin_count;
        degree = other.degree;
        module_ids = other.module_ids;
        return *this;
    }
#if (0)
    bool equal(const net_t &other){
        if(pin_id!=other.pin_id)
            return false;
        if(module_ids.size()!=other.module_ids.size())
            return false;
        for(int i=0;i<module_ids.size();++i){
            if(module_ids[i]!=other.module_ids[i])
                return false;
        }    
        return true;
    }
#endif    


    void reset(void){
        module_ids.clear();
        pin_count=0;
        pin_id=INVALID_PIN;
        id=0;
    }

    std::string toString() const {
        std::ostringstream stream;
        stream<<"id: "<<id <<", modules(";
        for(int i=0;i<module_ids.size();++i){
            stream<<"sb"<<module_ids[i];
            if(i!=module_ids.size()-1)
                stream<<", ";
            else stream<<"), ";
        }
        stream<<"pin: "<<pin_id ;
        stream<<", degree: "<<degree;
        return stream.str();

    }
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

struct module_t;
struct b_node_t {
    boost::shared_ptr<b_node_t> lchild;
    boost::shared_ptr<b_node_t> rchild;
    boost::shared_ptr<b_node_t> parent;
    unsigned int module_id;
    bool rotated;

    b_node_t(unsigned int v){
        lchild=rchild=parent=NULL;
        module_id=v;
        rotated=false;
    }
    
    inline bool is_as_root(void){
        return parent==NULL?true:false;
    }
    inline bool is_as_lchild(void){
        if((parent->lchild!=NULL) && parent->lchild->module_id==module_id)
            return true;
        else return false;
    }

    inline bool is_as_rchild(void){
        if((parent->rchild!=NULL) && parent->rchild->module_id==module_id)
            return true;
        else return false;
    }
    inline int num_of_children(void){
        int num=0;
        if(rchild!=NULL) num++;
        if(lchild!=NULL) num++;
        return num;
    }
    inline void rotate(void){
        rotated = rotated==true?false:true;
    }

    inline b_node_t& operator=(const b_node_t& other){
        lchild = other.lchild;
        rchild = other.rchild;
        parent = other.parent;
        module_id = other.module_id;
        rotated = other.rotated;
        return *this;
    }

    static void swap(boost::shared_ptr<b_node_t> a, boost::shared_ptr<b_node_t> b);
    static void move(boost::shared_ptr<b_node_t> a, boost::shared_ptr<b_node_t> dst_root);
    static void rotate(std::vector<module_t> &module_array,boost::shared_ptr<b_node_t> a);
    static shape_t pack(boost::shared_ptr<b_node_t>root,std::vector<module_t> & module_array, std::vector<unsigned int> &horz_contour,std::vector<unsigned int>&vert_contour);

    static int dfs_visit(boost::shared_ptr<b_node_t> root, int *ret_count);
    static bool verify(boost::shared_ptr<b_node_t>root,std::vector<module_t> & module_array);

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
    std::vector<unsigned int> net_ids; ///< net id array

    boost::shared_ptr<b_node_t> ptr_node; ///< pointer to node in b*-tree

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
        net_ids = other.net_ids;
        return *this;
    }

    inline module_t & rotate(){
        shape.rotate();
        return *this;
    }

    inline bool is_rotated(){
        return shape.rotated;
    }
    inline void get_rect(unsigned int rect[4][2]){
        unsigned int x = origin.x;
        unsigned int y = origin.y;
        unsigned int w = shape.w;
        unsigned int h = shape.h;
        unsigned int tmp[4][2]={{x,y}, {x, y+h}, {x+w, y+h}, {x+w, y}};
        memcpy(rect, tmp, sizeof(sizeof(unsigned int)*8));
    }

    ///< HPWL calculation for module-to-terminal and module-to-module
    inline int wirelength_to(terminal_t &t){
        int cx = origin.x + shape.w/2;
        int cy = origin.y + shape.h/2;
        return abs(cx-(int)t.coord.x)+abs(cy-(int)t.coord.y);
    }

    inline int wirelength_to(module_t &t){
        int cx = origin.x + shape.w/2;
        int cy = origin.y + shape.h/2;
        
        int t_cx = t.origin.x+t.shape.w/2;
        int t_cy = t.origin.y+t.shape.h/2;
        return abs(cx-t_cx)+abs(cy-t_cy);
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
