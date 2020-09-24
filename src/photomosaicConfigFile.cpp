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
	AddConfigItem(_T("SEED"), config.seed);
	
	AddConfigItem(_T("RECURSIVE"), config.recursiveSourceDirectories);
	AddConfigItem(_T("MULTIPLE_USE"), config.allowMultipleOccurrences);
	AddConfigItem(_T("GREYSCALE"), config.greyscaleOutput);
	
	AddConfigItem(_T("HUE_WEIGHT"), config.hueErrorWeight);
	AddConfigItem(_T("SAT_WEIGHT"), config.saturationErrorWeight);
	AddConfigItem(_T("VAL_WEIGHT"), config.valueErrorWeight);
}

void PhotoMosaicConfigFile::AssignDefaults()
{
	config.thumbnailSize = 0;
	config.subDivisionSize = 0;
	config.subSamples = 0;
	config.seed = -1;
	
	config.recursiveSourceDirectories = false;
	config.allowMultipleOccurrences = true;
	config.greyscaleOutput = false;
	
	config.hueErrorWeight = 1.0;
	config.saturationErrorWeight = 1.0;
	config.valueErrorWeight = 1.0;
}

bool PhotoMosaicConfigFile::ConfigIsOK()
{
	bool ok(true);
	if (config.centerFocusSourceDirectory.empty() && config.leftFocusSourceDirectory.empty() && config.rightFocusSourceDirectory.empty())
	{
		outStream << "Must specify at least one of " << GetKey(config.centerFocusSourceDirectory)
			<< ", " << GetKey(config.leftFocusSourceDirectory << ", or " << GetKey(config.rightFocusSourceDirectory) << std::endl;
		ok = false;
	}
	
	return ok;
	
	/*// Check to make sure arguments are valid
	if (PhotoDirectory.empty())
	{
		cout << "ERROR:  Photo directory not specified!  Use option '-dir' to specify!" << endl;
		exit(1);
	}

	if (OutputFileName.empty())
	{
		cout << "Warning:  Output file name not specified.  Use option '-o' to specify." << endl
			<< "Using default output filename:  output.jpg" << endl;
		OutputFileName.assign(_T("output.jpg"));
	}

	if (BigPictureFileName.empty())
	{
		cout << "ERROR:  Big Picture file name not specified!  Use option '-big-pic' to specify!" << endl;
		exit(1);
	}

	if (SubDivisionSize < 1)
	{
		cout << "ERROR:  Sub-division size is invalid!  Use option '-tilesize' to specify!" << endl;
		cout << "This is the number of pixels of the Big Picture that will " <<
			"become one sub-photo in the final image." << endl;
		exit(1);
	}

	if (SubPictureSize < 1)
	{
		cout << "ERROR:  Sub-photo size is invalid!  Use option '-subsize' to specify!" << endl;
		cout << "This is the size (width and height) to which each sub-photo will be resized." << endl;
		exit(1);
	}

	if (SubSamples < 1)
	{
		cout << "Warning:  Sub-samples not specified!  Use option '-subsamples' to specify!" << endl;
		cout << "Using default value 1 (one color sample per tile)." << endl;
		SubSamples = 1;
	}
	else if (SubSamples > SubDivisionSize)
	{
		cout << "Warning:  Invalid number of sub-samples specified!  Sub samples must be "
			<< "less or equal to sub-division size." << endl;
		cout << "Setting sub-samples to " << SubDivisionSize << "." << endl;
		SubSamples = SubDivisionSize;
	}*/
	
	return true;
}
