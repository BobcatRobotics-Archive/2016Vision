#include "imageTracker.hpp"

ImageTracker::ImageTracker(std::string src, cv::Mat& ref, double aspectThresh) {
  this->ref = ref.clone();
  this->cap = cv::VideoCapture(src);
  if (!this->cap.isOpened()) {
    std::cout << "Unable to open connection to: " << src << std::endl;
    abort();
  }
  this->aspectThresh = aspectThresh;
  this->isCap = true;

  // Convert to grayscale, and precompute reference moments
  cv::cvtColor(this->ref, this->ref, CV_BGR2GRAY);
  cv::HuMoments(cv::moments(this->ref, true), this->refFeatures);
  ImageTracker::log10(this->refFeatures);
}

ImageTracker::ImageTracker(cv::Mat& I, cv::Mat& ref, double aspectThresh) {
  this->ref = ref.clone();
  this->I = I.clone();
  this->aspectThresh = aspectThresh;
  this->isCap = false;

  // Convert to grayscale, and precompute reference moments
  cv::cvtColor(this->ref, this->ref, CV_BGR2GRAY);
  cv::HuMoments(cv::moments(this->ref, true), this->refFeatures);
  ImageTracker::log10(this->refFeatures);
}

ImageTracker::~ImageTracker() {
}

double ImageTracker::getNextOffset() {
  cv::Rect target = ImageTracker::findTarget();
  int targetCenterX = target.x + (target.width / 2);
  int imageCenterX = this->I.cols / 2;
  int deltaX  = targetCenterX - imageCenterX;
  
  double deltaTheta = std::atan(deltaX / ((X0 / (double) P0) * target.height));

  return deltaTheta * (180.0 / M_PI);
}

cv::Rect ImageTracker::findTarget() {
  cv::Mat I;
  if (this->isCap) {
    this->cap >> I;
  } else {
    I = this->I;
  }

  // Only retain pixels where G > 200
  cv::Mat Imask;
  cv::inRange(I, cv::Scalar(0, 200, 0), cv::Scalar(255, 255, 255), Imask);

  // Perform a morphological open with a 5x5 kernel
  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5), cv::Point(2, 2));
  cv::morphologyEx(Imask, Imask, cv::MORPH_OPEN, kernel);

  // Find contours to identify blobs
  std::vector<std::vector<cv::Point> > contours;
  std::vector<cv::Vec4i> hierarchy;
  cv::findContours(Imask, contours, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

  // Loop through each contour to find ones within tolerance for
  // correct Aspect Ratio
  std::vector<cv::Rect> interestingBlobs;
  for (int i = 0; i < contours.size(); ++i) {
    cv::Rect BB = cv::boundingRect(contours[i]);
    double aspectRatio = BB.width / (double) BB.height;

    if (std::abs(aspectRatio - ImageTracker::expectedAspect) <= this->aspectThresh) {
      interestingBlobs.push_back(BB);
    }
  }

  int minIdx = 0;
  double minValue = 100000;
  for (int i = 0; i < interestingBlobs.size(); ++i) {
    cv::Rect BB = interestingBlobs[i];
    cv::Mat Icrop = Imask(BB);

    // Compute the Hu Moments
    std::vector<double> cropFeatures;
    cv::HuMoments(cv::moments(Icrop, true), cropFeatures);
    ImageTracker::log10(cropFeatures);
    double delta = std::abs(ImageTracker::arrayDelta(cropFeatures, this->refFeatures));

    if (delta < minValue) {
      minIdx = i;
      minValue = delta;
    }
  }

  return interestingBlobs[minIdx];
}

void ImageTracker::log10(std::vector<double>& v) {
  for (int i = 0; i < v.size(); ++i) {
    v[i] = std::log10(std::abs(v[i]));
  }
}

double ImageTracker::arrayDelta(std::vector<double>& a, std::vector<double>& b) {
  if (a.size() != b.size()) {
    return 100000; // A Big Number
  }

  double accum = 0;
  for (int i = 0; i < a.size(); ++i) {
      accum += (a[i] - b[i]);
  }

  return accum;
}
