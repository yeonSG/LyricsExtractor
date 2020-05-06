#pragma once

#include <stdlib.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

enum PrintType	
{
	REDnBLUE,
	PINKnSKYBLUE
} typedef PrintType;

enum UnprintType
{
	WHITE,
	ORANGE
} typedef UnprintType;

class MVInformation
{
public:
	bool		m_isinfoCatched;
	PrintType	m_PrintType;
	UnprintType m_UnprintType;
	bool		m_TwinLine;

	MVInformation();
private:
	;
};