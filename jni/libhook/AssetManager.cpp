/*
 * AssetManager.cpp
 *
 *  Created on: 2015年7月28日
 *      Author: Administrator
 */


#include <unistd.h>
#include "AssetManager.h"

AssetFile::AssetFile(AAssetManager* mgr, const char* fileName)
{
	this->asset = AAssetManager_open(mgr, fileName, AASSET_MODE_UNKNOWN);
}

AssetFile::~AssetFile()
{
	AAsset_close(this->asset);
}

int AssetFile::Read(void* buf, int size)
{
	return AAsset_read(this->asset, buf, size);
}

off_t AssetFile::Seek(off_t offset, int origin)
{
	return AAsset_seek(this->asset, offset, origin);
}

off_t AssetFile::Position()
{
	return AAsset_seek(this->asset, 0, SEEK_CUR);
}

off_t AssetFile::Size()
{
	return AAsset_getLength(this->asset);
}

AssetManager::AssetManager(AAssetManager* mgr)
{
	this->mgr = mgr;
}

AssetManager::AssetManager(JNIEnv* env, jobject activity)
{
	jclass c = env->GetObjectClass(activity);
	//jmethodID methodID = env->GetMethodID(c, "getAssets", "()Landroid/content/res/AssetManager;");
	//if(!methodID)
	//	return;
	this->mgr = AAssetManager_fromJava(env, activity);
}

AssetFile* AssetManager::Open(const char* fileName)
{
	return new AssetFile(mgr, fileName);
}


