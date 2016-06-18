#include "avcodec.h" 
#include <assert.h>

/**
 * 0th order modified bessel function of the first kind.
 */
static double bessel(double x){
    double v=1;
    double t=1;
    int i;

    x= x*x/4;
    for(i=1; i<50; i++){
        t *= x/(i*i);
        v += t;
    }
    return v;
}

/**
 * builds a polyphase filterbank.
 * @param factor resampling factor
 * @param scale wanted sum of coefficients for each filter
 * @param type 0->cubic, 1->blackman nuttall windowed sinc, 2..16->kaiser windowed sinc beta=2..16
 */
void av_build_filter(FELEM *filter, double factor, int tap_count, int phase_count, int scale, int type)
{
	
    int ph, i;
    double x, y, w;
	double* tab = (double*)malloc(tap_count*sizeof(double));
    const int center= (tap_count-1)/2; 
	 

    /* if upsampling, only need to interpolate, no filter */
    if (factor > 1.0)
        factor = 1.0;

    for(ph=0;ph<phase_count;ph++) {
        double norm = 0;
        for(i=0;i<tap_count;i++) {
            x = M_PI * ((double)(i - center) - (double)ph / phase_count) * factor;
            if (x == 0) y = 1.0;
            else        y = sin(x) / x;
            switch(type){
            case 0:{
                const float d= -0.5; //first order derivative = -0.5
                x = fabs(((double)(i - center) - (double)ph / phase_count) * factor);
                if(x<1.0) y= 1 - 3*x*x + 2*x*x*x + d*(            -x*x + x*x*x);
                else      y=                       d*(-4 + 8*x - 5*x*x + x*x*x);
                break;}
            case 1:
                w = 2.0*x / (factor*tap_count) + M_PI;
                y *= 0.3635819 - 0.4891775 * cos(w) + 0.1365995 * cos(2*w) - 0.0106411 * cos(3*w);
                break;
            default:
                w = 2.0*x / (factor*tap_count*M_PI);
                y *= bessel(type*sqrt(FFMAX(1-w*w, 0)));
                break;
            }

            tab[i] = y;
            norm += y;
        }

        /* normalize so that an uniform color remains the same */
        for(i=0;i<tap_count;i++) {
#ifdef CONFIG_RESAMPLE_AUDIOPHILE_KIDDY_MODE
            filter[ph * tap_count + i] = tab[i] / norm;
#else
            filter[ph * tap_count + i] = av_clip(lrintf(tab[i] * scale / norm), FELEM_MIN, FELEM_MAX);
#endif
        }
    }

	free(tab);
 
}

AVResampleContext *av_resample_init(int out_rate, int in_rate, int filter_size, int phase_shift, int linear, double cutoff){
    AVResampleContext *c= av_mallocz(sizeof(AVResampleContext));
    double factor= FFMIN(out_rate * cutoff / in_rate, 1.0);
    int phase_count= 1<<phase_shift;

    c->phase_shift= phase_shift;
    c->phase_mask= phase_count-1;
    c->linear= linear;

    c->filter_length= FFMAX((int)ceil(filter_size/factor), 1);
    c->filter_bank= av_mallocz(c->filter_length*(phase_count+1)*sizeof(FELEM));
    av_build_filter(c->filter_bank, factor, c->filter_length, phase_count, 1<<FILTER_SHIFT, WINDOW_TYPE);
    memcpy(&c->filter_bank[c->filter_length*phase_count+1], c->filter_bank, (c->filter_length-1)*sizeof(FELEM));
    c->filter_bank[c->filter_length*phase_count]= c->filter_bank[c->filter_length - 1];

    c->src_incr= out_rate;
    c->ideal_dst_incr= c->dst_incr= in_rate * phase_count;
    c->index= -phase_count*((c->filter_length-1)/2);

    return c;
}

void av_resample_close(AVResampleContext *c){
    av_freep(&c->filter_bank);
    av_freep(&c);
}

void av_resample_compensate(AVResampleContext *c, int sample_delta, int compensation_distance){
//    sample_delta += (c->ideal_dst_incr - c->dst_incr)*(int64_t)c->compensation_distance / c->ideal_dst_incr;
    c->compensation_distance= compensation_distance;
    c->dst_incr = c->ideal_dst_incr - c->ideal_dst_incr * (int64_t)sample_delta / compensation_distance;
}

