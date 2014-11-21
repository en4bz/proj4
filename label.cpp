#include <string>
#include <vector>
#include <algorithm>

#include <boost/filesystem.hpp>

#include <opencv2/opencv.hpp>

namespace fs = boost::filesystem;

int main(int argc, char* argv[]) {
    if(argv[1] == nullptr){
        std::cerr << "Need Directory!" << std::endl;
        return 2;
    }
    fs::path someDir(argv[1]); // Directory where all the videos are
    fs::directory_iterator end_iter;

    fs::path leftoff;
    if(argv[2] != nullptr){
        leftoff = fs::path(argv[2]); // last video which was labeled. Allows resuming.
    }
    std::vector<fs::path> results;

    // Get filenames for videos in dir
    if(fs::exists(someDir) && fs::is_directory(someDir)) {
        for(fs::directory_iterator dir_iter(someDir) ; dir_iter != end_iter ; ++dir_iter) {
            if(fs::is_regular_file(dir_iter->status()) ) {
              results.emplace_back( *dir_iter);
            }
        }
    }

    bool skip = (argv[2] != nullptr);
    cv::namedWindow("Cars"); // Create a window called 'Cars'
    std::sort(results.begin(), results.end()); 
    for(auto& filename : results) {
        if(skip){
            skip = (filename != leftoff);
            continue;
        }
        std::cout << filename << "  = " << std::flush;
        std::cerr << filename << "  = " << std::flush;
        cv::Mat frame;
        cv::VideoCapture vid(filename.string()); // Open filename
        int z = 0;
        while(vid.read(frame)) { // read the next frame form the video into frame
            cv::imshow("Cars", frame); // display the last video frame into the cars window.
            z = cv::waitKey(1000.0/15.0); // wait for a period of time that makes the video run at 15fps.
            if( z > 0) break; // exit if labeled
        }
        if(z <= 0) {
            z = cv::waitKey(1000*100); // wait for label after showing video
        }
        std::cout << static_cast<char>(z) << std::endl;
        std::cerr << static_cast<char>(z) << std::endl;
    }
    return 0;
}
