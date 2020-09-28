/*===================================================================================
                                      Photomosaic
                          Copyright Kerry R. Loux 2009-2020

  This code is licensed under the MIT License (http://opensource.org/licenses/MIT).

===================================================================================*/

// File:  main.cpp
// Auth:  K. Loux
// Date:  6/21/2009
// Desc:  Creates a photomosaic when supplied with a directory containing
// source images, the filename of the "big picture," the output file name, the size
// to make each sub-picture, and the size of each sub-square of the big picture.

// Local headers
#include "photomosaicConfigFile.h"
#include "photomosaic.h"

// Standard C++ headers
#include <iostream>

// wxWidgets headers
#include <wx/app.h>

void ReportConfiguration(const PhotomosaicConfig& config)
{
	std::cout << "\nUsing photos from:\n  Center-focused:  " << config.centerFocusSourceDirectory
		<< "\n  Left-focused:  " << config.leftFocusSourceDirectory
		<< "\n  Right-focused:  " << config.rightFocusSourceDirectory << '\n';
	if (config.recursiveSourceDirectories)
		std::cout << "  (and sub-directories)";
		
	std::cout << "Target image is " << config.targetImageFileName << "\n\n"
		<< "Sub-photos will be rescaled to " << config.thumbnailSize << " pixels square, and will replace "
		<< config.subDivisionSize << " square blocks of the original image\n\n"
		<< "Images will be color sampled " << config.subSamples * config.subSamples << " times\n\n";

	if (config.greyscaleOutput)
		std::cout << "Output image will be greyscale\n";

	if (!config.thumbnailDirectory.empty())
		std::cout << "Thumbnail directory is '" << config.thumbnailDirectory << "'\n";
		
	std::cout << std::endl;
}

int main(int argc, char *argv[])
{
	if (!wxInitialize())
		return 1;

	if (argc != 2)
	{
		std::cout << "Usage:  " << argv[0] << " <config file name>" << std::endl;
		return 1;
	}
	
	PhotoMosaicConfigFile configFile;
	if (!configFile.ReadConfiguration(UString::ToStringType(argv[1])))
	{
		wxUninitialize();
		return 1;
	}

	ReportConfiguration(configFile.config);
	wxInitAllImageHandlers();

	Photomosaic photomosaic(configFile.config);
	wxImage mosaic(photomosaic.Build());

	if (!mosaic.SaveFile(configFile.config.outputFileName))
	{
		std::cerr << "Failed to write image to '" << configFile.config.outputFileName << "'\n";
		wxUninitialize();
		return 1;
	}
	
	wxUninitialize();
	return 0;
}
