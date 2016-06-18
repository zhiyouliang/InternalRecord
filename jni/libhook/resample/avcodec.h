 

#ifndef AVCODEC_AVCODEC_H
#define AVCODEC_AVCODEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h> 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef INT16_MIN
#define INT16_MIN       (-0x7fff-1)
#endif

#ifndef INT16_MAX
#define INT16_MAX       0x7fff
#endif


#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)

#define FFABS(a) ((a) >= 0 ? (a) : (-(a)))

#ifndef CONFIG_RESAMPLE_HP
#define FILTER_SHIFT 15

#define FELEM short
#define FELEM2 int
#define FELEML int64_t
#define FELEM_MAX INT16_MAX
#define FELEM_MIN INT16_MIN
#define WINDOW_TYPE 9
#elif !defined(CONFIG_RESAMPLE_AUDIOPHILE_KIDDY_MODE)
#define FILTER_SHIFT 30

#define FELEM int32_t
#define FELEM2 __int64
#define FELEML __int64
#define FELEM_MAX INT32_MAX
#define FELEM_MIN INT32_MIN
#define WINDOW_TYPE 12
#else
#define FILTER_SHIFT 0

#define FELEM double
#define FELEM2 double
#define FELEML double
#define WINDOW_TYPE 24
#endif 

enum SampleFormat {
	SAMPLE_FMT_NONE = -1,
	SAMPLE_FMT_U8,              ///< unsigned 8 bits
	SAMPLE_FMT_S16,             ///< signed 16 bits
	SAMPLE_FMT_S32,             ///< signed 32 bits
	SAMPLE_FMT_FLT,             ///< float
	SAMPLE_FMT_DBL,             ///< double
	SAMPLE_FMT_NB               ///< Number of sample formats. DO NOT USE if dynamically linking to libavcodec
}; 


typedef struct AVResampleContext{
	FELEM *filter_bank;
	int filter_length;
	int ideal_dst_incr;
	int dst_incr;
	int index;
	int frac;
	int src_incr;
	int compensation_distance;
	int phase_shift;
	int phase_mask;
	int linear;
}AVResampleContext;

typedef struct __tag_AVAudioConvert 
{
	int in_channels, out_channels;
	int fmt_pair;
}AVAudioConvert;

typedef  struct __tag_ReSampleContext
{     
	struct AVResampleContext *resample_context;
	short *temp[2];
	int temp_len;
	float ratio;
	/* channel convert */
	int input_channels, output_channels, filter_channels;
	AVAudioConvert *convert_ctx[2];
	enum SampleFormat sample_fmt[2]; ///< input and output sample format
	unsigned sample_size[2];         ///< size of one sample in sample_fmt
	short *buffer[2];                ///< buffers used for conversion to S16
	unsigned buffer_size[2];         ///< sizes of allocated buffers
}ReSampleContext;

long  _rint(double x);
long long llrint(double x);
long int lrint(double x);
long int lrintf(float x);
int av_clip(int a, int amin, int amax);



ReSampleContext *audio_resample_init(int output_channels, int input_channels,
                                     int output_rate, int input_rate);

int audio_resample(ReSampleContext *s, short *output, short *input, int nb_samples);

void audio_resample_close(ReSampleContext *s);

#ifdef __cplusplus
}
#endif


#endif /* AVCODEC_AVCODEC_H */
