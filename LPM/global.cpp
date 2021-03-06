#include "stdafx.h"
#include "global.h"

boost::filesystem::path localPath, dataPath, langPath;
std::ostream &infoStream = std::cout;

std::string msgDataDefault[msgCount] = {
	"I:Updating source",
	"I:Package list refreshed",
	"I:Decompressing package",
	"I:Decompressed",
	"I:Copying files",
	"I:Files copied",
	"I:Deleting files",
	"I:Deleted",
	"I:Searching package",
	"I:Package found",
	"I:Installing package",
	"I:Package installed",
	"I:Package(s) installed",
	"I:Removing package",
	"I:Package removed",
	"I:Upgrading package",
	"I:Reinstalling",
	"I:Running installation script",
	"I:Running initialization script",
	"I:Running purge script",
	"I:Running removal script",
	"I:Done",
	"I:Checking requirement",
	"I:Will do these:",
	"I:Will remove these packages:",
	"I:Connecting",
	"I:Downloading data",
	"I:Data downloaded",
	"I:Backing up",
	"I:Backed up",
	"I:Recovering from backup",
	"I:Recovered from backup",
	"I:Found new version of some package. Use 'lpm upgrade' to promote an upgrade:",
	"I:All packages are up to date.",
	"I:The package requirement and confliction are listed below:",
	"I:You may can use 'lpm --force COMMAND' to continue.",
	"I:Package stack trace:",
	"I:Using --lpmdir:",
	"I:Using --local:",
	"E:Incorrect pack info from source",
	"E:Script exited with code ",
	"E:Overwriting",
	"E:Illegal package",
	"E:Confliction found",
	"E:Invalid dependence",
	"E:Found a dependency which is impossible to implement",
	"E:Package not found",
	"E:Could not find specificed package. This package have dependency problem",
	"E:Package already installed",
	"E:Package not installed",
	"E:Needn't upgrade",
	"E:This package is depended by other package",
	"E:Dependence info not found",
	"E:Backup already exists",
	"E:Backup not found",
	"E:filesystem:",
	"E:network:",
	"E:unzip:Broken File",
	"E:unzip:inflateInit2 failed with code ",
	"E:unzip:inflate failed with code ",
	"E:unzip:inflateEnd failed with code ",
	"E:unzip:Memory overflow",
	"E:unzip:Buffer not enough",
	"E:STD:",
	"E:Failed to lock",
	"W:Conflict Package",
	"W:Script exited with code ",
	", rolling back",
	"W:This package is depended by:",

	"Install",
	"Upgrade",
	"Remove",
};
std::string msgData[msgCount];

fProgressReportCallback prCallbackP = NULL;

char* str2cstr(std::string arg)
{
	char* ret = new char[arg.size() + 1];
#ifdef _MSC_VER
	strcpy_s(ret, arg.size() + 1, arg.c_str());
#else
	strcpy(ret, arg.c_str());
#endif
	return ret;
}

void processEscChar(std::string &str)
{
	std::string::iterator itr = str.begin();
	for (; itr != str.end(); itr++)
	{
		if (*itr == '\\')
		{
			char replace ='\0';
			itr = str.erase(itr);
			if (itr == str.end())
				break;
			switch (*itr)
			{
				case 'a':
					replace = '\a';
					break;
				case 'b':
					replace = '\b';
					break;
				case 'f':
					replace = '\f';
					break;
				case 'n':
					replace = '\n';
					break;
				case 'r':
					replace = '\r';
					break;
				case 't':
					replace = '\t';
					break;
				case 'v':
					replace = '\v';
					break;
				case '\\':
					replace = '\\';
					break;
				case '\'':
					replace = '\'';
					break;
				case '\"':
					replace = '\"';
					break;
				case 'x':
				{
					std::stringstream tmp;
					tmp << std::hex;
					for (int i = 0; i < 2 && itr != str.end(); i++)
					{
						itr = str.erase(itr);
						if (!isxdigit(*itr))
							break;
						tmp << *itr;
					}
					itr = str.insert(itr, '\0');
					int tmpn = 0;
					tmp >> tmpn;
					replace = tmpn;
					break;
				}
				default:
				{
					if (*itr > '7' || *itr < '0')
						return;
					std::stringstream tmp;
					tmp << std::oct;
					for (int i = 0; i < 3 && itr != str.end(); i++)
					{
						if (*itr > '7' || *itr < '0')
							break;
						tmp << *itr;
						itr = str.erase(itr);
					}
					itr = str.insert(itr, '\0');
					int tmpn = 0;
					tmp >> tmpn;
					replace = tmpn;
				}
			}

			*itr = replace;
		}
	}
}
