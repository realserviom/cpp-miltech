#include <iostream>
#include "types.h"
#include <vector>
#include "functions.h"
#include "dbg.h"


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: ugv_odometry <input_path>\n";
        return 1;
    }

    std::string fileName = argv[1];

    NrkConfig myNrk;

    myNrk.ticks_per_revolution = 1024;
    myNrk.wheel_radius_m = 0.3;
    myNrk.wheelbase_m = 1.0;

    myNrk.updateCalculatedParams();

    std::vector<NrkStep> steps; 

    const bool load = loadNrkSteps(steps, fileName);

    if (!load) {
        std::cerr << "No steps\n";
        return 1;
    }

    std::vector<position> positions(steps.size());
    float x = 0;
    float y = 0;
    double theta = 0; 

    for (size_t i = 1; i < steps.size(); ++i) {
        int timestamp = steps[i].timestamp_ms;

        int d_fl = steps[i].fl_ticks - steps[i-1].fl_ticks;
        int d_fr = steps[i].fr_ticks - steps[i-1].fr_ticks;
        int d_bl = steps[i].bl_ticks - steps[i-1].bl_ticks;
        int d_br = steps[i].br_ticks - steps[i-1].br_ticks;
    
        double dL = ((d_fl + d_bl) / 2.0) * myNrk.distance_per_tick;
        double dR = ((d_fr + d_br) / 2.0) * myNrk.distance_per_tick;
     
        double d = (dL + dR) / 2.0;
        double dtheta = (dR - dL) / myNrk.wheelbase_m;

        x += d * std::cos(theta + dtheta / 2.0);
        y += d * std::sin(theta + dtheta / 2.0);
       
        theta += dtheta;

        // positions[i - 1] = {
        //     .timestamp_ms=timestamp,
        //     .pos={.x=x, .y=y},
        //     .theta=theta
        // };
        
        // dbg(timestamp, x, y, theta);

        // printf("%d %.2lf %.2lf %.2lf\n", timestamp, x, y, theta);

        std::cout << timestamp << " " 
          << std::fixed << std::setprecision(2) 
          << x << " " << y << " " << theta << std::endl;
    }

    // dbg(steps.size());
    // dbg(steps);

    // видаляємо масив кроків
    steps.clear();

    // видаляємо масив позицій
    //positions.clear();

    return 0;
}
