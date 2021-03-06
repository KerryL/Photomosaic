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

// wxWidgets headers
#include <wx/log.h>

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
	
	std::cout << "Image will require " << xTiles * yTiles << " tiles\nExtracting information from source image..." << std::endl;
	
	ThreadPool pool(std::thread::hardware_concurrency() * 2);
	TargetInfo targetInfo(xTiles);
	for (unsigned int x = 0; x < xTiles; ++x)
	{
		targetInfo[x].resize(yTiles);
		for (unsigned int y = 0; y < yTiles; ++y)
			pool.AddJob(std::make_unique<TileProcessJob>(targetImage.GetSubImage(wxRect(xOffset + x * config.subDivisionSize, yOffset + y * config.subDivisionSize,
				config.subDivisionSize, config.subDivisionSize)), config.subSamples, targetInfo[x][y]));
	}
	
	pool.WaitForAllJobsComplete();
	
	std::cout << "Preparing thumbnails..." << std::endl;
	const auto thumbnailInfo(GetThumbnailInfo());
	
	// Find the score for every thumbnail at every grid location
	std::cout << "Scoring tiles..." << std::endl;
	std::vector<std::vector<std::vector<double>>> scores(thumbnailInfo.size());// Lower scores represent better fit
	for (unsigned int i = 0; i < thumbnailInfo.size(); ++i)
		pool.AddJob(std::make_unique<ScoringJob>(*this, targetInfo, thumbnailInfo[i].info, scores[i]));
	pool.WaitForAllJobsComplete();
	auto sortedScores(CreateSortedScoreGrid(scores));
	const auto chosenTileIndices(ChooseTiles(sortedScores, config));
	
	std::cout << "Building output image..." << std::endl;
	return std::move(BuildOutputImage(chosenTileIndices, thumbnailInfo));
}

Photomosaic::ScoreGrid Photomosaic::CreateSortedScoreGrid(const std::vector<std::vector<std::vector<double>>>& scores)
{
	ScoreGrid sortedScores(scores.front().size());
	for (unsigned int x = 0; x < scores.front().size(); ++x)
	{
		sortedScores[x].resize(scores.front().front().size());
		for (unsigned int y = 0; y < scores.front().front().size(); ++y)
		{
			sortedScores[x][y].resize(scores.size());
			for (unsigned int thumb = 0; thumb < scores.size(); ++thumb)
			{
				sortedScores[x][y][thumb].thumbnailIndex = thumb;
				sortedScores[x][y][thumb].score = scores[thumb][x][y];
			}

			std::sort(sortedScores[x][y].begin(), sortedScores[x][y].end(), [](const TileScore& a, const TileScore& b)
			{
				return a.score < b.score;
			});
		}
	}

	return sortedScores;
}

std::vector<std::vector<unsigned int>> Photomosaic::ChooseTiles(ScoreGrid& scores, const PhotomosaicConfig& config)
{
	if (config.distancePenaltyScale > 0)
		ApplyDistancePenalty(scores, config);

	std::vector<std::vector<unsigned int>> chosenIndices(scores.size());
	for (unsigned int x = 0; x < scores.size(); ++x)
	{
		chosenIndices[x].resize(scores.front().size());
		for (unsigned int y = 0; y < scores.front().size(); ++y)
			chosenIndices[x][y] = scores[x][y].front().thumbnailIndex;
	}
	
	return chosenIndices;
}

