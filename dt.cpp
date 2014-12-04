#include <algorithm>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include <opencv2/opencv.hpp>

struct colon_is_space : std::ctype<char>
{
    colon_is_space() : std::ctype<char>(get_table()) {}

    static mask const* get_table() {
        static mask rc[table_size];
        rc[','] = std::ctype_base::space;
        rc['\n'] = std::ctype_base::space;
        return &rc[0];
    }
};


  
using std::locale;
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
    std::map<std::string,int> labels;
    
    std::fstream fin("labels.csv");
    fin.imbue(locale(fin.getloc(), new colon_is_space));
    while(1){
        int label = 5;
        std::string temp;
        fin >> temp >> label;
        if(!fin.good()) break; 
        temp.pop_back();
        temp.pop_back();
        temp.pop_back();
        temp.pop_back();
        labels[temp] = label;
        std::cout << temp << " --> " << label << std::endl; 
    }

    // Get filenames for videos in dir
    
    if(fs::exists(someDir) && fs::is_directory(someDir)) {
        for(fs::directory_iterator dir_iter(someDir) ; dir_iter != end_iter ; ++dir_iter) {
            if(fs::is_regular_file(dir_iter->status()) ) {
              results.emplace_back(*dir_iter);
            }
        }
    }

    std::ofstream fout("./manifest.info");

    bool skip = (argv[2] != nullptr);
    cv::namedWindow("Cars"); // Create a window called 'Cars'
    std::sort(results.begin(), results.end()); 
    const cv::Rect ROI(150, 50, 80, 165); 
    for(auto& filename : results) {
        if(skip){
            skip = (filename != leftoff);
            continue;
        }
        cv::VideoCapture vid(filename.string()); // Open filename
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
            double min, max;
            cv::minMaxIdx(deriv, &min, &max);
            deriv = (deriv - min);
            //cv::threshold(deriv, deriv, 255, 255, cv::THRESH_TRUNC);
            cv::minMaxIdx(deriv, &min, &max);
            // std::cout << max << std::endl;
            // cv::normalize(deriv, deriv, 0, 255, cv::NORM_MINMAX, CV_8UC1); //normalize on [0,1]
            // cv::threshold(deriv, deriv, 0.75, 1.0, cv::THRESH_TOZERO);
            // cv::imshow("Cars", deriv); // display the last video frame into the cars window.
            // cv::waitKey(50);
            std::string outfile("./all/");
            const std::string basename = fs::basename(filename);
            const std::string dirless = basename + ":" + std::to_string(i) + ".png";
            outfile += dirless;
            int label = 0;
            if( labels.find(basename) == labels.end())
                std::cout << "Error" << std::endl;
            else {
                label = labels[basename];
                if(max < 0.21 && label < 4) label = 0;
                std::cout << outfile << " " << label << std::endl;
            }
            bool ret = cv::imwrite(outfile, 255.0 * deriv);
            if(!ret)
                std::cout << outfile << std::endl;
            else
                fout << dirless << ' ' << label << std::endl;
            //z = cv::waitKey(1000.0/15.0); // wait for a period of time that makes the video run at 15fps.
        }
    }
    return 0;
}
