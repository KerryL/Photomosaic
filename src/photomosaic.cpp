/*===================================================================================
                                      Photomosaic
                          Copyright Kerry R. Loux 2009-2020

  This code is licensed under the MIT License (http://opensource.org/licenses/MIT).

===================================================================================*/

// File:  photomosaic.cpp
// Auth:  K. Loux
// Date:  9/17/2020
// Desc:  Core class for photomosaic application.

// Local headers
#include "photomosaic.h"

// Standard C++ headers
#include <cassert>
#include <iostream>

wxImage Photomosaic::Build()
{
	wxImage targetImage;
	if (!targetImage.LoadFile(config.targetImageFileName))
	{
		std::cerr << "Failed to load target image from '" << config.targetImageFileName << '\'' << std::endl;
		return targetImage;
	}
	
	const unsigned int width(targetImage.GetWidth());
	const unsigned int height(targetImage.GetHeight());
	const unsigned int xTiles(width / config.subDivisionSize);
	const unsigned int yTiles(height / config.subDivisionSize);
	
	// If the image size isn't evenly divisible by the tile size, center the tiles on the target image
	const unsigned int xOffset(width - xTiles * config.subDivisionSize);
	const unsigned int yOffset(height - yTiles * config.subDivisionSize);
	
	std::cout << "Image will require " << xTiles * yTiles << " tiles" << std::endl;
	
	TargetInfo targetInfo(xTiles);
	for (unsigned int x = 0; x < xTiles; ++x)
	{
		targetInfo[x].resize(yTiles);
		for (unsigned int y = 0; y < yTiles; ++y)
		{
			targetInfo[x][y] = GetColorInformation(targetImage.GetSubImage(wxRect(
				xOffset + x * config.subDivisionSize, yOffset + y * config.subDivisionSize,
				config.subDivisionSize, config.subDivisionSize)), config.subSamples);
		}
	}
	
	const auto thumbnailInfo(GetThumbnailInfo());
	
	// Find the score for every thumbnail at every grid location
	std::vector<std::vector<std::vector<double>>> scores(thumbnailInfo.size());
	for (unsigned int i = 0; i < thumbnailInfo.size(); ++i)
		scores[i] = ScoreGrid(targetInfo, thumbnailInfo[i].info);
		
	// TODO:  Choose images
	// TODO:  Build output image
	wxImage outputImage;
	
	return outputImage;
}

Photomosaic::InfoGrid Photomosaic::GetColorInformation(const wxImage& image, const unsigned int& subSamples)
{
	InfoGrid info(subSamples);
	const unsigned int sampleDimension(image.GetWidth() / subSamples);
	for (unsigned int x = 0; x < subSamples; ++x)
	{
		info[x].resize(subSamples);
		for (unsigned int y = 0; y < subSamples; ++y)
		{
			info[x][y].red = 0;
			info[x][y].green = 0;
			info[x][y].blue = 0;
			
			for (unsigned int i = 0; i < sampleDimension; ++i)
			{
				for (unsigned int j = 0; j < sampleDimension; ++j)
				{
					info[x][y].red += image.GetRed(x * sampleDimension + i, y * sampleDimension + j);
					info[x][y].green = image.GetGreen(x * sampleDimension + i, y * sampleDimension + j);
					info[x][y].blue = image.GetBlue(x * sampleDimension + i, y * sampleDimension + j);
				}
			}
			
			info[x][y].red /= sampleDimension * sampleDimension;
			info[x][y].green /= sampleDimension * sampleDimension;
			info[x][y].blue /= sampleDimension * sampleDimension;
		}
	}
	
	return info;
}
	
std::vector<std::vector<double>> Photomosaic::ScoreGrid(const TargetInfo& targetGrid, const InfoGrid& subInfo)
{
	// TODO:  Implement
	return std::vector<std::vector<double>>();
}

