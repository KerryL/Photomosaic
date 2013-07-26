/*===================================================================================
                                      Photomosaic
                          Copyright Kerry R. Loux 2009-2013

  This code is licensed under the MIT License (http://opensource.org/licenses/MIT).

===================================================================================*/

// File:  main.cpp
// Author:  K. Loux
// Date:  6/21/2009
// Description:  Creates a photomosaic when supplied with a directory containing
// source images, the filename of the "big picture," the output file name, the size
// to make each sub-picture, and the size of each sub-square of the big picture.

// Standard C++ headers
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <time.h>

// wxWidgets headers
#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/file.h>
#include <wx/image.h>

// Color information structure
struct COLOR_INFO
{
	int AverageRed;
	int AverageGreen;
	int AverageBlue;
};

// Prototypes
void GetColorInformation(const wxImage &Image, COLOR_INFO *Info, const int &SubSamples, const bool &MakeGrey);
void SetBestLocation(int **BigPictureMap, const int &NumberXPhotos, const int &NumberYPhotos,
					 const COLOR_INFO *TileInfo, const COLOR_INFO *const *const *BigPictureColorInfo,
					 const int &SubSamples, const int &Index);
int FindBestMatch(const COLOR_INFO *const *AvailableColors, const int &NumberOfColors,
				  const COLOR_INFO *DesiredColor, const int &SubSamples,
				  const int *UsedIndecies, const int &NumberOfUsedTiles, bool NoRepeats);
void CopyTileToOutput(const wxImage &Tile, const int &TileSize, unsigned char *OutputData,
					  const int &Width, const int &XTile, const int &YTile);
void ComputeRelativeError(const wxImage &Original, const wxImage &Final);

using namespace std;

