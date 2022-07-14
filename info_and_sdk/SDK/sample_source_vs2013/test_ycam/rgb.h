#pragma once

#include "point.h"

struct imageRGB{
	unsigned char *data;
	int width,height,size,osize;
	int wleft,wtop,wright,wbottom;
	int f_mem;
	pointRGB *rgb;
	imageRGB();
	imageRGB(int w,int h);
	imageRGB(int w,int h,void *ptr);
	imageRGB(int w,int h,void *ptr,int c);
	imageRGB(int w,int h,int c);
	pointRGB get_pixel(int,int);
	pointRGB *get_pixel_ptr(int,int,int =0);
	void set_pixel(int,int,pointRGB*);
	void free(void);
	void clear(int);
	void change_size(int,int);
	unsigned char *alloc_mem(int,int);
	void diff(imageRGB*);
	void diff0(imageRGB*);
	void copy(imageRGB*);
	void copy(imageRGB&);
	int get_twof(void);
	//
	void setColorf(int colorf);
	int colorf(){ return colorf_; };
	int point_size(){ return (colorf_==1 ? 3:1); };
	void makeRGB();
	void makeGRAY();
	void set_data(unsigned char *d){
		data=d;
		rgb=(pointRGB*)data;
	};
private:
	int colorf_;	//0:monochrome,1:color
};

inline pointRGB *imageRGB::get_pixel_ptr(int x,int y,int w) {
	if(x<width && y<height) {
		if (colorf_){
			return &rgb[y*(w==0 ? width: w)+x];
		}
		else{
			return (pointRGB*)&data[y*(w==0 ? width: w)+x];
		}
	}
	else return 0;
}