std::vector<Photomosaic::ImageInfo> Photomosaic::GetThumbnailInfo() const
{
	std::vector<Photomosaic::ImageInfo> info;
	
	if (config.recursiveSourceDirectories)
	{
		for (auto& entry : stdfs::recursive_directory_iterator(config.centerFocusSourceDirectory))
		{
			ImageInfo tempInfo;
			if (ProcessThumbnailDirectoryEntry(entry, config.thumbnailDirectory, CropHint::Center, tempInfo, config.thumbnailSize, config.subSamples))
				info.push_back(std::move(tempInfo));
		}

		for (auto& entry : stdfs::recursive_directory_iterator(config.leftFocusSourceDirectory))
		{
			ImageInfo tempInfo;
			if (ProcessThumbnailDirectoryEntry(entry, config.thumbnailDirectory, CropHint::Left, tempInfo, config.thumbnailSize, config.subSamples))
				info.push_back(std::move(tempInfo));
		}
			
		for (auto& entry : stdfs::recursive_directory_iterator(config.rightFocusSourceDirectory))
		{
			ImageInfo tempInfo;
			if (ProcessThumbnailDirectoryEntry(entry, config.thumbnailDirectory, CropHint::Right, tempInfo, config.thumbnailSize, config.subSamples))
				info.push_back(std::move(tempInfo));
		}
	}
	else
	{
		for (auto& entry : stdfs::directory_iterator(config.centerFocusSourceDirectory))
		{
			ImageInfo tempInfo;
			if (ProcessThumbnailDirectoryEntry(entry, config.thumbnailDirectory, CropHint::Center, tempInfo, config.thumbnailSize, config.subSamples))
				info.push_back(std::move(tempInfo));
		}

		for (auto& entry : stdfs::directory_iterator(config.leftFocusSourceDirectory))
		{
			ImageInfo tempInfo;
			if (ProcessThumbnailDirectoryEntry(entry, config.thumbnailDirectory, CropHint::Left, tempInfo, config.thumbnailSize, config.subSamples))
				info.push_back(std::move(tempInfo));
		}
			
		for (auto& entry : stdfs::directory_iterator(config.rightFocusSourceDirectory))
		{
			ImageInfo tempInfo;
			if (ProcessThumbnailDirectoryEntry(entry, config.thumbnailDirectory, CropHint::Right, tempInfo, config.thumbnailSize, config.subSamples))
				info.push_back(std::move(tempInfo));
		}
	}
	
	return std::move(info);
}

bool Photomosaic::ProcessThumbnailDirectoryEntry(const stdfs::directory_entry& entry, const std::string& thumbnailDirectory, const CropHint& cropHint,
	ImageInfo& info, const unsigned int& thumbnailSize, const unsigned int& subSamples)
{
#ifdef _WIN32
	if (entry.status().type() != stdfs::file_type::regular)
#else
	if (!entry.is_regular_file())
#endif// _WIN32
		return false;
		
	bool foundExistingThumbnail(false);
	if (!thumbnailDirectory.empty())
	{
		// TODO:  check for match in thumbnail directory first
	}
		
	if (!foundExistingThumbnail)
	{
		if (!info.image.LoadFile(entry.path().generic_string()))
			return false;

		if (info.image.GetHeight() > info.image.GetWidth())// No implementation for crop top/bottom, so force these to center vertically for now
			info.image.Resize(wxSize(info.image.GetWidth(), info.image.GetWidth()), wxPoint(0, (info.image.GetHeight() - info.image.GetWidth()) / 2));
		else if (cropHint == CropHint::Center)
			info.image.Resize(wxSize(info.image.GetHeight(), info.image.GetHeight()), wxPoint((info.image.GetWidth() - info.image.GetHeight()) / 2, 0));
		else if (cropHint == CropHint::Left)
			info.image.Resize(wxSize(info.image.GetHeight(), info.image.GetHeight()), wxPoint(0, 0));
		else if (cropHint == CropHint::Right)
			info.image.Resize(wxSize(info.image.GetHeight(), info.image.GetHeight()), wxPoint(info.image.GetWidth() - info.image.GetHeight(), 0));
		else
		{
			assert(false && "unexpected crop hint");
		}
		
		info.image.Scale(thumbnailSize, thumbnailSize);
		
		if (!thumbnailDirectory.empty())
		{
			stdfs::path thumbnailPath(thumbnailDirectory);
			thumbnailPath.append(entry.path().filename().generic_string());
			if (!info.image.SaveFile(thumbnailPath.generic_string()))
				std::cerr << "Failed to write thumbnail to '" << thumbnailPath.generic_string() << '\'' << std::endl;
		}
	}

	info.info = GetColorInformation(info.image, subSamples);
		
	return true;
}