int main(int argc, char *argv[])
{
	//==============================================================================
	//================= INITIALIZATION AND COMMAND LINE ARGUMENTS ==================
	//==============================================================================

	wxString PhotoDirectory = wxEmptyString;
	wxString BigPictureFileName = wxEmptyString;
	wxString OutputFileName  = wxEmptyString;
	wxString ThumbnailDirectory = wxEmptyString;
	int SubPictureSize = 0;
	int SubDivisionSize = 0;
	int SubSamples = 0;
	bool Recursive = false;
	bool NoRepeats = false;
	bool Greyscale = false;
	bool GreyScore = false;
	bool Seeded = false;

	cout << endl;

	// Parse arguments
	int i;
	wxString Command;
	for (i = 1; i < argc; i++)
	{
		Command.assign(argv[i]);
		if (Command.compare(_T("-dir")) == 0)
		{
			PhotoDirectory.assign(argv[i + 1]);
			i++;
		}
		else if (Command.compare(_T("-o")) == 0)
		{
			OutputFileName.assign(argv[i + 1]);
			i++;
		}
		else if (Command.compare(_T("-big-pic")) == 0)
		{
			BigPictureFileName.assign(argv[i + 1]);
			i++;
		}
		else if (Command.compare(_T("-tilesize")) == 0)
		{
			SubDivisionSize = atoi(argv[i + 1]);
			i++;
		}
		else if (Command.compare(_T("-subsize")) == 0)
		{
			SubPictureSize = atoi(argv[i + 1]);
			i++;
		}
		else if (Command.compare(_T("-subsamples")) == 0)
		{
			SubSamples = atoi(argv[i + 1]);
			i++;
		}
		else if (Command.compare(_T("-seed")) == 0)
		{
			cout << "Seeding with " << atoi(argv[i + 1]) << endl;
			srand(atoi(argv[i + 1]));
			Seeded = true;
			i++;
		}
		else if (Command.CmpNoCase(_T("-r")) == 0)
			Recursive = true;
		else if (Command.compare(_T("-no-repeats")) == 0)
			NoRepeats = true;
		else if (Command.compare(_T("-grey")) == 0)
			Greyscale = true;
		else if (Command.compare(_T("-greyscore")) == 0)
			GreyScore = true;
		else if (Command.compare(_T("-save-thumbs")) == 0)
		{
			ThumbnailDirectory.assign(argv[i + 1]);
			i++;
		}
		else
			cout << "Unrecognized argument: " << argv[i] << "!  Ignoring..." << endl;
	}

	// If we haven't already done so, seed the random number generator
	if (!Seeded)
		srand(time(0));

	// Check to make sure arguments are valid
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
	}

	// Make sure the photo directory exists
	if (!wxDir::Exists(PhotoDirectory))
	{
		cout << "ERROR:  The directory '" << PhotoDirectory << "' does not exist!" << endl;
		exit(1);
	}

	wxString BigPicturePathAndFileName;
	if (BigPictureFileName.Find(_T('/')) == wxNOT_FOUND
		&& BigPictureFileName.Find(_T('\\')) == wxNOT_FOUND)
	{
		BigPicturePathAndFileName = PhotoDirectory + _T('/') + BigPictureFileName;
		if (!wxFile::Exists(BigPicturePathAndFileName))
		{
			cout << "ERROR:  The big picture file '" << BigPictureFileName
				<< "' does not exist in the directory '" << PhotoDirectory << "'." << endl;
			exit(1);
		}
	}
	else
	{
		BigPicturePathAndFileName = BigPictureFileName;
		if (!wxFile::Exists(BigPicturePathAndFileName))
		{
			cout << "ERROR:  The big picture file '" << BigPictureFileName
				<< "' does not exist!" << endl;
			exit(1);
		}
	}

	if (!ThumbnailDirectory.empty())
	{
		if (!wxDir::Exists(ThumbnailDirectory))
		{
			cout << "ERROR:  Thumbnail directory '" << ThumbnailDirectory
				<< "' does not exist!" << endl;
			exit(1);
		}
	}

	// Tell the user what we're going to attempt
	cout << endl;
	cout << "Using photos from " << PhotoDirectory;
	if (Recursive)
		cout << " (and sub-directories)";
	cout << " to re-create " << BigPictureFileName << "." << endl << endl;
	cout << "Sub-photos will be rescaled to " << SubPictureSize << " by " << SubPictureSize <<
		" square, and will replace " << SubDivisionSize << " by " << SubDivisionSize <<
		" square blocks of the original image." << endl << endl;
	cout << "Images will be color sampled " << SubSamples * SubSamples << " times."
		<< endl << endl;

	if (Greyscale)
		cout << "Images will be converted to greyscale." << endl;

	if (GreyScore)
		cout << "Images will be converted to greyscale, only for determining fit." << endl;

	if (!ThumbnailDirectory.empty())
		cout << "Thumbnails will be saved to '" << ThumbnailDirectory << "' after resizing." << endl << endl;

	//==============================================================================
	//======== CONVERT IMAGES TO BITMAP FORMAT AND GET COLOR INFORMATION ===========
	//==============================================================================

	// Load wx image handlers
	wxImage::AddHandler(new wxBMPHandler);// Not needed?!
	wxImage::AddHandler(new wxJPEGHandler);

	int FileSearchFlags;
	if (Recursive)
		FileSearchFlags = wxDIR_FILES | wxDIR_DIRS;
	else
		FileSearchFlags = wxDIR_FILES;

	// Find all of the bitmap images
	wxArrayString ImageList;
	int NumberOfPictureFiles = (int)wxDir::GetAllFiles(PhotoDirectory, &ImageList,
		_T("*.bmp"), FileSearchFlags);

	// Find all of the jpeg images
	NumberOfPictureFiles += (int)wxDir::GetAllFiles(PhotoDirectory, &ImageList,
		_T("*.jpg"), FileSearchFlags);

	// Find all of the jpeg images (with capitalized extensions)
	NumberOfPictureFiles += (int)wxDir::GetAllFiles(PhotoDirectory, &ImageList,
		_T("*.JPG"), FileSearchFlags);

	if (NumberOfPictureFiles <= 0)
	{
		cout << "ERROR:  No picture files found in '" << PhotoDirectory << "'!" << endl;
		exit(1);
	}

	cout << "Found " << NumberOfPictureFiles << " image files!" << endl;

	// Load the big picture image
	wxImage BigPicture(BigPicturePathAndFileName);

	// Figure out how to break up the BigPicture into sub-pictures
	int NumberXPhotos = BigPicture.GetWidth() / SubDivisionSize;
	int NumberYPhotos = BigPicture.GetHeight() / SubDivisionSize;

	cout << "Mosaic will require " << NumberXPhotos * NumberYPhotos << " tiles!" << endl;

	if (NoRepeats)
		cout << "Repeats are NOT allowed!" << endl;

	if (NoRepeats && NumberOfPictureFiles < NumberXPhotos * NumberYPhotos)
	{
		cout << "ERROR:  Not enough picture files to avoid repeats!" << endl;
		exit(1);
	}

	// Do the pre-processing procedure for all of the images in the directory
	COLOR_INFO **ColorInfoArray = new COLOR_INFO*[NumberOfPictureFiles];
	wxImage *Photo = new wxImage[NumberOfPictureFiles];
	cout << endl << "Converting images to bitmaps and rescaling to "
		<< SubPictureSize << " by " << SubPictureSize;
	wxRect SubSquare(0, 0, 0, 0);
	for (i = 0; i < NumberOfPictureFiles; i++)
	{
		cout << ".";

		// Allocate the array for sub-samples
		ColorInfoArray[i] = new COLOR_INFO[SubSamples * SubSamples];

		// Load the next image as a wxImage object
		Photo[i].LoadFile(ImageList[i]);

		// Rescale the image so it is SubPictureSize by SubPictureSize square
		// To avoid distoring the image, use GetSubImage to get the middle square
		// of the picture.  This may have unwanted affects (crops indescriminantly).
		if (Photo[i].GetHeight() != Photo[i].GetWidth())
		{
			if (Photo[i].GetHeight() > Photo[i].GetWidth())
			{
				SubSquare.SetX(0);
				SubSquare.SetY((Photo[i].GetHeight() - Photo[i].GetWidth()) / 2);
				SubSquare.SetHeight(Photo[i].GetWidth());
				SubSquare.SetWidth(Photo[i].GetWidth());
			}
			else
			{
				SubSquare.SetX((Photo[i].GetWidth() - Photo[i].GetHeight()) / 2);
				SubSquare.SetY(0);
				SubSquare.SetHeight(Photo[i].GetHeight());
				SubSquare.SetWidth(Photo[i].GetHeight());
			}
			Photo[i] = Photo[i].GetSubImage(SubSquare);
		}

		// Convert to greyscale (if specified)
		if (Greyscale)
			Photo[i] = Photo[i].ConvertToGreyscale();

		// Only re-scale if necessary
		if (Photo[i].GetHeight() != SubPictureSize || Photo[i].GetWidth() != SubPictureSize)
		{
			Photo[i].Rescale(SubPictureSize, SubPictureSize, wxIMAGE_QUALITY_HIGH);

			// Save the image to the thumbnail directory (if specified)
			if (!ThumbnailDirectory.empty())
#ifdef __WXGTK__
				Photo[i].SaveFile(ThumbnailDirectory + _T('/') +
					ImageList[i].Mid(ImageList[i].Last(_T('/'))));
#else
				Photo[i].SaveFile(ThumbnailDirectory + _T('/') +
					ImageList[i].Mid(ImageList[i].Last(_T('\\'))));
#endif
		}

		// Get the color information
		GetColorInformation(Photo[i], ColorInfoArray[i], SubSamples, GreyScore);
	}
	cout << endl << endl;

	//==============================================================================
	//================ DECODE PHOTOS AND STORE COLOR INFORMATION ===================
	//==============================================================================

	cout << "Analyzing the Big Picture" << endl;

	int **BigPictureMap = new int*[NumberXPhotos];
	COLOR_INFO ***BigPictureColorInfo;
	BigPictureColorInfo = new COLOR_INFO**[NumberXPhotos];
	int j;
	for (i = 0; i < NumberXPhotos; i++)
	{
		BigPictureMap[i] = new int[NumberYPhotos];
		BigPictureColorInfo[i] = new COLOR_INFO*[NumberYPhotos];

		for (j = 0; j < NumberYPhotos; j++)
		{
			BigPictureMap[i][j] = -1;
			BigPictureColorInfo[i][j] = new COLOR_INFO[SubSamples * SubSamples];
		}
	}

	cout << "Determining desired tile colors";
	// Go through all of the sub-pictures and generate the "desired" color information for each one
	wxRect SubRect(0, 0, SubDivisionSize, SubDivisionSize);
	for (i = 0; i < NumberXPhotos; i++)
	{
		SubRect.SetX(i * SubDivisionSize);
		for (j = 0; j < NumberYPhotos; j++)
		{
			cout << ".";
			SubRect.SetY(j * SubDivisionSize);
			GetColorInformation(BigPicture.GetSubImage(SubRect), BigPictureColorInfo[i][j], SubSamples, GreyScore || Greyscale);
		}
	}
	cout << endl << endl;

	//==============================================================================
	//================ GENERATE OUTPUT IMAGE AND APPLY COMPRESSION =================
	//==============================================================================

	// Create an array in which we can store indecies as we use them (to avoid repeats)
	int NumberOfTiles = NumberXPhotos * NumberYPhotos;
	int NumberOfUsedIndecies = 0;
	int *UsedIndecies = new int[NumberOfTiles];
	for (i = 0; i < NumberOfTiles; i++)
		UsedIndecies[i] = -1;

	// Find the "best match" image based on the desired and available color information
	// Store the actual color info in the output array
	cout << "Determining best matches from available photos";

	for (i = 0; i < NumberXPhotos; i++)
	{
		for (j = 0; j < NumberYPhotos; j++)
		{
			cout << ".";

			// If there are no more tiles to assign, then stop trying
			//if (NumberOfUsedIndecies

			// Find the best match
			if (BigPictureMap[i][j] == -1)
			{
				BigPictureMap[i][j] = FindBestMatch(ColorInfoArray,
					NumberOfPictureFiles, BigPictureColorInfo[i][j], SubSamples,
					UsedIndecies, NumberOfUsedIndecies, NoRepeats);

				// If we can't repeat any tiles, then make sure that this tile
				// doesn't fit better somewhere else
				if (NoRepeats)
				{
					UsedIndecies[NumberOfUsedIndecies] = BigPictureMap[i][j];
					BigPictureMap[i][j] = -1;
					SetBestLocation(BigPictureMap, NumberXPhotos, NumberYPhotos,
						ColorInfoArray[UsedIndecies[NumberOfUsedIndecies]], BigPictureColorInfo,
						SubSamples, UsedIndecies[NumberOfUsedIndecies]);

					j--;
					NumberOfUsedIndecies++;
				}
			}
		}
	}
	cout << endl << endl;

	// Generate the output image
	wxImage OutputImage(NumberXPhotos * SubPictureSize, NumberYPhotos * SubPictureSize);

	cout << "Copying tiles into image";
	for (i = 0; i < NumberXPhotos; i++)
	{
		for (j = 0; j < NumberYPhotos; j++)
			CopyTileToOutput(Photo[BigPictureMap[i][j]],
				SubPictureSize, OutputImage.GetData(), OutputImage.GetWidth(), i, j);
	}

	cout << endl << endl;

	// Set some options specifically for the save the date stuff
	// I'm leaving these here because I'm too lazy to create a formal
	// option for them, but it should be done eventually, and I don't
	// want to forget how they work
	/*cout << OutputImage.GetOption(_T("quality")) << endl;
	cout << OutputImage.GetOption(_T("Resolution")) << endl;
	cout << OutputImage.GetOption(_T("ResolutionUnit")) << endl;*/
	OutputImage.SetOption(_T("quality"), 100);
	OutputImage.SetOption(_T("ResolutionUnit"), wxIMAGE_RESOLUTION_INCHES);
	OutputImage.SetOption(_T("Resolution"), OutputImage.GetWidth() / 8);//*/
