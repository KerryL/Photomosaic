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

class Photomosaic
{
public:
	Photomosaic(const PhotomosaicConfig& config) : config(config) {}
	wxImage Build();

private:
	const PhotomosaicConfig config;
	
	struct SquareInfo
	{
		int red;
		int green;
		int blue;
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
	
	static std::vector<std::vector<double>> ScoreGrid(const TargetInfo& targetGrid, const InfoGrid& subInfo);
	
	enum class CropHint
	{
		Left,
		Center,
		Right
	};
	
	static bool ProcessThumbnailDirectoryEntry(const std::filesystem::directory_entry& entry, const std::string& thumbnailDirectory, const CropHint& cropHint,
		ImageInfo& info, const unsigned int& thumbnailSize, const unsigned int& subSamples);
};

#endif// PHOTOMOSAIC_H_
