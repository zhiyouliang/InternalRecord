#ifndef MEDIACODEC_JNI_H_
#define MEDIACODEC_JNI_H_


typedef struct __tag_media_codec_context media_codec_context;


media_codec_context* init_media_codec(JNIEnv *env);
bool media_codec_start(JNIEnv *env,media_codec_context* context,const char *psz_mime,jobject media_format);
bool media_codec_decode(JNIEnv *env,media_codec_context* context,const unsigned char* p_buf, int i_size,int64_t timestamp);
int media_codec_get_frame(JNIEnv *env,media_codec_context* context,unsigned char** p_out,int* out_len,int64_t* out_timestamp, int* output_flags,int64_t timeout);
bool media_codec_release_output(JNIEnv *env,media_codec_context* context, int i_index);
bool media_codec_stop(JNIEnv *env,media_codec_context* context);
void uninit_media_codec(JNIEnv *env,media_codec_context* context);

#endif //MEDIACODEC_JNI_H_