//	cout << "after set: " << OutputImage.GetOption(_T("ResolutionUnit")) << endl;

	// Write the output image to file
	cout << "Saving image to '" << OutputFileName << "'" << endl;
	// Parse the file name to determine the correct type
	int FileType;
	if (OutputFileName.Mid(OutputFileName.Find(_T('.'), true)).CmpNoCase(_T(".jpg")) ||
		OutputFileName.Mid(OutputFileName.Find(_T('.'), true)).CmpNoCase(_T(".jpeg")))
		FileType = wxBITMAP_TYPE_JPEG;
	else
		FileType = wxBITMAP_TYPE_BMP;
	OutputImage.SaveFile(OutputFileName, FileType);

	// Compute the relative error (adding which color will make the error reduce the most?)
	ComputeRelativeError(BigPicture, OutputImage);

	// Free dynamically allocated memory
	for (i = 0; i < NumberOfPictureFiles; i++)
	{
		delete [] ColorInfoArray[i];
		ColorInfoArray[i] = NULL;
	}
	delete [] ColorInfoArray;
	ColorInfoArray = NULL;

	delete [] Photo;
	Photo = NULL;

	for (i = 0; i < NumberXPhotos; i++)
	{
		delete [] BigPictureMap[i];
		BigPictureMap[i] = NULL;

		for (j = 0; j < NumberYPhotos; j++)
		{
			delete [] BigPictureColorInfo[i][j];
			BigPictureColorInfo[i][j] = NULL;
		}
		delete [] BigPictureColorInfo[i];
		BigPictureColorInfo[i] = NULL;
	}
	delete [] BigPictureColorInfo;
	BigPictureColorInfo = NULL;
	delete [] BigPictureMap;
	BigPictureMap = NULL;

	delete [] UsedIndecies;
	UsedIndecies = NULL;

	return 0;
}

