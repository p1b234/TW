#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QRect>
#include <TW_Concurrent_Buffer.h>
#include <TW_paDIC_cuFFTCC2D.h>
#include <opencv2\opencv.hpp>

using cuFftcc2D = TW::paDIC::cuFFTCC2D;
using cuFftcc2DPtr = std::unique_ptr<cuFftcc2D>;
using ImageBuffer = TW::Concurrent_Buffer<cv::Mat>;
using ImageBufferPtr = std::shared_ptr<ImageBuffer>;

typedef struct 
{
	bool m_isLeftBtnReleased;
	bool m_isRightBtnReleased;
	QRect m_roiBox;
} MouseData;

typedef struct
{
    int averageFPS;
    int nFramesProcessed;
} ThreadStatisticsData;

#endif // STRUCTURES_H