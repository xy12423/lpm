#include "stdafx.h"
#include "unzip.h"

#define skip(n)	for (i = 0; i < n; i++)                 \
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

errInfo unzip(dataBuf::const_iterator dataBegin, dataBuf::const_iterator dataEnd, boost::filesystem::path path)
{
	UINT i;
	std::ofstream fout;
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
					for (i = 0; i < fileSize; i++)
					{
						if (dataBegin == dataEnd)
							return errInfo("E:unzip:Broken File");
						fout.put(*dataBegin);
						dataBegin++;
					}
					break;
				case 8:
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

					const int blockSize = 0x10000000;
					
					BYTE *fileBuf = new BYTE[blockSize];

					z_stream zstream;
					zstream.zalloc = static_cast<alloc_func>(Z_NULL);
					zstream.zfree = static_cast<free_func>(Z_NULL);
					zstream.opaque = 0;
					zstream.next_in = NULL;
					zstream.avail_in = 0;
					int err = inflateInit2(&zstream, -8);
					if (err != Z_OK)
						return errInfo("E:unzip:inflateInit2 failed with code " + num2str(err));
					zstream.next_in = dataBuf;
					zstream.avail_in = fileSize;
					zstream.next_out = fileBuf;
					zstream.avail_out = fileOriginSize;

					while (true)
					{
						zstream.next_out = fileBuf;
						zstream.avail_out = blockSize;
						err = inflate(&zstream, Z_SYNC_FLUSH);
						if (err == Z_OK || err == Z_STREAM_END)
						{
							ptr = fileBuf;
							for (i = 0; i < blockSize - zstream.avail_out; i++)
							{
								fout.put(*ptr);
								ptr++;
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