void GetColorInformation(const wxImage &Image, COLOR_INFO *Info, const int &SubSamples, const bool &MakeGrey)
{
	wxImage SubImage;
	int XStep = Image.GetWidth() / SubSamples;
	int YStep = Image.GetHeight() / SubSamples;
	wxRect SubSize(0, 0, XStep, YStep);
	int i, j, Sample;
	for (Sample = 0; Sample < SubSamples * SubSamples; Sample++)
	{
		Info[Sample].AverageRed = 0;
		Info[Sample].AverageGreen = 0;
		Info[Sample].AverageBlue = 0;

		// Get the sub-sample from the Image
		SubSize.SetX(int(Sample / SubSamples) * XStep);
		SubSize.SetY((Sample - int(Sample / SubSamples) * SubSamples) * YStep);
		SubImage = Image.GetSubImage(SubSize);

		// Convert to greyscale, if necessary for scoring
		if (MakeGrey)
			SubImage = SubImage.ConvertToGreyscale();

		// Get the average red, green, and blue for the image
		for (i = 0; i < SubImage.GetWidth(); i++)
		{
			for (j = 0; j < SubImage.GetHeight(); j++)
			{
				Info[Sample].AverageRed += (int)SubImage.GetRed(i, j);
				Info[Sample].AverageGreen += (int)SubImage.GetGreen(i, j);
				Info[Sample].AverageBlue += (int)SubImage.GetBlue(i, j);
			}
		}

		Info[Sample].AverageRed /= i * j;
		Info[Sample].AverageGreen /= i * j;
		Info[Sample].AverageBlue /= i * j;
	}

	return;
}

