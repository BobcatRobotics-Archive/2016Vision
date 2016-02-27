#ifndef IMAGETRACKER_H
#define IMAGETRACKER_H

// Constants (CALIBRATE ME!)
const int X0 = 20*12; // inches
const int P0 = 38;    // pixels

#include <opencv2/opencv.hpp>

class ImageTracker {
public:
  ImageTracker(std::string src, cv::Mat& ref, double aspectThresh);
  ImageTracker(cv::Mat& I, cv::Mat& ref, double aspectThresh);
  ~ImageTracker();
  double getNextOffset();
private:
  cv::Rect findTarget();
  static void log10(std::vector<double>& v);
  static double arrayDelta(std::vector<double>& a, std::vector<double>& b);

  cv::Mat ref;
  cv::VideoCapture cap;
  cv::Mat I;
  std::vector<double> refFeatures;
  double aspectThresh;
  static const double expectedAspect = 5.0/3.0;
  bool isCap;
};

#endif /* IMAGETRACKER_H */
