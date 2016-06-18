/*
 * AssetManager.h
 *
 *  Created on: 2015年7月28日
 *      Author: Administrator
 */

#ifndef LIBHOOK_ASSETMANAGER_H_
#define LIBHOOK_ASSETMANAGER_H_

#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <stdio.h>

class AssetFile
{
	public:
		AssetFile(AAssetManager *mgr, const char* fileName);
		virtual ~AssetFile();

		int Read(void* buf, int size);
		off_t Seek(off_t offset, int origin);
		off_t Position();
		off_t Size();
	public:
		AAsset* asset;
};

class AssetManager
{
	public:
		AssetManager(AAssetManager *mgr);
		AssetManager(JNIEnv* env, jobject activity);

		AssetFile* Open(const char* fileName);
	protected:
		AAssetManager* mgr;
};



#endif /* LIBHOOK_ASSETMANAGER_H_ */
