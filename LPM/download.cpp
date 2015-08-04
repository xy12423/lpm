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
#include "download.h"

double sizeAll = 0;
size_t sizeDownloaded = 0;
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	dataBuf *myBuf = static_cast<dataBuf*>(userp);
	char *dataBuf = static_cast<char*>(buffer);
	size_t sizePart = size * nmemb;
	for (size_t i = 0; i < sizePart; i++)
	{
		myBuf->push_back(*dataBuf);
		dataBuf++;
	}
	sizeDownloaded += sizePart;
	if (prCallbackP != NULL)
		(*prCallbackP)(sizeDownloaded * 100 / sizeAll, sizeDownloaded);
	return sizePart;
}

size_t write_data_to_file(void *buffer, size_t size, size_t nmemb, void *userp)
{
	std::ofstream *fs = static_cast<std::ofstream*>(userp);
	char *dataBuf = static_cast<char*>(buffer);
	size_t sizePart = size * nmemb;
	fs->write(dataBuf, sizePart);
	sizeDownloaded += sizePart;
	if (prCallbackP != NULL)
		(*prCallbackP)(sizeDownloaded * 100 / sizeAll, sizeDownloaded);
	return sizePart;
}

size_t save_header(void *ptr, size_t size, size_t nmemb, void *data)
{
	return size * nmemb;
}

errInfo download(const std::string &add, dataBuf *buf)
{
	std::vector<char> errBuf;
	errBuf.reserve(2048);
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, add.c_str());
	curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errBuf.data());
	CURLcode success;
	infoStream << msgData[MSGI_CONNECTING] << std::endl;
	if (prCallbackP != NULL)
	{
		curl_easy_setopt(handle, CURLOPT_HEADER, 1);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, save_header);
		success = curl_easy_perform(handle);
		if (success != CURLcode::CURLE_OK)
		{
			curl_easy_cleanup(handle);
			return errInfo(msgData[MSGE_NETWORK] + errBuf.data());
		}
		success = curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &sizeAll);
		if (success != CURLcode::CURLE_OK)
		{
			curl_easy_cleanup(handle);
			return errInfo(msgData[MSGE_NETWORK] + errBuf.data());
		}
		curl_easy_setopt(handle, CURLOPT_HEADER, 0);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 0);
		(*prCallbackP)(0, 0);
	}

	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, buf);
	infoStream << msgData[MSGI_DOWNLOADING] << std::endl;
	sizeDownloaded = 0;
	success = curl_easy_perform(handle);
	if (success != CURLcode::CURLE_OK)
	{
		curl_easy_cleanup(handle);
		return errInfo(msgData[MSGE_NETWORK] + errBuf.data());
	}
	if (prCallbackP != NULL)
		(*prCallbackP)(100, sizeAll);
	curl_easy_cleanup(handle);
	
	infoStream << msgData[MSGI_DOWNLOADED] << std::endl;
	return errInfo();
}

errInfo download(const std::string &add, std::string path)
{
	std::vector<char> errBuf;
	errBuf.reserve(2048);
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, add.c_str());
	curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errBuf.data());
	CURLcode success;
	infoStream << msgData[MSGI_CONNECTING] << std::endl;
	if (prCallbackP != NULL)
	{
		curl_easy_setopt(handle, CURLOPT_HEADER, 1);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, save_header);
		success = curl_easy_perform(handle);
		if (success != CURLcode::CURLE_OK)
		{
			curl_easy_cleanup(handle);
			return errInfo(msgData[MSGI_DOWNLOADING] + errBuf.data());
		}
		success = curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &sizeAll);
		if (success != CURLcode::CURLE_OK)
		{
			curl_easy_cleanup(handle);
			return errInfo(msgData[MSGI_DOWNLOADING] + errBuf.data());
		}
		curl_easy_setopt(handle, CURLOPT_HEADER, 0);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 0);
		(*prCallbackP)(0, 0);
	}

	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_to_file);
	std::ofstream fout(path, std::ios_base::out | std::ios_base::binary);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &fout);
	infoStream << msgData[MSGI_DOWNLOADING] << std::endl;
	sizeDownloaded = 0;
	success = curl_easy_perform(handle);
	if (success != CURLcode::CURLE_OK)
	{
		curl_easy_cleanup(handle);
		return errInfo(msgData[MSGI_DOWNLOADING] + errBuf.data());
	}
	if (prCallbackP != NULL)
		(*prCallbackP)(100, sizeAll);
	curl_easy_cleanup(handle);

	infoStream << msgData[MSGI_DOWNLOADED] << std::endl;
	return errInfo();
}
