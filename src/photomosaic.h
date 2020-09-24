/*===================================================================================
                                      Photomosaic
                          Copyright Kerry R. Loux 2009-2020

  This code is licensed under the MIT License (http://opensource.org/licenses/MIT).

===================================================================================*/

// File:  photomosaic.h
// Auth:  K. Loux
// Date:  9/17/2020
// Desc:  Core class for photomosaic application.

#ifndef PHOTOMOSAIC_H_
#define PHOTOMOSAIC_H_

// Local headers
#include "photomosaicConfig.h"

// wxWidgets headers
#include <wx/wx.h>
#include <wx/image.h>

// Standard C++ headers
#include <vector>
#include <filesystem>

#ifdef _WIN32
namespace stdfs = std::experimental::filesystem;
#else
namespace stdfs = std::filesystem;
#endif// _WIN32

class Photomosaic
{
public:
	Photomosaic(const PhotomosaicConfig& config) : config(config) {}
	wxImage Build();

private:
	const PhotomosaicConfig config;
	
	struct SquareInfo
	{
		double hue;
		double saturation;
		double value;
	};
	
	typedef std::vector<std::vector<SquareInfo>> InfoGrid;
	typedef std::vector<std::vector<InfoGrid>> TargetInfo;
	
	static InfoGrid GetColorInformation(const wxImage& image, const unsigned int& subSamples);
	
	struct ImageInfo
	{
		wxImage image;
		InfoGrid info;
	};

	std::vector<ImageInfo> GetThumbnailInfo() const;
	
	std::vector<std::vector<double>> ScoreGrid(const TargetInfo& targetGrid, const InfoGrid& thumbnail) const;
	double ComputeScore(const InfoGrid& targetSquare, const InfoGrid& thumbnail) const;
	
	enum class CropHint
	{
		Left,
		Center,
		Right
	};
	
	static bool ProcessThumbnailDirectoryEntry(const stdfs::directory_entry& entry, const std::string& thumbnailDirectory, const CropHint& cropHint,
		ImageInfo& info, const unsigned int& thumbnailSize, const unsigned int& subSamples);
		
	static SquareInfo RGBToHSV(const double& red, const double& blue, const double& green);
	static SquareInfo ComputeAverageColor(const std::vector<SquareInfo>& colors);
};

#endif// PHOTOMOSAIC_H_