int av_resample(AVResampleContext *c, short *dst, short *src, int *consumed, int src_size, int dst_size, int update_ctx){
    int dst_index, i;
    int index= c->index;
    int frac= c->frac;
    int dst_incr_frac= c->dst_incr % c->src_incr;
    int dst_incr=      c->dst_incr / c->src_incr;
    int compensation_distance= c->compensation_distance;

  if(compensation_distance == 0 && c->filter_length == 1 && c->phase_shift==0){
        int64_t index2= ((int64_t)index)<<32;
        int64_t incr= (1LL<<32) * c->dst_incr / c->src_incr;
        dst_size= FFMIN(dst_size, (src_size-1-index) * (int64_t)c->src_incr / c->dst_incr);

        for(dst_index=0; dst_index < dst_size; dst_index++){
            dst[dst_index] = src[index2>>32];
            index2 += incr;
        }
        frac += dst_index * dst_incr_frac;
        index += dst_index * dst_incr;
        index += frac / c->src_incr;
        frac %= c->src_incr;
  }else{
    for(dst_index=0; dst_index < dst_size; dst_index++){
        FELEM *filter= c->filter_bank + c->filter_length*(index & c->phase_mask);
        int sample_index= index >> c->phase_shift;
        FELEM2 val=0;

        if(sample_index < 0){
            for(i=0; i<c->filter_length; i++)
                val += src[FFABS(sample_index + i) % src_size] * filter[i];
        }else if(sample_index + c->filter_length > src_size){
            break;
        }else if(c->linear){
            FELEM2 v2=0;
            for(i=0; i<c->filter_length; i++){
                val += src[sample_index + i] * (FELEM2)filter[i];
                v2  += src[sample_index + i] * (FELEM2)filter[i + c->filter_length];
            }
            val+=(v2-val)*(FELEML)frac / c->src_incr;
        }else{
            for(i=0; i<c->filter_length; i++){
                val += src[sample_index + i] * (FELEM2)filter[i];
            }
        }

#ifdef CONFIG_RESAMPLE_AUDIOPHILE_KIDDY_MODE
        dst[dst_index] = av_clip_int16(lrintf(val));
#else
        val = (val + (1<<(FILTER_SHIFT-1)))>>FILTER_SHIFT;
        dst[dst_index] = (unsigned)(val + 32768) > 65535 ? (val>>31) ^ 32767 : val;
#endif

        frac += dst_incr_frac;
        index += dst_incr;
        if(frac >= c->src_incr){
            frac -= c->src_incr;
            index++;
        }

        if(dst_index + 1 == compensation_distance){
            compensation_distance= 0;
            dst_incr_frac= c->ideal_dst_incr % c->src_incr;
            dst_incr=      c->ideal_dst_incr / c->src_incr;
        }
    }
  }
    *consumed= FFMAX(index, 0) >> c->phase_shift;
    if(index>=0) index &= c->phase_mask;

    if(compensation_distance){
        compensation_distance -= dst_index;
        assert(compensation_distance > 0);
    }
    if(update_ctx){
        c->frac= frac;
        c->index= index;
        c->dst_incr= dst_incr_frac + c->src_incr*dst_incr;
        c->compensation_distance= compensation_distance;
    }
#if 0
    if(update_ctx && !c->compensation_distance){
#undef rand
        av_resample_compensate(c, rand() % (8000*2) - 8000, 8000*2);
av_log(NULL, AV_LOG_DEBUG, "%d %d %d\n", c->dst_incr, c->ideal_dst_incr, c->compensation_distance);
    }
#endif

    return dst_index;
}

long  _rint(double x)
{
	if(x >= 0.) 
		return (long)(x + 0.5);
	else
		return (long)(x - 0.5);
}

#if !HAVE_LLRINT
long long llrint(double x)
{
	return _rint(x);
}
#endif /* HAVE_LLRINT */

#if !HAVE_LRINT
long int lrint(double x)
{
	return _rint(x);
}
#endif /* HAVE_LRINT */

#if !HAVE_LRINTF
long int lrintf(float x)
{
	return (int)(_rint(x));
}

int av_clip(int a, int amin, int amax)
{
	if      (a < amin) return amin;
	else if (a > amax) return amax;
	else               return a;
}

#endif /* HAVE_LRINTF */
