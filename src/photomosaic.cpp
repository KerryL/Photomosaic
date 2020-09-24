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
#include <algorithm>

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
		scores[i] = ScoreGrid(targetInfo, thumbnailInfo[i].info);// Lower scores represent better fits

	const auto chosenTileIndices(ChooseTiles(scores));
	return std::move(BuildOutputImage(chosenTileIndices, thumbnailInfo));
}

std::vector<std::vector<unsigned int>> Photomosaic::ChooseTiles(std::vector<std::vector<std::vector<double>>> scores)
{
	// TODO:  Modify algorithm to include a penalty for using too many of a sinagle image or too many of the same image in a small area
	std::vector<std::vector<unsigned int>> chosenIndices(scores.size());
	for (unsigned int x = 0; x < scores.size(); ++x)
	{
		chosenIndices[x].resize(scores[x].size());
		for (unsigned int y = 0; y < scores.front().size(); ++y)
		{
			const auto bestScoreIndex(std::distance(std::max_element(scores[x][y].begin(), scores[x][y].end()), scores[x][y].begin()));
			chosenIndices[x][y] = bestScoreIndex;
		}
	}
	
	return chosenIndices;
}

wxImage Photomosaic::BuildOutputImage(const std::vector<std::vector<unsigned int>>& chosenTileIndices, const std::vector<ImageInfo>& thumbnailInfo)
{
	wxImage image(chosenTileIndices.size() * thumbnailInfo.front().image.GetWidth(), chosenTileIndices.front().size() * thumbnailInfo.front().image.GetHeight());
	for (unsigned int i = 0; i < static_cast<unsigned int>(image.GetWidth()); ++i)
	{
		for (unsigned int j = 0; j < static_cast<unsigned int>(image.GetHeight()); ++j)
		{
			const auto thumbnailSize(thumbnailInfo.front().image.GetWidth());
			const auto xChoice(i / chosenTileIndices.size());
			const auto yChoice(j / chosenTileIndices.front().size());
			const auto& thumb(thumbnailInfo[chosenTileIndices[xChoice][yChoice]].image);
			const auto xOffset(i - xChoice * thumbnailSize);
			const auto yOffset(j - yChoice * thumbnailSize);
			image.SetRGB(i, j, thumb.GetRed(xOffset, yOffset), thumb.GetGreen(xOffset, yOffset), thumb.GetBlue(xOffset, yOffset));
		}
	}

	return std::move(image);
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
			std::vector<SquareInfo> pixelValues(sampleDimension * sampleDimension);
			for (unsigned int i = 0; i < sampleDimension; ++i)
			{
				for (unsigned int j = 0; j < sampleDimension; ++j)
				{
					pixelValues[i * sampleDimension + j] = RGBToHSV(image.GetRed(x * sampleDimension + i, y * sampleDimension + j),
						image.GetGreen(x * sampleDimension + i, y * sampleDimension + j),
						image.GetBlue(x * sampleDimension + i, y * sampleDimension + j));
				}
			}
			
			info[x][y] = ComputeAverageColor(pixelValues);
		}
	}
	
	return info;
}
	
std::vector<std::vector<double>> Photomosaic::ScoreGrid(const TargetInfo& targetGrid, const InfoGrid& thumbnail) const
{
	std::vector<std::vector<double>> scores(targetGrid.size());
	for (unsigned int i = 0; i < targetGrid.size(); ++i)
	{
		scores[i].resize(targetGrid.front().size());
		for (unsigned int j = 0; j < targetGrid.size(); ++j)
			scores[i][j] = ComputeScore(targetGrid[i][j], thumbnail);
	}
	
	return scores;
}

// Implemented as a cost function, so lower values represent better fits
double Photomosaic::ComputeScore(const InfoGrid& targetSquare, const InfoGrid& thumbnail) const
{
	double score(0.0);
	for (unsigned int i = 0; i < targetSquare.size(); ++i)
	{
		for (unsigned int j = 0; j < targetSquare.size(); ++j)
		{
			const double hueError(fmod(targetSquare[i][j].hue - thumbnail[i][j].hue, 1.0));
			if (hueError > 0.5)
				score += (1.0 - hueError) * config.hueErrorWeight;
			else
				score += hueError * config.hueErrorWeight;
			score += fabs(targetSquare[i][j].saturation - thumbnail[i][j].saturation) * config.saturationErrorWeight;
			score += fabs(targetSquare[i][j].value - thumbnail[i][j].value) * config.valueErrorWeight;
		}
	}
	
	return score;
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
		const std::string sep([&thumbnailDirectory]()
		{
#ifdef _WIN32
			if (thumbnailDirectory.back() != '\\')
				return std::string("\\");
#else
			if (thumbnailDirectory.back() != '/')
				return std::string("/");
#endif
			return std::string();
		}());
		
		foundExistingThumbnail = info.image.LoadFile(thumbnailDirectory + sep + entry.path().filename().generic_string());
		if (foundExistingThumbnail &&
			(static_cast<unsigned int>(info.image.GetWidth()) != thumbnailSize || static_cast<unsigned int>(info.image.GetHeight()) != thumbnailSize))
		{
			std::cerr << "Loaded existing thumbnail; expected dimension = " << thumbnailSize << " but found dimension = " << info.image.GetWidth() << "x" << info.image.GetHeight() << '\n';
			return false;
		}
	}
		
	if (!foundExistingThumbnail)
	{
		if (!info.image.LoadFile(entry.path().generic_string()))
		{
			std::cerr << "Failed to load image from '" << entry.path().generic_string() << "'\n";
			return false;
		}

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

Photomosaic::SquareInfo Photomosaic::RGBToHSV(const double& red, const double& blue, const double& green)
{
	assert(red >= 0.0 && red <= 1.0);
	assert(blue >= 0.0 && blue <= 1.0);
	assert(green >= 0.0 && green <= 1.0);
	
	SquareInfo si;
	const double maxColor(std::max(std::max(red, green), blue));
	const double minColor(std::min(std::min(red, green), blue));
	const double chroma(maxColor - minColor);
	if (chroma == 0.0)
		si.hue = 0.0;
	else if (maxColor == red)
		si.hue = (green - blue) / chroma;
	else if (maxColor == green)
		si.hue = (blue - red) / chroma + 2.0;
	else// if (maxColor == blue)
		si.hue = (red - green) / chroma + 4.0;
		
	// At this point, si.hue is in a range from -1.0 to 5.0, representing an angle between 0 and 360 deg on the color wheel
	// Scale to lie within the range 0 to 1
	si.hue = (si.hue + 1.0) / 6.0;

	if (si.value == 0.0)
		si.saturation = 0.0;
	else
		si.saturation = chroma / si.value;
		
	si.value = maxColor;
		
	return si;
}

Photomosaic::SquareInfo Photomosaic::ComputeAverageColor(const std::vector<SquareInfo>& colors)
{
	double hueX(0.0);
	double hueY(0.0);
	double saturation(0.0);
	double value(0.0);
	for (const auto& c : colors)
	{
		hueX += cos(c.hue * 2.0 * M_PI);
		hueY += sin(c.hue * 2.0 * M_PI);
		saturation += c.saturation;
		value += c.value;
	}
	
	SquareInfo si;
	si.hue = atan2(hueY, hueX) / 2.0 / M_PI;
	si.saturation = saturation / colors.size();
	value = si.value / colors.size();
	return si;
}
