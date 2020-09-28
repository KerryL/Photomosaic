/*===================================================================================
                                      Photomosaic
                          Copyright Kerry R. Loux 2009-2020

  This code is licensed under the MIT License (http://opensource.org/licenses/MIT).

===================================================================================*/

// File:  photomosaicConfig.h
// Auth:  K. Loux
// Date:  9/17/2020
// Desc:  Configuration parameters for photomosaic application.

#ifndef PHOTOMOSAIC_CONFIG_H_
#define PHOTOMOSAIC_CONFIG_H_

// Standard C++ headers
#include <string>

struct PhotomosaicConfig
{
	std::string centerFocusSourceDirectory;
	std::string leftFocusSourceDirectory;
	std::string rightFocusSourceDirectory;
	
	std::string targetImageFileName;
	std::string outputFileName;
	std::string thumbnailDirectory;
	
	int thumbnailSize = 0;
	int subDivisionSize = 0;
	int subSamples = 0;
	
	bool recursiveSourceDirectories = false;
	bool allowMultipleOccurrences = true;
	bool greyscaleOutput = false;
	
	double hueErrorWeight;
	double saturationErrorWeight;
	double valueErrorWeight;
	
	unsigned int distancePenaltyCountThreshold;
	double distancePenaltyScale;
};

#endif// PHOTOMOSAIC_CONFIG_H_
