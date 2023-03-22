#include "testing.h"

#include "xmath/xmath.h"
#include <math.h>
#include <stdio.h>
#include <assert.h>

void make_sine_table(double * sine_lut, unsigned proc_frame_length){
    for(unsigned i=0;i<(proc_frame_length/4) + 1;i++)
        sine_lut[i] = (double)sin(2.0*M_PI*(double)i / (double)proc_frame_length);
}

static size_t reverse_bits(size_t x, int n) {
	size_t result = 0;
	for (int i = 0; i < n; i++, x >>= 1)
		result = (result << 1) | (x & 1U);
	return result;
}

void bit_reverse( complex_double_t pts[], const uint32_t N )
{
    uint32_t shift = CLS_S32(N);
    for(uint32_t i = 1; i < N-1; i++) {
        uint32_t rev = reverse_bits(i,32) >> (shift + 1);
        if (rev > i) {
            double im1, re1, im2, re2;
            im1 = pts[i].im;
            re1 = pts[i].re;
            im2 = pts[rev].im;
            re2 = pts[rev].re;
            pts[i].re = re2;
            pts[i].im = im2;
            pts[rev].re = re1;
            pts[rev].im = im1;
        }
    }
}

void forward_fft (
    complex_double_t pts[],
    const uint32_t  N,
    const double   sine[] )
{
    uint32_t shift = 30-CLS_S32(N);
    for(uint32_t step = 2 ; step <= N; step = step * 2, shift--) {
        uint32_t step2 = step >> 1;
        uint32_t step4 = step2 >> 1;
        uint32_t k;
        for(k = 0; k < step4 + (step2&1); k++) {
            double rRe = sine[(N>>2)-(k<<shift)];
            double rIm = sine[k<<shift];
            for(int32_t block = k+N-step; block >= 0; block-=step) {
                double tRe = pts[block].re;
                double tIm = pts[block].im;
                double tRe2 = pts[block + step2].re;
                double tIm2 = pts[block + step2].im;

                double sRe2 = (tRe2*rRe) + (tIm2*rIm);
                double sIm2 =  (tIm2*rRe) - (tRe2*rIm);

                pts[block].re = tRe + sRe2;
                pts[block].im = tIm + sIm2;
                pts[block+step2].re = tRe - sRe2;
                pts[block+step2].im = tIm - sIm2;
            }
        }
        for(k=(step2 & 1); k < step4; k++) {
            double rRe = -sine[k<<shift];
            double rIm = sine[(N>>2)-(k<<shift)];
            for(int32_t block = k+step4+N-step; block >= 0; block-=step) {
                double tRe = pts[block].re;
                double tIm = pts[block].im;
                double tRe2 = pts[block + step2].re;
                double tIm2 = pts[block + step2].im;

                double sRe2 = (tRe2 * rRe) + (tIm2 * rIm);
                double sIm2 = (tIm2 * rRe) - (tRe2 * rIm);

                pts[block].re = tRe + sRe2;
                pts[block].im = tIm + sIm2;
                pts[block+step2].re = tRe - sRe2;
                pts[block+step2].im = tIm - sIm2;
            }
        }
    }
}

void inverse_fft (
    complex_double_t pts[],
    const uint32_t  N,
    const double   sine[] )
{
    uint32_t shift = 30-CLS_S32(N);
    for(uint32_t step = 2 ; step <= N; step = step * 2, shift--) {
        uint32_t step2 = step >> 1;
        uint32_t step4 = step2 >> 1;
        uint32_t k;
        for(k = 0; k < step4 + (step2&1); k++) {
            double rRe = sine[(N>>2)-(k<<shift)];
            double rIm = sine[k<<shift];
            for(unsigned block = k; block < k+N; block+=step) {
                double tRe = pts[block].re;
                double tIm = pts[block].im;
                double tRe2 = pts[block + step2].re;
                double tIm2 = pts[block + step2].im;

                double sRe2 = (tRe2*rRe)-(tIm2*rIm);
                double sIm2 = (tRe2*rIm)+(tIm2*rRe);

                pts[block].re = tRe + sRe2;
                pts[block].im = tIm + sIm2;
                pts[block+step2].re = tRe - sRe2;
                pts[block+step2].im = tIm - sIm2;
            }
        }
        for(k=(step2 & 1); k < step2-step4; k++) {
            double rRe = -sine[k<<shift];
            double rIm = sine[(N>>2)-(k<<shift)];
            for(unsigned block = k+step4; block < k+step4+N; block+=step) {
                double tRe = pts[block].re;
                double tIm = pts[block].im;
                double tRe2 = pts[block + step2].re;
                double tIm2 = pts[block + step2].im;

                double sRe2 = (tRe2*rRe)-(tIm2*rIm);
                double sIm2 = (tRe2*rIm)+(tIm2*rRe);

                pts[block].re = tRe + sRe2;
                pts[block].im = tIm + sIm2;
                pts[block+step2].re = tRe - sRe2;
                pts[block+step2].im = tIm - sIm2;
            }
        }
    }

    for(unsigned i=0;i<N;i++){
        pts[i].re /= N;
        pts[i].im /= N;
    }

}

void split_spectrum( complex_double_t pts[], const uint32_t N ){

    for(uint32_t i=1;i<N/2;i++){
        double a_re = (pts[i].re + pts[N-i].re)/2;
        double a_im = (pts[i].im - pts[N-i].im)/2;
        double b_re = (pts[i].im + pts[N-i].im)/2;
        double b_im = (-pts[i].re + pts[N-i].re)/2;

        pts[i].re = a_re;
        pts[i].im = a_im;
        pts[N-i].re = b_re;
        pts[N-i].im = b_im;
    }

    double re = pts[N/2].re;
    double im = pts[N/2].im;

    pts[N/2].re = pts[0].im;
    pts[N/2].im = im;
    pts[0].im = re;


    for(uint32_t i=1;i<N/4;i++){
        complex_double_t a = pts[N/2 + i];
        complex_double_t b = pts[N - i];
        pts[N/2 + i] = b;
        pts[N - i]  = a;
    }
}

void merge_spectra( complex_double_t pts[], const uint32_t N ){
    for(uint32_t i=1;i<N/4;i++){
        complex_double_t a = pts[N/2 + i];
        complex_double_t b = pts[N - i];
        pts[N/2 + i] = b;
        pts[N - i]  = a;
    }

    double t = pts[0].im;
    pts[0].im = pts[N/2].re;
    pts[N/2].re = t;

    for(uint32_t i=1;i<N/2;i++){

        double a_re = pts[i].re;
        double a_im = pts[i].im;
        double b_re = pts[N-i].re;
        double b_im = pts[N-i].im;

        pts[i].re = a_re-b_im;
        pts[i].im = a_im+b_re;
        pts[N-i].re = b_im+a_re;
        pts[N-i].im = b_re-a_im;
    }
}


