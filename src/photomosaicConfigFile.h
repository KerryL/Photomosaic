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
	
	bool IsSpecified(const std::string& s);
	template<typename T>
	bool IsStrictlyPositive(const T& t);
	template<typename T>
	bool IsPositive(const T& t);
};

template<typename T>
bool PhotoMosaicConfigFile::IsStrictlyPositive(const T& t)
{
	if (t <= 0)
	{
		outStream << GetKey(t) << " must be strictly positive" << std::endl;
		return false;
	}
	
	return true;
}

template<typename T>
bool PhotoMosaicConfigFile::IsPositive(const T& t)
{
	if (t < 0)
	{
		outStream << GetKey(t) << " must be positive" << std::endl;
		return false;
	}
	
	return true;
}

#endif// PHOTOMOSAIC_CONFIG_FILE_H_
