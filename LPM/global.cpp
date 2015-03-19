#include "stdafx.h"
#include "global.h"

boost::filesystem::path localPath, dataPath;
#ifdef _LPM_GUI
std::stringstream infoStream;
#else
std::ostream &infoStream = std::cout;
std::ostream& myEndl(std::ostream& os)
{
	os << std::endl;
	return os;
}
#endif