// The distance penalty is generated using a "repulsive force" type model.  So the closer two tiles
// are, the stronger they repel each other and the higher the penalty added to both tiles.
void Photomosaic::ApplyDistancePenalty(ScoreGrid& scores, const PhotomosaicConfig& config)
{
	struct Coordinate
	{
		Coordinate() = default;
		Coordinate(const unsigned int& x, const unsigned int& y) : x(x), y(y) {}

		unsigned int x;
		unsigned int y;
	};

	for (unsigned int thumb = 0; thumb < scores.front().front().size(); ++thumb)
	{
		std::vector<std::vector<double>> penalty(scores.size());
		std::vector<Coordinate> coords;
		for (unsigned int x = 0; x < penalty.size(); ++x)
		{
			penalty[x].resize(scores.front().size());
			for (unsigned int y = 0; y < penalty.front().size(); ++y)
			{
				if (scores[x][y].front().thumbnailIndex == thumb)
				{
					penalty[x][y] = 1.0;
					coords.push_back(Coordinate(x, y));
				}
				else
					penalty[x][y] = 0.0;
			}
		}

		if (config.distancePenaltyCountThreshold > 0 && coords.size() < config.distancePenaltyCountThreshold)
			continue;

		std::vector<std::vector<double>> distances(coords.size());
		const unsigned int refDistance(scores.size() * scores.size() + scores.front().size() * scores.front().size());
		for (unsigned int i = 0; i < distances.size(); ++i)
		{
			distances[i].resize(distances.size());
			for (unsigned int j = 0; j < distances.front().size(); ++j)
				distances[i][j] = static_cast<double>((coords[i].x - coords[j].x) * (coords[i].x - coords[j].x) + (coords[i].y - coords[j].y) * (coords[i].y - coords[j].y)) / refDistance;
		}

		for (unsigned int i = 0; i < coords.size(); ++i)
		{
			double sumRecipDistance(0.0);
			for (unsigned int j = 0; j < distances.size(); ++j)
			{
				if (distances[i][j] > 0.0)
					sumRecipDistance += 1.0 / distances[i][j];
			}

			assert(penalty[coords[i].x][coords[i].y] == 1.0);
			penalty[coords[i].x][coords[i].y] = sumRecipDistance * config.distancePenaltyScale;
		}

		for (unsigned int x = 0; x < penalty.size(); ++x)
		{
			for (unsigned int y = 0; y < penalty.front().size(); ++y)
				scores[x][y].front().score += penalty[x][y];
		}
	}
}

wxImage Photomosaic::BuildOutputImage(const std::vector<std::vector<unsigned int>>& chosenTileIndices, const std::vector<ImageInfo>& thumbnailInfo)
{
	wxImage image(chosenTileIndices.size() * thumbnailInfo.front().image.GetWidth(), chosenTileIndices.front().size() * thumbnailInfo.front().image.GetHeight());
	for (unsigned int i = 0; i < static_cast<unsigned int>(image.GetWidth()); ++i)
	{
		for (unsigned int j = 0; j < static_cast<unsigned int>(image.GetHeight()); ++j)
		{
			const auto thumbnailSize(thumbnailInfo.front().image.GetWidth());
			const auto xChoice(i / thumbnailSize);
			const auto yChoice(j / thumbnailSize);
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
					pixelValues[i * sampleDimension + j] = RGBToHSV(image.GetRed(x * sampleDimension + i, y * sampleDimension + j) / 255.0,
						image.GetGreen(x * sampleDimension + i, y * sampleDimension + j) / 255.0,
						image.GetBlue(x * sampleDimension + i, y * sampleDimension + j) / 255.0);
				}
			}
			
			info[x][y] = ComputeAverageColor(pixelValues);
		}
	}
	
	return info;
}
	
std::vector<std::vector<double>> Photomosaic::ScoreAllThumbnailsOnGrid(const TargetInfo& targetGrid, const InfoGrid& thumbnail) const
{
	std::vector<std::vector<double>> scores(targetGrid.size());
	for (unsigned int i = 0; i < targetGrid.size(); ++i)
	{
		scores[i].resize(targetGrid.front().size());
		for (unsigned int j = 0; j < targetGrid[i].size(); ++j)
			scores[i][j] = ComputeScore(targetGrid[i][j], thumbnail);
	}
	
	return std::move(scores);
}

