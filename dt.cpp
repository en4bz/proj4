#include <string>
#include <vector>
#include <algorithm>

#include <boost/filesystem.hpp>

#include <opencv2/opencv.hpp>

namespace fs = boost::filesystem;

cv::Mat dt(size_t pos, const std::vector<cv::Mat>& seq, int CV_TYPE = CV_32F){

  cv::Mat t_der(seq[pos].size(), CV_TYPE);
  if(pos == 0)
    t_der = (seq[pos+1] - seq[pos]) / 2.0;
  else if(pos == seq.size() -1)
    t_der = (seq[pos] - seq[pos-1]) / 2.0;
  else
    t_der = (seq[pos+1] - seq[pos-1]) / 2.0;
 
  return t_der;
}

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
        cv::VideoCapture vid(filename.string()); // Open filename
        int z = 0;
        cv::Rect ROI(140, 50, 100, 160);
        cv::Mat frame;
        std::vector<cv::Mat> traffic;

        while(vid.read(frame)) { // read the next frame form the video into frame
            frame = frame(ROI);
            cv::cvtColor(frame, frame, CV_BGR2GRAY);
            frame.convertTo(frame, CV_32FC1, 1.0/255.0);
            traffic.push_back(frame.clone());
        }
        for(size_t i = 0; i < traffic.size(); i++){
            cv::Mat deriv = dt(i, traffic);
            cv::normalize(deriv, deriv, 0, 1.0, cv::NORM_MINMAX, CV_32FC1); //normalize on [0,1]
            //cv::GaussianBlur(deriv, deriv, cv::Size(5,5), 3);
            cv::imshow("Cars", deriv); // display the last video frame into the cars window.
            z = cv::waitKey(1000.0/15.0); // wait for a period of time that makes the video run at 15fps.
        }
        if(z <= 0) {
            z = cv::waitKey(1000*100); // wait for label after showing video
        }
        std::cout << static_cast<char>(z) << std::endl;
        std::cerr << static_cast<char>(z) << std::endl;
    }
    return 0;
}
