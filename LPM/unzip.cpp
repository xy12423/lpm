#include "stdafx.h"
#include "unzip.h"

#define skip(n)	for (i = 0; i < (n); i++)                 \
				{                                       \
					dataBegin++;                        \
					if (dataBegin == dataEnd)           \
						return errInfo("E:unzip:Broken File");  \
				}
#define readUINT(var)	for (i = 0; i < 4; i++)                                          \
						{                                                                \
							if (dataBegin == dataEnd)                                    \
								return errInfo("E:unzip:Broken File");                           \
							var = (var >> 8) | (static_cast<UINT>(*dataBegin) << 24);    \
							dataBegin++;                                                 \
						}
#define readBYTE(var)	for (i = 0; i < 2; i++)                                          \
						{                                                                \
							if (dataBegin == dataEnd)                                    \
								return errInfo("E:unzip:Broken File");                           \
							var = (var >> 8) | (static_cast<UINT>(*dataBegin) << 8);     \
							dataBegin++;                                                 \
						}

fProgressReportCallback prCallbackP = NULL;

errInfo unzip(dataBuf::const_iterator dataBegin, dataBuf::const_iterator dataEnd, boost::filesystem::path path)
{
	UINT i;
	std::ofstream fout;
	ULONGLONG sizeAll = 0, sizeInflated = 0;
	double progress = 0;
	if (prCallbackP != NULL)
	{
		dataBuf::const_iterator dataPtr = dataBegin;
		(*prCallbackP)(0);
		while (dataBegin != dataEnd)
		{
			UINT head = 0;
			readUINT(head);
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
		dataBegin = dataPtr;
	}
	while (dataBegin != dataEnd)
	{
		UINT head = 0;
		readUINT(head);
		if (head != 0x04034b50)
			break;
		skip(4);
		USHORT compMethod = 0;
		readBYTE(compMethod);
		skip(8);
		UINT fileSize, fileOriginSize;
		readUINT(fileSize);
		readUINT(fileOriginSize);
		USHORT nameLen, extLen;
		readBYTE(nameLen);
		readBYTE(extLen);
		std::string name;
		for (i = 0; i < nameLen; i++)
		{
			if (dataBegin == dataEnd)
				return errInfo("E:unzip:Broken File");
			name.push_back(*dataBegin);
			dataBegin++;
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
					if (prCallbackP != NULL)	//Need report
					{
						for (i = 0; i < fileSize; i++)
						{
							if (dataBegin == dataEnd)
								return errInfo("E:unzip:Broken File");
							fout.put(*dataBegin);
							dataBegin++;
							sizeInflated += 1;
							if ((i & 0xFF) == 0)
							{
								progress = static_cast<double>(sizeInflated) * 100 / sizeAll;
								(*prCallbackP)(progress);
							}
						}
						progress = static_cast<double>(sizeInflated) * 100 / sizeAll;
						(*prCallbackP)(progress);
					}
					else
					{
						for (i = 0; i < fileSize; i++)
						{
							if (dataBegin == dataEnd)
								return errInfo("E:unzip:Broken File");
							fout.put(*dataBegin);
							dataBegin++;
						}
					}
					break;
				case 8:
					const int blockSize = 0x10000000;

					BYTE *dataBuf = new BYTE[fileSize];
					BYTE *ptr = dataBuf;
					for (i = 0; i < fileSize; i++)
					{
						if (dataBegin == dataEnd)
						{
							delete[] dataBuf;
							return errInfo("E:unzip:Broken File");
						}
						*ptr = *dataBegin;
						ptr++;
						dataBegin++;
					}

					z_stream zstream;
					zstream.zalloc = static_cast<alloc_func>(Z_NULL);
					zstream.zfree = static_cast<free_func>(Z_NULL);
					zstream.opaque = 0;
					zstream.next_in = NULL;
					zstream.avail_in = 0;
					int err = inflateInit2(&zstream, -8);
					if (err != Z_OK)
					{
						delete[] dataBuf;
						return errInfo("E:unzip:inflateInit2 failed with code " + num2str(err));
					}
					zstream.next_in = dataBuf;
					zstream.avail_in = fileSize;
					BYTE *fileBuf = new BYTE[blockSize];
					zstream.next_out = fileBuf;
					zstream.avail_out = blockSize;

					while (true)
					{
						zstream.next_out = fileBuf;
						zstream.avail_out = blockSize;
						err = inflate(&zstream, Z_SYNC_FLUSH);
						if (err == Z_OK || err == Z_STREAM_END)
						{
							ptr = fileBuf;
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
								(*prCallbackP)(progress);
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
											return errInfo("E:unzip:Memory overflow");
										case Z_BUF_ERROR:
											return errInfo("E:unzip:File Format Error:Buffer not enough");
										case Z_DATA_ERROR:
											return errInfo("E:unzip:Broken File");
										default:
											return errInfo(str2cstr("E:unzip:inflateEnd failed with code " + num2str(err)));
									}
								}
							}
						}
						else
						{
							switch (err)
							{
								case Z_MEM_ERROR:
									return errInfo("E:unzip:Memory overflow");
								case Z_BUF_ERROR:
									return errInfo("E:unzip:File Format Error:Buffer not enough");
								case Z_DATA_ERROR:
									return errInfo("E:unzip:Broken File");
								default:
									return errInfo(str2cstr("E:unzip:inflate failed with code " + num2str(err)));
							}
						}
					}

					delete[] dataBuf;
					delete[] fileBuf;
					break;
			}

			fout.close();
		}
	}
	return errInfo();
}
