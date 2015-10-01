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

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define skip(n)	fin.seekg(n, std::ios_base::cur);
#define readUINT(var)	fin.read(reinterpret_cast<char*>(&(var)), 4);                    \
						if (fin.eof())                                                   \
							return errInfo(msgData[MSGE_UNZIP_BROKEN]);
#define readUSHORT(var)	fin.read(reinterpret_cast<char*>(&(var)), 2);                    \
						if (fin.eof())                                                   \
							return errInfo(msgData[MSGE_UNZIP_BROKEN]);

inline errInfo get_zlib_err(int err)
{
	switch (err)
	{
		case Z_MEM_ERROR:
			return errInfo(msgData[MSGE_UNZIP_MEM_OVERFLOW]);
		case Z_BUF_ERROR:
			return errInfo(msgData[MSGE_UNZIP_BUF_OVERFLOW]);
		case Z_DATA_ERROR:
			return errInfo(msgData[MSGE_UNZIP_BROKEN]);
		default:
			return errInfo(msgData[MSGE_UNZIP_INFLATEEND] + std::to_string(err));
	}
}

const int blockSize = 0x100000;

errInfo unzip(std::string fPath, boost::filesystem::path path)
{
	std::ifstream fin(fPath, std::ios_base::in | std::ios_base::binary);
	std::ofstream fout;
	uint64_t sizeAll = 0, sizeInflated = 0;
	double progress = 0;
	if (prCallbackP != NULL)
	{
		(*prCallbackP)(0, 0);
		while (!fin.eof())
		{
			uint32_t head = 0;
			fin.read(reinterpret_cast<char*>(&head), 4);
			if (fin.eof())
				break;
			if (head != 0x04034b50)
				break;
			skip(10);
			uint32_t fileSize, fileOriginSize;
			readUINT(fileSize);
			readUINT(fileOriginSize);
			sizeAll += fileOriginSize;
			uint16_t nameLen, extLen;
			readUSHORT(nameLen);
			readUSHORT(extLen);
			uint64_t skipLen = static_cast<uint64_t>(fileSize) + nameLen + extLen;
			skip(skipLen);
		}
		fin.close();
		fin.open(fPath, std::ios_base::in | std::ios_base::binary);
	}
	while (!fin.eof())
	{
		uint32_t head = 0;
		fin.read(reinterpret_cast<char*>(&head), 4);
		if (fin.eof())
			break;

		switch (head)
		{
			case 0x08074b50:
				skip(12);
				break;
			case 0x02014b50:
			{
				skip(24);
				uint16_t name_size, ext_size, comment_size;
				readUSHORT(name_size);
				readUSHORT(ext_size);
				readUSHORT(comment_size);
				skip(4);
				uint32_t file_attr;
				readUINT(file_attr);
				skip(4);
				std::string name;
				std::unique_ptr<char[]> name_buf = std::make_unique<char[]>(name_size);
				fin.read(name_buf.get(), name_size);
				if (fin.eof())
					throw(0);
				name.assign(name_buf.get(), name_size);
				skip(ext_size + comment_size);

				if (!(file_attr & DIR_ATTRIBUTE) && fs::is_directory(path / name))
				{
					fs::remove(path / name);
					fout.open((path / name).string());
					fout.close();
				}

				break;
			}
			case 0x04034b50:
			{
				skip(2);
				uint16_t flag = 0, comp_method = 0;
				readUSHORT(flag);
				readUSHORT(comp_method);
				skip(4);
				uint32_t crc32_read, compressed_size, origin_size;
				readUINT(crc32_read);
				readUINT(compressed_size);
				readUINT(origin_size);
				USHORT name_size, ext_size;
				readUSHORT(name_size);
				readUSHORT(ext_size);
				std::string name;
				std::unique_ptr<char[]> name_buf = std::make_unique<char[]>(name_size);
				fin.read(name_buf.get(), name_size);
				if (fin.eof())
					throw(0);
				name.assign(name_buf.get(), name_size);
				skip(ext_size);

				if (origin_size == 0)
					boost::filesystem::create_directories(path / name);
				else
				{
					boost::filesystem::path filePath = path / name;
					fout.open(filePath.string(), std::ios::out | std::ios::binary);

					switch (comp_method)
					{
						case 0:
						{
							std::unique_ptr<char[]> buf = std::make_unique<char[]>(compressed_size);
							fin.read(buf.get(), compressed_size);
							if (fin.eof())
								throw(0);
							fout.write(buf.get(), compressed_size);
							sizeInflated += compressed_size;
							if (prCallbackP != NULL)	//Need report
							{
								progress = static_cast<double>(sizeInflated) * 100 / sizeAll;
								(*prCallbackP)(progress, sizeInflated);
							}
							break;
						}
						case 8:
							std::unique_ptr<BYTE[]> in_buf = std::make_unique<BYTE[]>(blockSize);
							std::unique_ptr<BYTE[]> out_buf = std::make_unique<BYTE[]>(blockSize);
							uint32_t compressed_last = compressed_size;
							uint32_t crc32_real = 0;

							z_stream zstream;
							zstream.zalloc = static_cast<alloc_func>(Z_NULL);
							zstream.zfree = static_cast<free_func>(Z_NULL);
							zstream.opaque = 0;
							zstream.next_in = NULL;
							zstream.avail_in = 0;
							int err = inflateInit2(&zstream, -15);
							if (err != Z_OK)
								return errInfo(msgData[MSGE_UNZIP_INFLATEINIT] + std::to_string(err));

							fin.read(reinterpret_cast<char*>(in_buf.get()), min(compressed_last, blockSize));
							zstream.next_in = in_buf.get();
							zstream.avail_in = fin.gcount();
							compressed_last -= fin.gcount();

							while (true)
							{
								zstream.next_out = out_buf.get();
								zstream.avail_out = blockSize;
								err = inflate(&zstream, Z_NO_FLUSH);
								if (err == Z_OK || err == Z_STREAM_END)
								{
									uint32_t inflatedSize = blockSize - zstream.avail_out;
									fout.write(reinterpret_cast<char*>(out_buf.get()), inflatedSize);
									crc32_real = crc32(crc32_real, out_buf.get(), inflatedSize);

									if (prCallbackP != NULL)
									{
										sizeInflated += inflatedSize;
										progress = static_cast<double>(sizeInflated) * 100 / sizeAll;
										(*prCallbackP)(progress, sizeInflated);
									}

									if (err == Z_STREAM_END)
									{
										if ((err = inflateEnd(&zstream)) == Z_OK)
											break;
										else
											return get_zlib_err(err);
									}
									else if (zstream.avail_in == 0 && compressed_last != 0)
									{
										fin.read(reinterpret_cast<char*>(in_buf.get()), min(compressed_last, blockSize));
										zstream.next_in = in_buf.get();
										zstream.avail_in = fin.gcount();
										compressed_last -= fin.gcount();
									}
								}
								else
									return get_zlib_err(err);
							}
							
							if (crc32_read != crc32_real)
								return errInfo(msgData[MSGE_UNZIP_BROKEN]);

							break;
					}

					fout.close();
				}

				if (flag & 0x4)
					skip(12);

				break;
			}
			default:
				fin.setstate(fout.eofbit);
				break;
		}
	}
	fin.close();
	return errInfo();
}