// Implemented as a cost function, so lower values represent better fits
double Photomosaic::ComputeScore(const InfoGrid& targetSquare, const InfoGrid& thumbnail) const
{
	double score(0.0);
	for (unsigned int i = 0; i < targetSquare.size(); ++i)
	{
		for (unsigned int j = 0; j < targetSquare.front().size(); ++j)
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
	ThreadPool pool(std::thread::hardware_concurrency() * 2);
	std::mutex infoAccessMutex;
	
	if (config.recursiveSourceDirectories)
	{
		if (!config.centerFocusSourceDirectory.empty())
		{
			for (auto& entry : stdfs::recursive_directory_iterator(config.centerFocusSourceDirectory))
				pool.AddJob(std::make_unique<ThumbnailProcessJob>(entry, config, CropHint::Center, info, infoAccessMutex));
		}

		if (!config.leftFocusSourceDirectory.empty())
		{
			for (auto& entry : stdfs::recursive_directory_iterator(config.leftFocusSourceDirectory))
				pool.AddJob(std::make_unique<ThumbnailProcessJob>(entry, config, CropHint::Left, info, infoAccessMutex));
		}
			
		if (!config.rightFocusSourceDirectory.empty())
		{
			for (auto& entry : stdfs::recursive_directory_iterator(config.rightFocusSourceDirectory))
				pool.AddJob(std::make_unique<ThumbnailProcessJob>(entry, config, CropHint::Right, info, infoAccessMutex));
		}
	}
	else
	{
		if (!config.centerFocusSourceDirectory.empty())
		{
			for (auto& entry : stdfs::directory_iterator(config.centerFocusSourceDirectory))
				pool.AddJob(std::make_unique<ThumbnailProcessJob>(entry, config, CropHint::Center, info, infoAccessMutex));
		}

		if (!config.leftFocusSourceDirectory.empty())
		{
			for (auto& entry : stdfs::directory_iterator(config.leftFocusSourceDirectory))
				pool.AddJob(std::make_unique<ThumbnailProcessJob>(entry, config, CropHint::Left, info, infoAccessMutex));
		}
			
		if (!config.rightFocusSourceDirectory.empty())
		{
			for (auto& entry : stdfs::directory_iterator(config.rightFocusSourceDirectory))
				pool.AddJob(std::make_unique<ThumbnailProcessJob>(entry, config, CropHint::Right, info, infoAccessMutex));
		}
	}
	
	pool.WaitForAllJobsComplete();
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

	wxLogNull noLog;// Disable logging when loading image files since we expect that some or all may fail

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
			std::cerr << "Loaded existing thumbnail; expected dimension = " << thumbnailSize << "x" << thumbnailSize
				<< " but found dimension = " << info.image.GetWidth() << "x" << info.image.GetHeight() << '\n';
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

		const int minDim(std::min(info.image.GetHeight(), info.image.GetWidth()));
		const wxSize squareSize(minDim, minDim);
		wxPoint offset;
		if (minDim == info.image.GetWidth())// No implementation for crop top/bottom, so force these to center vertically for now
			offset = wxPoint(0, (info.image.GetWidth() - info.image.GetHeight()) / 2);
		else if (cropHint == CropHint::Center)
			offset = wxPoint((minDim - info.image.GetWidth()) / 2, 0);
		else if (cropHint == CropHint::Left)
			offset = wxPoint(0, 0);
		else if (cropHint == CropHint::Right)
			offset = wxPoint(minDim - info.image.GetWidth(), 0);
		else
		{
			assert(false && "unexpected crop hint");
		}

		info.image.Resize(squareSize, offset);
		info.image.Rescale(thumbnailSize, thumbnailSize);
		
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

	si.value = maxColor;

	if (si.value == 0.0)
		si.saturation = 0.0;
	else
		si.saturation = chroma / si.value;
		
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
	si.value = value / colors.size();
	return si;
}
