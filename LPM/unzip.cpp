/*
Live Package Manager, Package Manager for LBLive
Copyright (C) <2015>  <xy12423>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "stdafx.h"
#include "unzip.h"

#define skip(n)	fin.seekg(n, std::ios_base::cur);
#define readUINT(var)	fin.read(reinterpret_cast<char*>(&(var)), 4);                    \
						if (fin.eof())                                                   \
							return errInfo(msgData[MSGE_UNZIP_BROKEN]);
#define readBYTE(var)	fin.read(reinterpret_cast<char*>(&(var)), 2);                    \
						if (fin.eof())                                                   \
							return errInfo(msgData[MSGE_UNZIP_BROKEN]);

errInfo unzip(std::string fPath, boost::filesystem::path path)
{
	UINT i;
	std::ifstream fin(fPath, std::ios_base::in | std::ios_base::binary);
	std::ofstream fout;
	ULONGLONG sizeAll = 0, sizeInflated = 0;
	double progress = 0;
	if (prCallbackP != NULL)
	{
		(*prCallbackP)(0, 0);
		while (!fin.eof())
		{
			UINT head = 0;
			fin.read(reinterpret_cast<char*>(&head), 4);
			if (fin.eof())
				break;
			if (head != 0x04034b50)
				break;
			skip(14);
			UINT fileSize, fileOriginSize;
			readUINT(fileSize);
			readUINT(fileOriginSize);
			sizeAll += fileOriginSize;
			USHORT nameLen, extLen;
			readBYTE(nameLen);
			readBYTE(extLen);
			ULONGLONG skipLen = static_cast<ULONGLONG>(fileSize) + nameLen + extLen;
			skip(skipLen);
		}
		fin.close();
		fin.open(fPath, std::ios_base::in | std::ios_base::binary);
	}
	while (!fin.eof())
	{
		UINT head = 0;
		fin.read(reinterpret_cast<char*>(&head), 4);
		if (fin.eof())
			break;
		if (head == 0x08074b50)
		{
			skip(12);
			continue;
		}
		else if (head == 0x02014b50)
		{
			skip(24);
			USHORT nameLen, extLen, commentLen;
			readBYTE(nameLen);
			readBYTE(extLen);
			readBYTE(commentLen);
			skip(4);
			UINT fileAttribute;
			readUINT(fileAttribute);
			skip(4);
			std::string name;
			{
				char *nameBuf = new char[nameLen];
				fin.read(nameBuf, nameLen);
				if (fin.eof())
				{
					delete[] nameBuf;
					return errInfo(msgData[MSGE_UNZIP_BROKEN]);
				}
				name = std::string(nameBuf, nameLen);
				delete[] nameBuf;
			}
			skip(extLen + commentLen);

			if (!(fileAttribute & DIR_ATTRIBUTE) && fs::is_directory(path / name))
			{
				fs::remove(path / name);
				fout.open((path / name).string());
				fout.close();
			}

			continue;
		}
		else if (head != 0x04034b50)
			break;
		skip(2);
		USHORT flag = 0, compMethod = 0;
		readBYTE(flag);
		readBYTE(compMethod);
		skip(8);
		UINT fileSize, fileOriginSize;
		readUINT(fileSize);
		readUINT(fileOriginSize);
		USHORT nameLen, extLen;
		readBYTE(nameLen);
		readBYTE(extLen);
		std::string name;
		{
			char *nameBuf = new char[nameLen];
			fin.read(nameBuf, nameLen);
			if (fin.eof())
			{
				delete[] nameBuf;
				return errInfo(msgData[MSGE_UNZIP_BROKEN]);
			}
			name = std::string(nameBuf, nameLen);
			delete[] nameBuf;
		}
		skip(extLen);
		if (fileSize == 0)
		{
			boost::filesystem::create_directories(path / name);
		}
		else
		{
			boost::filesystem::path filePath = path / name;
			fout.open(filePath.string(), std::ios::out | std::ios::binary);

			switch (compMethod)
			{
				case 0:
				{
					char* buf = new char[fileSize];
					try
					{
						fin.read(buf, fileSize);
						if (fin.eof())
							return errInfo(msgData[MSGE_UNZIP_BROKEN]);
						fout.write(buf, fileSize);
						sizeInflated += fileSize;
					}
					catch (...)
					{
						delete[] buf;
						throw;
					}
					delete[] buf;
					if (prCallbackP != NULL)	//Need report
					{
						progress = static_cast<double>(sizeInflated) * 100 / sizeAll;
						(*prCallbackP)(progress, sizeInflated);
					}
					break;
				}
				case 8:
					const int blockSize = 0x100000;

					BYTE *dataBuf = new BYTE[fileSize];
					BYTE *fileBuf = new BYTE[blockSize];

					try
					{
						fin.read(reinterpret_cast<char*>(dataBuf), fileSize);
						if (fin.eof())
							return errInfo(msgData[MSGE_UNZIP_BROKEN]);

						z_stream zstream;
						zstream.zalloc = static_cast<alloc_func>(Z_NULL);
						zstream.zfree = static_cast<free_func>(Z_NULL);
						zstream.opaque = 0;
						zstream.next_in = NULL;
						zstream.avail_in = 0;
						int err = inflateInit2(&zstream, -15);
						if (err != Z_OK)
						{
							throw(errInfo(msgData[MSGE_UNZIP_INFLATEINIT] + std::to_string(err)));
						}
						zstream.next_in = dataBuf;
						zstream.avail_in = fileSize;

						while (true)
						{
							zstream.next_out = fileBuf;
							zstream.avail_out = blockSize;
							err = inflate(&zstream, Z_NO_FLUSH);
							if (err == Z_OK || err == Z_STREAM_END)
							{
								BYTE *ptr = fileBuf;
								UINT inflatedSize = blockSize - zstream.avail_out;
								for (i = 0; i < inflatedSize; i++)
								{
									fout.put(*ptr);
									ptr++;
								}
								if (prCallbackP != NULL)
								{
									sizeInflated += inflatedSize;
									progress = static_cast<double>(sizeInflated) * 100 / sizeAll;
									(*prCallbackP)(progress, sizeInflated);
								}
								if (err == Z_STREAM_END || (err == Z_OK && zstream.avail_in == 0))
								{
									if ((err = inflateEnd(&zstream)) == Z_OK)
										break;
									else
									{
										switch (err)
										{
											case Z_MEM_ERROR:
												throw(errInfo(msgData[MSGE_UNZIP_MEM_OVERFLOW]));
											case Z_BUF_ERROR:
												throw(errInfo(msgData[MSGE_UNZIP_BUF_OVERFLOW]));
											case Z_DATA_ERROR:
												throw(errInfo(msgData[MSGE_UNZIP_BROKEN]));
											default:
												throw(errInfo(msgData[MSGE_UNZIP_INFLATEEND] + std::to_string(err)));
										}
									}
								}
							}
							else
							{
								switch (err)
								{
									case Z_MEM_ERROR:
										throw(errInfo(msgData[MSGE_UNZIP_MEM_OVERFLOW]));
									case Z_BUF_ERROR:
										throw(errInfo(msgData[MSGE_UNZIP_BUF_OVERFLOW]));
									case Z_DATA_ERROR:
										throw(errInfo(msgData[MSGE_UNZIP_BROKEN]));
									default:
										throw(errInfo(msgData[MSGE_UNZIP_INFLATE] + std::to_string(err)));
								}
							}
						}
					}
					catch (errInfo ex)
					{
						delete[] dataBuf;
						delete[] fileBuf;
						return ex;
					}
					catch (...)
					{
						delete[] dataBuf;
						delete[] fileBuf;
						throw;
					}

					delete[] dataBuf;
					delete[] fileBuf;
					break;
			}

			fout.close();
		}
	}
	fin.close();
	return errInfo();
}
