#include <climits>
#include "etc.h"

fn_imgst::fn_imgst()
{
	clear();
}

void fn_imgst::clear()
{
	count = 0;
	min_avg = INT_MAX;
	max_avg = INT_MIN;
	cur_avg = -1;
}

void fn_imgst::operator()(unsigned char *img, int width, int height, int stride)
{
	++count;
	int a = 0;
	unsigned char *p = img;
	for (int y = 0; y < height; ++y, p += stride){
		for (int x = 0; x < width; ++x){
			a += p[x];
		}
	}
	a = (int)((float)a / (width * height) + 0.5f);
	if (a < min_avg) min_avg = a;
	if (max_avg < a) max_avg = a;
	cur_avg = a;
}
