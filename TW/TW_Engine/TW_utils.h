#ifndef TW_UTILS_H
#define	TW_UTILS_H

#include "TW.h"

#include <opencv2\opencv.hpp>
#include <cuda_runtime.h>
#include <cmath>

namespace TW
{
//!--------------------Inline and template functions----------------------------------------
template<typename T>
void deleteObject(T*& param)
{
	if (param != nullptr && param != 0 && param != NULL)
	{
		delete param;
		param = nullptr;
	}
}
/// \brief Return the 1D index of a 2D matrix.
///
/// \param x position in x direction in a 2D matrix
/// \param y position in y direction in a 2D matrix
/// \param width width (number of cols) of a 2D matrix
inline __host__ __device__ int_t ELT2D(int_t x, int_t y, int_t width)
{
	return (y*width + x);
}

/// \brief Return the 1D index of a 3D matrix.
///
/// \param x position in x direction in a 3D matrix
/// \param y position in y direction in a 3D matrix
/// \param z position in z direction in a 3D matrix
/// \param width width of a 3D matrix
/// \param height height of a 3D matrix
inline __host__ __device__ int_t ELT3D(int_t x, int_t y, int_t z, int_t width, int_t height)
{
	return ((z*height + y)*width + x);
}

/// \brief lerp function for t
///
/// \param a lower bound 
/// \param b upper bound
/// \param lerp parameter
inline __device__ __host__ real_t lerp(real_t a, real_t b, real_t t)
{
	return a + t*(b - a);
}

/// \brief clamp function
///
/// \param a lower bound
/// \param b upper bound
/// \param f value to be clamped
inline __device__ __host__ real_t clamp(real_t f, real_t a, real_t b)
{
#ifdef TW_USE_DOUBLE
	return fmax(a, fmin(f, b));
#else
	return fmaxf(a, fminf(f, b));
#endif // TW_USE_DOUBL
}

/// \brief Template function for computing a block-sum reduction
///
/// \param sdata data in the block's shared memory
/// \param mySum the value to be added on 
/// \param tid thread id
template <unsigned int blockSize, class Real>
__device__
void reduceBlock(
volatile Real *sdata, Real mySum, const unsigned int tid)
{
	sdata[tid] = mySum;
	__syncthreads();

	// do reduction in shared mem
	if (blockSize >= 512){ if (tid < 256) { sdata[tid] = mySum = mySum + sdata[tid + 256]; } __syncthreads(); }
	if (blockSize >= 256){ if (tid < 128) { sdata[tid] = mySum = mySum + sdata[tid + 128]; } __syncthreads(); }
	if (blockSize >= 128){ if (tid < 64) { sdata[tid] = mySum = mySum + sdata[tid + 64]; } __syncthreads(); }

	if (tid < 32)
	{
		if (blockSize >= 64){ sdata[tid] = mySum = mySum + sdata[tid + 32]; }
		if (blockSize >= 32){ sdata[tid] = mySum = mySum + sdata[tid + 16]; }
		if (blockSize >= 16){ sdata[tid] = mySum = mySum + sdata[tid + 8]; }
		if (blockSize >= 8)	{ sdata[tid] = mySum = mySum + sdata[tid + 4]; }
		if (blockSize >= 4)	{ sdata[tid] = mySum = mySum + sdata[tid + 2]; }
		if (blockSize >= 2) { sdata[tid] = mySum = mySum + sdata[tid + 1]; }
	}
	__syncthreads();
}

/// \brief Template function for computing a block-max reduction
///
/// \param sdata data in the block's shared memory
/// \param sindex index of the corresponding data in shared memory
/// \param data initial data
/// \param ind initial index
/// \param tid thread id
template <unsigned int blockSize, class Real>
__device__
void reduceToMaxBlock(
volatile Real *sdata, volatile int *sindex,
Real data, int ind, const unsigned int tid)
{

	// do reduction in shared mem
	sdata[tid] = data;
	sindex[tid] = ind;
	__syncthreads();

	if (blockSize >= 512){ if (tid < 256){ if (sdata[tid] < sdata[tid + 256]){ sdata[tid] = sdata[tid + 256]; sindex[tid] = sindex[tid + 256]; } } __syncthreads(); }
	if (blockSize >= 256){ if (tid < 128){ if (sdata[tid] < sdata[tid + 128]){ sdata[tid] = sdata[tid + 128]; sindex[tid] = sindex[tid + 128]; } } __syncthreads(); }
	if (blockSize >= 128){ if (tid < 64) { if (sdata[tid] < sdata[tid + 64]){ sdata[tid] = sdata[tid + 64]; sindex[tid] = sindex[tid + 64]; } } __syncthreads(); }

	if (tid < 32)
	{
		if (blockSize >= 64) { if (sdata[tid] < sdata[tid + 32]){ sdata[tid] = sdata[tid + 32]; sindex[tid] = sindex[tid + 32]; } }
		if (blockSize >= 32) { if (sdata[tid] < sdata[tid + 16]){ sdata[tid] = sdata[tid + 16]; sindex[tid] = sindex[tid + 16]; } }
		if (blockSize >= 16) { if (sdata[tid] < sdata[tid + 8]){ sdata[tid] = sdata[tid + 8]; sindex[tid] = sindex[tid + 8]; } }
		if (blockSize >= 8) { if (sdata[tid] < sdata[tid + 4]){ sdata[tid] = sdata[tid + 4]; sindex[tid] = sindex[tid + 4]; } }
		if (blockSize >= 4)	{ if (sdata[tid] < sdata[tid + 2]){ sdata[tid] = sdata[tid + 2]; sindex[tid] = sindex[tid + 2]; } }
		if (blockSize >= 2) { if (sdata[tid] < sdata[tid + 1]){ sdata[tid] = sdata[tid + 1]; sindex[tid] = sindex[tid + 1]; } }
	}
	__syncthreads();
}

/// \brief Function for computing the CUFFT complex number scaling operation
/// 
/// \param a the input complex number
/// \param s the scaling factor
/// \return the scaled new complex number
static __device__ __host__
inline cudafftComplex ComplexScale(cudafftComplex a, real_t s)
{
	cudafftComplex c;
	c.x = s * a.x;
	c.y = s * a.y;
	return c;
}

/// \brief Function for computing the CUFFT complex number multiplication
/// 
/// \param a the input complex number
/// \param b another input complex number
/// \return the result of the multiplication
static __device__ __host__
inline cudafftComplex ComplexMul(cudafftComplex a, cudafftComplex b)
{
	cudafftComplex c;
	c.x = a.x * b.x + a.y * b.y;
	c.y = a.x * b.y - a.y * b.x;
	return c;
}

// ----------------------Inline and Template Functions End-------------------------!


// !---------------------Host Utility Functions -------------------------------------

/// \brief Function to compute positions of POIs on single-core CPU.
/// No need to pre-allocate memory for the pointer.
///
/// \param Out_h_iPXY position array in device memory
/// \param iNumberX number of POIs in x direction
/// \param iNumberY number of POIs in y direction
/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
/// \param iSubsetX half size of the square subset in x direction
/// \param iSubsetY half size of the square subset in y direction
/// \param iGridSpaceX number of pixels between two POIs in x direction
/// \param iGirdSpaceY number of pixels between two POIs in y direction
TW_LIB_DLL_EXPORTS void ComputePOIPositions_s(// Output
											  int_t ***&Out_h_iPXY,			// Return the host handle
											  // Inputs
											  int_t iNumberX, int_t iNumberY,
											  int_t iMarginX, int_t iMarginY,
											  int_t iSubsetX, int_t iSubsetY,
											  int_t iGridSpaceX, int_t iGridSpaceY);

/// \brief Function to compute positions of POIs on single-core CPU.
/// No need to pre-allocate memory for the pointer.
///
/// \param Out_h_iPXY position array in device memory
/// \param iStartX, iStartY The start position of the ROI
/// \param iNumberX number of POIs in x direction
/// \param iNumberY number of POIs in y direction
/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
/// \param iSubsetX half size of the square subset in x direction
/// \param iSubsetY half size of the square subset in y direction
/// \param iGridSpaceX number of pixels between two POIs in x direction
/// \param iGirdSpaceY number of pixels between two POIs in y direction
TW_LIB_DLL_EXPORTS void ComputePOIPositions_s(// Output
											  int_t ***&Out_h_iPXY,			// Return the host handle
											  // Inputs
											  int_t iStartX, int_t iStartY,
											  int_t iNumberX, int_t iNumberY,
											  int_t iMarginX, int_t iMarginY,
											  int_t iSubsetX, int_t iSubsetY,
											  int_t iGridSpaceX, int_t iGridSpaceY);

/// \brief Function to compute positions of POIs on multi-core CPU.
/// No need to pre-allocate memory for the pointer.
///
/// \param Out_h_iPXY position array in device memory
/// \param iNumberX number of POIs in x direction
/// \param iNumberY number of POIs in y direction
/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
/// \param iSubsetX half size of the square subset in x direction
/// \param iSubsetY half size of the square subset in y direction
/// \param iGridSpaceX number of pixels between two POIs in x direction
/// \param iGirdSpaceY number of pixels between two POIs in y direction
TW_LIB_DLL_EXPORTS void ComputePOIPositions_m(// Output
											  int_t ***&Out_h_iPXY,			// Return the host handle
											  // Inputs
											  int_t iNumberX, int_t iNumberY,
											  int_t iMarginX, int_t iMarginY,
											  int_t iSubsetX, int_t iSubsetY,
											  int_t iGridSpaceX, int_t iGridSpaceY);


/// \brief Function to compute positions of POIs on Multi-core CPU.
/// No need to pre-allocate memory for the pointer.
///
/// \param Out_h_iPXY position array in device memory
/// \param iStartX, iStartY The start position of the ROI
/// \param iNumberX number of POIs in x direction
/// \param iNumberY number of POIs in y direction
/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
/// \param iSubsetX half size of the square subset in x direction
/// \param iSubsetY half size of the square subset in y direction
/// \param iGridSpaceX number of pixels between two POIs in x direction
/// \param iGirdSpaceY number of pixels between two POIs in y direction
TW_LIB_DLL_EXPORTS void ComputePOIPositions_m(// Output
											  int_t ***&Out_h_iPXY,			// Return the host handle
											  // Inputs
											  int_t iStartX, int_t iStartY,
											  int_t iNumberX, int_t iNumberY,
											  int_t iMarginX, int_t iMarginY,
											  int_t iSubsetX, int_t iSubsetY,
											  int_t iGridSpaceX, int_t iGridSpaceY);

enum class AccuracyOrder
{	
	Quadratic,
	Quartic,
	Octic
};

/// \brief Sequential Function to compute the gradients x&y of an image using Central Difference Scheme
/// Note: The cv::Mat must be 8UC1 format, otherwise this method cannot be called
///
/// \param image cv::Mat image used to calculate the gradients
/// \param iStartX, iStartY The start positions of the ROI
/// \param iROIWidth, iROIHeight The width&height of the ROI
/// \param iImgWidth, iImgHeight THe width&height of the image
/// \param accuracyOrder The accuracy order of the gradients (Taylor's series)
/// \param Gx gradient in x direction, Gx = nullptr if no need to calculate it
/// \param Gy gradient in y direction, Gy = nullptr if no need to calculate it
TW_LIB_DLL_EXPORTS void Gradient_s(//Inputs
								   const cv::Mat& image,
								   int_t iStartX, int_t iStartY,
								   int_t iROIWidth, int_t iROIHeight,
								   int_t iImgWidth, int_t iImgHeight,
								   AccuracyOrder accuracyOrder,
								   //Output
								   real_t **Gx,
								   real_t **Gy);

/// \brief Sequential Function to compute the gradients x&y of an image using Central Difference Scheme
/// Note: The cv::Mat must be 8UC1 format, otherwise this method cannot be called
///
/// \param image cv::Mat image used to calculate the gradients
/// \param iStartX, iStartY The start positions of the ROI
/// \param iROIWidth, iROIHeight The width&height of the ROI
/// \param iImgWidth, iImgHeight THe width&height of the image
/// \param accuracyOrder The accuracy order of the gradients (Taylor's series)
/// \param Gx gradient in x direction, Gx = nullptr if no need to calculate it
/// \param Gy gradient in y direction, Gy = nullptr if no need to calculate it
/// \param Gxy gradient in xy direction, Gxy = nullptr if no need to calculate it
TW_LIB_DLL_EXPORTS void GradientXY_s(//Inputs
								     const cv::Mat& image,
								     int_t iStartX, int_t iStartY,
								     int_t iROIWidth, int_t iROIHeight,
								     int_t iImgWidth, int_t iImgHeight,
								     AccuracyOrder accuracyOrder,
								     //Output
								     real_t **Gx,
								     real_t **Gy,
									 real_t **Gxy);

TW_LIB_DLL_EXPORTS void GradientXY_2Images_s(//Inputs
											 const cv::Mat& image1,
											 const cv::Mat& image2,
											 int_t iStartX, int_t iStartY,
											 int_t iROIWidth, int_t iROIHeight,
											 int_t iImgWidth, int_t iImgHeight,
											 AccuracyOrder accuracyOrder,
											 //Outputs
											 real_t **Fx,
											 real_t **Fy,
											 real_t **Gx,
											 real_t **Gy,
											 real_t **Gxy);

/// \brief Multi-threaded Function to compute the gradients x&y of an image using Central Difference Scheme
/// Note: The cv::Mat must be 8UC1 format, otherwise this method cannot be called
///
/// \param image cv::Mat image used to calculate the gradients
/// \param iStartX, iStartY The start positions of the ROI
/// \param iROIWidth, iROIHeight The width&height of the ROI
/// \param iImgWidth, iImgHeight THe width&height of the image
/// \param accuracyOrder The accuracy order of the gradients (Taylor's series)
/// \param Gx gradient in x direction, Gx = nullptr if no need to calculate it
/// \param Gy gradient in y direction, Gy = nullptr if no need to calculate it
TW_LIB_DLL_EXPORTS void Gradient_m(//Inputs
								   const cv::Mat& image,
								   int_t iStartX, int_t iStartY,
								   int_t iROIWidth, int_t iROIHeight,
								   int_t iImgWidth, int_t iImgHeight,
								   AccuracyOrder accuracyOrder,
								   //Output
								   real_t **Gx,
								   real_t **Gy);

/// \brief Multi-core Function to compute the gradients x&y of an image using Central Difference Scheme
/// Note: The cv::Mat must be 8UC1 format, otherwise this method cannot be called
///
/// \param image cv::Mat image used to calculate the gradients
/// \param iStartX, iStartY The start positions of the ROI
/// \param iROIWidth, iROIHeight The width&height of the ROI
/// \param iImgWidth, iImgHeight THe width&height of the image
/// \param accuracyOrder The accuracy order of the gradients (Taylor's series)
/// \param Gx gradient in x direction, Gx = nullptr if no need to calculate it
/// \param Gy gradient in y direction, Gy = nullptr if no need to calculate it
/// \param Gxy gradient in xy direction, Gxy = nullptr if no need to calculate it
TW_LIB_DLL_EXPORTS void GradientXY_m(//Inputs
								     const cv::Mat& image,
								     int_t iStartX, int_t iStartY,
								     int_t iROIWidth, int_t iROIHeight,
								     int_t iImgWidth, int_t iImgHeight,
								     AccuracyOrder accuracyOrder,
								     //Output
								     real_t **Gx,
								     real_t **Gy,
									 real_t **Gxy);

TW_LIB_DLL_EXPORTS void GradientXY_2Images_m(//Inputs
											 const cv::Mat& image1,
											 const cv::Mat& image2,
											 int_t iStartX, int_t iStartY,
											 int_t iROIWidth, int_t iROIHeight,
											 int_t iImgWidth, int_t iImgHeight,
											 AccuracyOrder accuracyOrder,
											 //Outputs
											 real_t **Fx,
											 real_t **Fy,
											 real_t **Gx,
											 real_t **Gy,
											 real_t **Gxy);

/// \brief Sequential function to precompute the Bicubic B-spline interpolation coefficients LUT.
/// NOTE: The control points are assumed to be on the B-spline surface
/// Reference: 刘洪臣, et al. (2007). "基于双三次B样条曲面亚像元图像插值方法." 哈尔滨工业大学学报 39(7): 1121-1124.
///
/// \param image cv::Mat image
/// \param iStartX, iStartY The start positions of the ROI
/// \param iROIWidth, iROIHeight The width&height of the ROI
/// \param iImgWidth, iImgHeight THe width&height of the image
/// \param fBSpline Bspline LUT
TW_LIB_DLL_EXPORTS void BicubicSplineCoefficients_s(// Inputs
												    const cv::Mat& image,
												    int_t iStartX, int_t iStartY,
												    int_t iROIWidth, int_t iROIHeight,
												    int_t iImgWidth, int_t iImgHeight,
												    // Output
												    real_t ****fBSpline);

/// \brief Sequential function to precompute Bicubic interpolation coefficients LUT
/// 
/// \param image cv::Mat image
/// \param Tx, Ty, Txy Gradients of the image in x, y&xy directions
/// \param iStartX, iStartY The start positions of the ROI
/// \param iROIWidth, iROIHeight The width&height of the ROI
/// \param iImgWidth, iImgHeight THe width&height of the image
/// \param fBicubic Bspline LUT
TW_LIB_DLL_EXPORTS void BicubicCoefficients_s(// Inputs
											  const cv::Mat& image,
											  real_t **Tx,
											  real_t **Ty,
											  real_t **Txy,
											  int_t iStartX, int_t iStartY,
											  int_t iROIWidth, int_t iROIHeight,
											  int_t iImgWidth, int_t iImgHeight,
											  // Outputs
											  real_t ****fBicubic);

/// \brief Multi-core function to precompute the Bicubic B-spline interpolation coefficients LUT.
/// NOTE: The control points are assumed to be on the B-spline surface
/// Reference: 刘洪臣, et al. (2007). "基于双三次B样条曲面亚像元图像插值方法." 哈尔滨工业大学学报 39(7): 1121-1124.
///
/// \param image cv::Mat image
/// \param iStartX, iStartY The start positions of the ROI
/// \param iROIWidth, iROIHeight The width&height of the ROI
/// \param iImgWidth, iImgHeight THe width&height of the image
/// \param fBSpline Bspline LUT
/// OpenMP is used for the parallelization
TW_LIB_DLL_EXPORTS void BicubicSplineCoefficients_m(// Inputs
												    const cv::Mat& image,
												    int_t iStartX, int_t iStartY,
												    int_t iROIWidth, int_t iROIHeight,
												    int_t iImgWidth, int_t iImgHeight,
												    // Output
												    real_t ****fBSpline);

/// \brief Multi-core function to precompute Bicubic interpolation coefficients LUT
/// 
/// \param image cv::Mat image
/// \param Tx, Ty, Txy Gradients of the image in x, y&xy directions
/// \param iStartX, iStartY The start positions of the ROI
/// \param iROIWidth, iROIHeight The width&height of the ROI
/// \param iImgWidth, iImgHeight THe width&height of the image
/// \param fBicubic Bspline LUT
/// OpenMP is used for parallelization.
TW_LIB_DLL_EXPORTS void BicubicCoefficients_m(// Inputs
											  const cv::Mat& image,
											  real_t **Tx,
											  real_t **Ty,
											  real_t **Txy,
											  int_t iStartX, int_t iStartY,
											  int_t iROIWidth, int_t iROIHeight,
											  int_t iImgWidth, int_t iImgHeight,
											  // Outputs
											  real_t ****fBicubic);

// ---------------------------CPU Utility Functions End-------------------------------!

// !----------------------------GPU Wrapper Functions----------------------------------

/// \brief Function to compute positions of POIs on GPU within the ROI, the result is stored in 
/// an device array. NOTE: No need to pre-allocate memory for the device pointer
///
/// \param Out_d_iPXY position array in device memory
/// \param iNumberX number of POIs in x direction
/// \param iNumberY number of POIs in y direction
/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
/// \param iSubsetX half size of the square subset in x direction
/// \param iSubsetY half size of the square subset in y direction
/// \param iGridSpaceX number of pixels between two POIs in x direction
/// \param iGirdSpaceY number of pixels between two POIs in y direction
TW_LIB_DLL_EXPORTS void cuComputePOIPositions(// Output
										 	 int_t *&Out_d_iPXY,			// Return the device handle
											 // Inputs
											 int_t iNumberX, int_t iNumberY,
											 int_t iMarginX, int_t iMarginY,
											 int_t iSubsetX, int_t iSubsetY,
											 int_t iGridSpaceX, int_t iGridSpaceY);

/// \brief Function to compute positions of POIs on GPU within the ROI, the result is stored both in 
/// an device array and a host array. NOTE: No need to pre-allocate memory for the two pointers.
///
/// \param Out_d_iPXY position array in device memory
/// \param Out_h_iPXY position array in host memory
/// \param iNumberX number of POIs in x direction
/// \param iNumberY number of POIs in y direction
/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
/// \param iSubsetX half size of the square subset in x direction
/// \param iSubsetY half size of the square subset in y direction
/// \param iGridSpaceX number of pixels between two POIs in x direction
/// \param iGirdSpaceY number of pixels between two POIs in y direction
TW_LIB_DLL_EXPORTS void cuComputePOIPositions(// Outputs
											 int_t *&Out_d_iPXY,			// Return the device handle
											 int_t ***&Out_h_iPXY,			// Retrun the host handle
											 // Inputs
											 int_t iNumberX, int_t iNumberY,
											 int_t iMarginX, int_t iMarginY,
											 int_t iSubsetX, int_t iSubsetY,
											 int_t iGridSpaceX, int_t iGridSpaceY);

/// \brief Function to compute positions of POIs on GPU within the whole image, the result is stored both in 
/// an device array and a host array. NOTE: No need to pre-allocate memory for the two pointers.
///
/// \param Out_d_iPXY position array in device memory
/// \param Out_h_iPXY position array in host memory
/// \param iStartX x coordinate of the top-left point of ROI
/// \param iStartY y coordinate of the top-left point of ROI
/// \param iNumberX number of POIs in x direction
/// \param iNumberY number of POIs in y direction
/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
/// \param iSubsetX half size of the square subset in x direction
/// \param iSubsetY half size of the square subset in y direction
/// \param iGridSpaceX number of pixels between two POIs in x direction
/// \param iGirdSpaceY number of pixels between two POIs in y direction
TW_LIB_DLL_EXPORTS void cuComputePOIPositions(// Outputs
											  int_t *&Out_d_iPXY,			// Return the device handle
											  int_t ***&Out_h_iPXY,			// Retrun the host handle
											  // Inputs
											  int_t iStartX, int_t iStartY, // Start top-left point of the ROI
											  int_t iNumberX, int_t iNumberY,
											  int_t iMarginX, int_t iMarginY,
											  int_t iSubsetX, int_t iSubsetY,
											  int_t iGridSpaceX, int_t iGridSpaceY);

/// \brief Function to compute positions of POIs on GPU within the whole image, the result is stored in 
/// a device array. NOTE: No need to pre-allocate memory for the two pointers.
///
/// \param Out_d_iPXY position array in device memory
/// \param Out_h_iPXY position array in host memory
/// \param iStartX x coordinate of the top-left point of ROI
/// \param iStartY y coordinate of the top-left point of ROI
/// \param iNumberX number of POIs in x direction
/// \param iNumberY number of POIs in y direction
/// \param iMarginX number of extra safe pixels at ROI boundary in x direction
/// \param iMarginY number of extra safe pixels at ROI boundary in y direction
/// \param iSubsetX half size of the square subset in x direction
/// \param iSubsetY half size of the square subset in y direction
/// \param iGridSpaceX number of pixels between two POIs in x direction
/// \param iGirdSpaceY number of pixels between two POIs in y direction
TW_LIB_DLL_EXPORTS void cuComputePOIPositions(// Outputs
											  int_t *&Out_d_iPXY,			// Retrun the device handle
											  // Inputs
											  int_t iStartX, int_t iStartY, // Start top-left point of the ROI
											  int_t iNumberX, int_t iNumberY,
											  int_t iMarginX, int_t iMarginY,
											  int_t iSubsetX, int_t iSubsetY,
											  int_t iGridSpaceX, int_t iGridSpaceY);

/// \brief Function to compute Gradients of fImgF & fImgG on GPU, the results are stored in device
/// arrays. NOTE: The memory must be pre-allocated before calling this function. This rountine is 
/// especially useful for ICGN + Bicubic interpolation
///
/// \param fImgF Reference Image 
/// \param fImgG Target Image
/// \param iStartX x coordinate of the top-left point of ROI
/// \param iStartY y coordinate of the top-left point of ROI
/// \param iROIWidth Width of ROI
/// \param iROIHeight Height of ROI
/// \param iImgWidth Width of the image
/// \param iImgHeight Height of the image
/// \param AccuracyOrder The accuracy order of the gradients (Taylor's series)
/// \param Fx, Fy Gradient of fImgF in x,y dimensions
/// \param Gx, Gy, Gxy Gradient of fImgG in x, y adn xy dimensions
TW_LIB_DLL_EXPORTS void cuGradientXY_2Images(// Inputs
									  		 uchar1 *fImgF, uchar1 *fImgG,
											 int_t iStartX,   int_t iStartY,
											 int_t iROIWidth, int_t iROIHeight,
											 int_t iImgWidth, int_t iImgHeight,
											 AccuracyOrder accuracy,
										     // Outputs
											 real_t *Fx, real_t *Fy,
											 real_t *Gx, real_t *Gy, real_t *Gxy);

/// \brief Function to compute Gradients of fImg on GPU, the results are stored in device
/// arrays. NOTE: The memory must be pre-allocated before calling this function. This rountine is 
/// especially useful for ICGN + Bicubic Bspline interpolation
///
/// \param fImg input image
/// \param iStartX x coordinate of the top-left point of ROI
/// \param iStartY y coordinate of the top-left point of ROI
/// \param iROIWidth Width of ROI
/// \param iROIHeight Height of ROI
/// \param iImgWidth Width of the image
/// \param iImgHeight Height of the image
/// \param AccuracyOrder The accuracy order of the gradients (Taylor's series)
/// \param Gx, Gy Gradient of fImgF in x,y dimensions
TW_LIB_DLL_EXPORTS void cuGradient(// Inputs
								   uchar1 *fImg,
								   int_t iStartX,   int_t iStartY,
								   int_t iROIWidth, int_t iROIHeight,
								   int_t iImgWidth, int_t iImgHeight,
								   AccuracyOrder accuracy,
								   // Outputs
								   real_t *Gx, real_t *Gy);

/// \brief Function to Compute the Bicubic Interpolation Coefficients on GPU, the results are 
/// stroed in device memory
/// 
/// \param dIn_fImgT The image
/// \param dIn_fTx	 The Gradient X, but with range [iStar, iStart + iROI - 1]
/// \param dIn_fTy	 The Gradient Y, but with range [iStar, iStart + iROI - 1]
/// \param dIn_fTxy	 The Gradient XY,but with range [iStar, iStart + iROI - 1]
/// \param iStartX x coordinate of the top-left point of ROI
/// \param iStartY y coordinate of the top-left point of ROI
/// \param iROIWidth Width of ROI
/// \param iROIHeight Height of ROI
/// \param iImgWidth Width of the image
/// \param iImgHeight Height of the image
/// \param dOut_fBicubicInterpolants The LUT size: iROIHeight*iROIWidth*4*float4
TW_LIB_DLL_EXPORTS void cuBicubicCoefficients(// Inputs
				  							  const uchar1* dIn_fImgT, 
											  const real_t* dIn_fTx,
											  const real_t* dIn_fTy, 
											  const real_t* dIn_fTxy, 
											  const int iStartX, const int iStartY,
											  const int iROIWidth, const int iROIHeight, 
											  const int iImgWidth, const int iImgHeight, 
											  // Outputs
											  real_t4* dOut_fBicubicInterpolants);

/// \brief GPU function to compute z = ax + y in parallel.
/// \ strided for loop is used for the optimized performance and kernel size 
/// \ flexibility. NOTE: results are saved in vector y
///
/// \param n number of elements of x and y vector
/// \param a multiplier
/// \param x, y input vectors
/// \param devID the device number of the GPU in use
TW_LIB_DLL_EXPORTS void cuSaxpy(// Inputs
								int_t n, 
								real_t a, 
								real_t *x,
								int_t devID,
								// Output
								real_t *y);

// -----------------------------------GPU Wrapper Functions End-----------------------------!


} //!- namespace TW


#endif // !UTILS_H
