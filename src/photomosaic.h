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
#include "threadPool.h"

// wxWidgets headers
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

	struct TileScore
	{
		unsigned int thumbnailIndex;
		double score;
	};

	typedef std::vector<std::vector<std::vector<TileScore>>> ScoreGrid;
	
	static ScoreGrid CreateSortedScoreGrid(const std::vector<std::vector<std::vector<double>>>& scores);
	static std::vector<std::vector<unsigned int>> ChooseTiles(ScoreGrid& scores, const PhotomosaicConfig& config);
	static void ApplyDistancePenalty(ScoreGrid& scores, const PhotomosaicConfig& config);
	static wxImage BuildOutputImage(const std::vector<std::vector<unsigned int>>& chosenTiles, const std::vector<ImageInfo>& thumbnailInfo);
	
	std::vector<std::vector<double>> ScoreAllThumbnailsOnGrid(const TargetInfo& targetGrid, const InfoGrid& thumbnail) const;
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
	
	class ThumbnailProcessJob : public ThreadPool::JobInfoBase
	{
	public:
		ThumbnailProcessJob(const stdfs::directory_entry& entry, const PhotomosaicConfig& config,
			const CropHint& cropHint, std::vector<Photomosaic::ImageInfo>& info, std::mutex& mutex) : entry(entry), config(config), cropHint(cropHint), info(info), mutex(mutex) {}
		
	protected:
		const stdfs::directory_entry entry;
		const PhotomosaicConfig& config;
		const CropHint cropHint;
		
		std::vector<Photomosaic::ImageInfo>& info;
		std::mutex& mutex;
			
		void DoJob() override
		{
			ImageInfo tempInfo;
			if (ProcessThumbnailDirectoryEntry(entry, config.thumbnailDirectory, cropHint, tempInfo, config.thumbnailSize, config.subSamples))
			{
				std::lock_guard<std::mutex> lock(mutex);
				info.push_back(std::move(tempInfo));
			}
		}
	};
	
	class TileProcessJob : public ThreadPool::JobInfoBase
	{
	public:
		TileProcessJob(const wxImage&& subRect, const unsigned int& subSamples, InfoGrid& targetInfo) : subRect(std::move(subRect)), subSamples(subSamples), targetInfo(targetInfo) {}
		
	protected:
		const wxImage subRect;
		const unsigned int subSamples;
		InfoGrid& targetInfo;
		
		void DoJob() override
		{
			targetInfo = GetColorInformation(subRect, subSamples);
		}
	};

	class ScoringJob : public ThreadPool::JobInfoBase
	{
	public:
		ScoringJob(const Photomosaic& self, const TargetInfo& targetInfo, const InfoGrid& thumbnail,
			std::vector<std::vector<double>>& score) : self(self), targetInfo(targetInfo), thumbnail(thumbnail), score(score) {}

	protected:
		const Photomosaic& self;
		const TargetInfo& targetInfo;
		const InfoGrid& thumbnail;
		std::vector<std::vector<double>>& score;

		void DoJob() override
		{
			score = self.ScoreAllThumbnailsOnGrid(targetInfo, thumbnail);
		}
	};
};

#endif// PHOTOMOSAIC_H_
