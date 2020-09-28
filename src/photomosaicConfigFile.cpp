/*===================================================================================
                                      Photomosaic
                          Copyright Kerry R. Loux 2009-2020

  This code is licensed under the MIT License (http://opensource.org/licenses/MIT).

===================================================================================*/

// File:  photomosaicConfigFile.cpp
// Auth:  K. Loux
// Date:  9/17/2020
// Desc:  Config file class for photomosaic application.

// Local headers
#include "photomosaicConfigFile.h"

void PhotoMosaicConfigFile::BuildConfigItems()
{
	AddConfigItem(_T("SOURCE_CENTER"), config.centerFocusSourceDirectory);
	AddConfigItem(_T("SOURCE_LEFT"), config.leftFocusSourceDirectory);
	AddConfigItem(_T("SOURCE_RIGHT"), config.rightFocusSourceDirectory);

	AddConfigItem(_T("TARGET_IMAGE"), config.targetImageFileName);
	AddConfigItem(_T("OUTPUT_FILE"), config.outputFileName);
	AddConfigItem(_T("THUMBNAIL_DIR"), config.thumbnailDirectory);
	
	AddConfigItem(_T("THUMBNAIL_SIZE"), config.thumbnailSize);
	AddConfigItem(_T("SUBDIVISION_SIZE"), config.subDivisionSize);
	AddConfigItem(_T("SUBSAMPLES"), config.subSamples);
	
	AddConfigItem(_T("RECURSIVE"), config.recursiveSourceDirectories);
	AddConfigItem(_T("MULTIPLE_USE"), config.allowMultipleOccurrences);
	AddConfigItem(_T("GREYSCALE"), config.greyscaleOutput);
	
	AddConfigItem(_T("HUE_WEIGHT"), config.hueErrorWeight);
	AddConfigItem(_T("SAT_WEIGHT"), config.saturationErrorWeight);
	AddConfigItem(_T("VAL_WEIGHT"), config.valueErrorWeight);

	AddConfigItem(_T("DIST_COUNT_THRESHOLD"), config.distancePenaltyCountThreshold);
	AddConfigItem(_T("DIST_PENALTY_SCALE"), config.distancePenaltyScale);
}

void PhotoMosaicConfigFile::AssignDefaults()
{
	config.thumbnailSize = 0;
	config.subDivisionSize = 0;
	config.subSamples = 0;
	
	config.recursiveSourceDirectories = false;
	config.allowMultipleOccurrences = true;
	config.greyscaleOutput = false;
	
	config.hueErrorWeight = 1.0;
	config.saturationErrorWeight = 1.0;
	config.valueErrorWeight = 1.0;

	config.distancePenaltyCountThreshold = 2;
	config.distancePenaltyScale = 0.0;
}

bool PhotoMosaicConfigFile::ConfigIsOK()
{
	bool ok(true);
	if (config.centerFocusSourceDirectory.empty() && config.leftFocusSourceDirectory.empty() && config.rightFocusSourceDirectory.empty())
	{
		outStream << "Must specify at least one of " << GetKey(config.centerFocusSourceDirectory)
			<< ", " << GetKey(config.leftFocusSourceDirectory) << ", or " << GetKey(config.rightFocusSourceDirectory) << std::endl;
		ok = false;
	}
	
	ok = IsSpecified(config.targetImageFileName) && ok;
	ok = IsSpecified(config.outputFileName) && ok;
	
	ok = IsStrictlyPositive(config.thumbnailSize) && ok;
	ok = IsStrictlyPositive(config.subDivisionSize) && ok;
	ok = IsPositive(config.subSamples) && ok;
	
	ok = IsPositive(config.hueErrorWeight) && ok;
	ok = IsPositive(config.saturationErrorWeight) && ok;
	ok = IsPositive(config.valueErrorWeight) && ok;

	ok = IsPositive(config.distancePenaltyCountThreshold) && ok;
	ok = IsPositive(config.distancePenaltyScale) && ok;
	
	return ok;
}

bool PhotoMosaicConfigFile::IsSpecified(const std::string& s)
{
	if (s.empty())
	{
		outStream << GetKey(s) << " must be specified" << std::endl;
		return false;
	}
	
	return true;
}
