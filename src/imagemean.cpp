#include <deque>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;

#include "easylogging++.h"
#include "config.hpp"
#include "cvedia.hpp"
#include "api.hpp"
#include "imagemean.hpp"

ImageMean::ImageMean(int mean_mode) {

	LOG(INFO) << "Initializing ImageMean";

	mImageCount = 0;
	mImageWidth = 0;
	mImageHeight = 0;
	mMeanMode = mean_mode;
}

ImageMean::~ImageMean() {

}

float ImageMean::GetGlobalMean() {

	return mMeanGlobal[0] / (mImageCount*3);
}

vector<float> ImageMean::GetChannelMean() {

	vector<float> chan_mean;

 	for (int k=0; k<mMeanChannel.size(); k++) {

		chan_mean.push_back(mMeanChannel[k][0] / mImageCount);
 	}

 	return chan_mean;
}

void ImageMean::AddImage(cv::Mat img) {

	if (mImageCount == 0) {
		mImageWidth = img.size().width;
		mImageHeight = img.size().height;

		mMeanChannel.resize(img.channels());
	}

	if (img.size().width != mImageWidth || img.size().height != mImageHeight || img.channels() != mMeanChannel.size()) {
		LOG(ERROR) << "images must all be of the same dimensions for Mean calculation";
 	}

 	vector<cv::Mat> channels;
 	cv::split(img, channels);

 	for (int k=0; k<img.channels(); k++) {
		cv::Scalar m = cv::mean(channels[k]);
		mMeanChannel[k] += m;
		mMeanGlobal += m;
 	}

	mImageCount++;
}

cv::Mat ImageMean::SubtractMean(cv::Mat img) {

	cv::Mat output;

	if (mMeanMode == MEAN_GLOBAL) {
		cv::subtract(img, mMeanGlobal, output);
	} else if (mMeanMode == MEAN_CHANNEL) {

	}

	return output;
}
