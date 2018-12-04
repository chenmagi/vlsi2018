
#include "view.hpp"
#include "globalvar.hpp"
#include <string>
#include <stdlib.h>
#if defined(USE_UI)
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
void show_result(const char *name,solution_t & solution,  char *saved_fname){
    global_var_t *global_var = global_var_t::get_ref();
    shape_t target_die_shape = global_var->get_die_shape();
    int scaling=2;
    int padding=20;
    cv::Scalar colors[3]={cv::Scalar(255,0,0), cv::Scalar(0,255,0), cv::Scalar(0,0,255)};
    cv::Mat image(target_die_shape.w*scaling+padding, target_die_shape.h*scaling+padding, CV_8UC3, cv::Scalar(255,255,255));
    cv::rectangle(image, cv::Point(0,0),cv::Point(target_die_shape.w*scaling, target_die_shape.h*scaling)
        , cv::Scalar(128,128,128), cv::FILLED, 1);
    for(int k=0;k<solution.modules.size();++k){
        cv::Point pt1(solution.modules[k].origin.x*scaling, solution.modules[k].origin.y*scaling);
        int dx, dy;
        unsigned int cx, cy;
        if(solution.lookup_tbl[k]->rotated){
            dx = solution.modules[k].origin.x + solution.modules[k].shape.h;
            dy = solution.modules[k].origin.y + solution.modules[k].shape.w;
            cx=solution.modules[k].origin.x;
            cy=solution.modules[k].origin.y + solution.modules[k].shape.w/2;
        }
        else {
            dx = solution.modules[k].origin.x + solution.modules[k].shape.w;
            dy = solution.modules[k].origin.y + solution.modules[k].shape.h;
            cx=solution.modules[k].origin.x;
            cy = solution.modules[k].origin.y + solution.modules[k].shape.h/2;

        }
        cv::Point pt2(dx*scaling,dy*scaling);
        if(solution.lookup_tbl[k]->is_as_root())
            cv::rectangle(image, pt1, pt2, colors[0], cv::FILLED, 1);
        else if(solution.lookup_tbl[k]->is_as_lchild())
            cv::rectangle(image, pt1, pt2, colors[1], 1, 1);
        else cv::rectangle(image, pt1, pt2, colors[2], 1, 1);
        std::string label;
        char str[32]={0};
        snprintf(str, sizeof(str),"sb%d", k);
        cv::putText(image, str, cv::Point(cx*scaling, cy*scaling), cv::FONT_HERSHEY_DUPLEX,
         0.5, cv::Scalar(0,0,0), 1);
    }
   

    cv::namedWindow(name);
    cv::imshow(name, image);
    cv::waitKey(0);
    if(saved_fname!=NULL){
        cv::imwrite(saved_fname, image);
    }
    return;


}
#endif

