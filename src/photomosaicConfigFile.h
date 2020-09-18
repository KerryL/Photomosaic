/*===================================================================================
                                      Photomosaic
                          Copyright Kerry R. Loux 2009-2020

  This code is licensed under the MIT License (http://opensource.org/licenses/MIT).

===================================================================================*/

// File:  photomosaicConfigFile.h
// Auth:  K. Loux
// Date:  9/17/2020
// Desc:  Config file class for photomosaic application.

#ifndef PHOTOMOSAIC_CONFIG_FILE_H_
#define PHOTOMOSAIC_CONFIG_FILE_H_

// Local headers
#include "utilities/configFile.h"
#include "photomosaicConfig.h"

class PhotoMosaicConfigFile : public ConfigFile
{
public:
	PhotomosaicConfig config;
	
protected:
	void BuildConfigItems() override;
	void AssignDefaults() override;
	bool ConfigIsOK() override;
};

#endif// PHOTOMOSAIC_CONFIG_FILE_H_
