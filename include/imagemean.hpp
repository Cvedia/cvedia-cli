#ifndef _IMAGEMEAN_HPP
#define _IMAGEMEAN_HPP

#include <mutex>

#define MEAN_NONE		0
#define MEAN_GLOBAL		1
#define MEAN_CHANNEL 	2
#define MEAN_PIXEL		3

using namespace std;

#include "api.hpp"

class ImageMean {

public:
	ImageMean(int mean_mode);
	~ImageMean();

	void AddImage(cv::Mat img);
	float GetGlobalMean();
	vector<float> GetChannelMean();
	cv::Mat SubtractMean(cv::Mat img);

private:

	int mImageWidth;
	int mImageHeight;
	int mImageCount;
	int mMeanMode;

	cv::Scalar mMeanGlobal;
	vector<cv::Scalar> mMeanChannel;
	cv::Mat mMeanImg;
};

#endif