bool IndexAlreadyUsed(const int &Index, const int *UsedIndecies, int NumberOfTiles)
{
	int i;
	for (i = 0; i < NumberOfTiles; i++)
	{
		if (UsedIndecies[i] == Index)
			return true;
		else if (UsedIndecies[i] == -1)
			return false;
	}

	return false;
}

unsigned long GetColorError(const COLOR_INFO *DesiredColor,
							const COLOR_INFO *AvailableColors, const int &SubSamples)
{
	// Error for this color is the sum of the errors of each sub-sample
	int i;
	unsigned long Error = 0;
	for (i = 0; i < SubSamples * SubSamples; i++)
		Error += (DesiredColor[i].AverageRed - AvailableColors[i].AverageRed) *
			(DesiredColor[i].AverageRed - AvailableColors[i].AverageRed) +
			(DesiredColor[i].AverageGreen - AvailableColors[i].AverageGreen) *
			(DesiredColor[i].AverageGreen - AvailableColors[i].AverageGreen) +
			(DesiredColor[i].AverageBlue - AvailableColors[i].AverageBlue) *
			(DesiredColor[i].AverageBlue - AvailableColors[i].AverageBlue);

	return Error;
}

void SetBestLocation(int **BigPictureMap, const int &NumberXPhotos, const int &NumberYPhotos,
					 const COLOR_INFO *TileInfo, const COLOR_INFO *const *const *BigPictureColorInfo,
					 const int &SubSamples, const int &Index)
{
	// If this is called, it's because the TileInfo specified is a best match for
	// somewhere in the big picture.  We need to now go about it the other way and
	// find the best location in the picture for this tile (we want the best possible
	// image while also avoiding repeats)
	int i, j;
	unsigned long MaximumError = 255 * 255 * 3 * SubSamples * SubSamples;
	unsigned long **Error = new unsigned long*[NumberXPhotos];
	for (i = 0; i < NumberXPhotos; i++)
		Error[i] = new unsigned long[NumberYPhotos];

	// Get the errors for this tile at every possible location
	// If it could work equally well at other locations, we'll
	// store that information in a vector and choose one at
	// random to avoid favoring one side of the image.
	for (i = 0; i < NumberXPhotos; i++)
	{
		for (j = 0; j < NumberYPhotos; j++)
		{
			if (BigPictureMap[i][j] == -1)
				Error[i][j] = GetColorError(BigPictureColorInfo[i][j], TileInfo, SubSamples);
			else
				Error[i][j] = MaximumError + 1;
		}
	}

	// Find the minimum error
	unsigned long MinimumError = MaximumError + 1;
	vector<pair<int, int> > BestLocations;
	pair<int, int> Temp;
	for (i = 0; i < NumberXPhotos; i++)
	{
		for (j = 0; j < NumberYPhotos; j++)
		{
			if (Error[i][j] < MinimumError)
			{
				// Empty the vector and start fresh
				BestLocations.clear();
				MinimumError = Error[i][j];
			}

			if (Error[i][j] == MinimumError)
			{
				Temp.first = i;
				Temp.second = j;
				BestLocations.push_back(Temp);
			}
		}
	}

	// Randomize the order of the best locations
	random_shuffle(BestLocations.begin(), BestLocations.end());

	// Assign the best location to this image
	BigPictureMap[BestLocations[0].first][BestLocations[0].second] = Index;

	for (i = 0; i < NumberXPhotos; i++)
	{
		delete [] Error[i];
		Error[i] = NULL;
	}
	delete [] Error;
	Error = NULL;

	return;
}

