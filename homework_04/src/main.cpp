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

    std::vector<NrkStep> steps; 

    const bool load = loadNrkSteps(steps, fileName);

    if (!load) {
        std::cerr << "No steps\n";
        return 1;
    }

    dbg(steps.size());
    dbg(steps);



    // TODO: implement wheel odometry for a 4-wheel differential-drive UGV.
    //
    // Parameters:
    //   ticks_per_revolution = 1024
    //   wheel_radius_m       = 0.3
    //   wheelbase_m          = 1.0
    //
    // Input:  text file with 5 whitespace-separated numbers per line:
    //         timestamp_ms fl_ticks fr_ticks bl_ticks br_ticks
    // Output: same tabular format on stdout, starting from the second sample:
    //         timestamp_ms x y theta

    return 0;
}