int FindBestMatch(const COLOR_INFO *const *AvailableColors, const int &NumberOfColors,
				  const COLOR_INFO *DesiredColor, const int &SubSamples,
				  const int *UsedIndecies, const int &NumberOfUsedTiles, bool NoRepeats)
{
	// Calculate the error on a least-squares basis of the RGB values
	unsigned long *Error = new unsigned long[NumberOfColors];
	unsigned long MaximumError = 255 * 255 * 3 * SubSamples * SubSamples;
	int i;
	for (i = 0; i < NumberOfColors; i++)
	{
		// Don't check excluded tiles
		if (IndexAlreadyUsed(i, UsedIndecies, NumberOfUsedTiles) && NoRepeats)
			Error[i] = MaximumError + 1;
		else
			Error[i] = GetColorError(DesiredColor, AvailableColors[i], SubSamples);
	}

	// Find the minimum error
	unsigned long MinimumError = MaximumError + 1;
	int MinimumErrorIndex = NumberOfColors + 1;
	for (i = 0; i < NumberOfColors; i++)
	{
		if (Error[i] < MinimumError)
		{
			MinimumError = Error[i];
			MinimumErrorIndex = i;
		}
	}

	// Free the dynamic memory
	delete [] Error;
	Error = NULL;

	return MinimumErrorIndex;
}

void CopyTileToOutput(const wxImage &Tile, const int &TileSize, unsigned char *OutputData,
					  const int &Width, const int &XTile, const int &YTile)
{
	int i, j;
	int TileTop = YTile * TileSize;
	int TileLeft = XTile * TileSize;

	cout << ".";
	for (i = 0; i < TileSize; i++)
	{
		for (j = 0; j < TileSize; j++)
		{
			OutputData[(TileLeft + i + (TileTop + j) * Width) * 3] = Tile.GetRed(i, j);
			OutputData[(TileLeft + i + (TileTop + j) * Width) * 3 + 1] = Tile.GetGreen(i, j);
			OutputData[(TileLeft + i + (TileTop + j) * Width) * 3 + 2] = Tile.GetBlue(i, j);
		}
	}

	return;
}

void ComputeRelativeError(const wxImage &Original, const wxImage &Final)
{
	// Rescale the output image to match the original's size
	wxImage FinalRescaled = Final.Scale(Original.GetWidth(), Original.GetHeight(), wxIMAGE_QUALITY_HIGH);

	// Go through every pixel and add up the red, green, and blue errors
	unsigned long RedError = 0, GreenError = 0, BlueError = 0;
	int i, j;
	for (i = 0; i < Original.GetWidth(); i++)
	{
		for (j = 0; j < Original.GetHeight(); j++)
		{
			RedError += Original.GetRed(i, j) - FinalRescaled.GetRed(i, j);
			GreenError += Original.GetGreen(i, j) - FinalRescaled.GetGreen(i, j);
			BlueError += Original.GetBlue(i, j) - FinalRescaled.GetBlue(i, j);
		}
	}

	// Print the results
	cout << endl << "Relative Errors:" << endl;
	cout << "Red:  " << RedError << endl;
	cout << "Green:  " << GreenError << endl;
	cout << "Blue:  " << BlueError << endl;

	return;
}
