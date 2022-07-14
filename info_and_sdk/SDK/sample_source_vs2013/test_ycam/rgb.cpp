#include <stdio.h>
#include <memory.h>
#include <math.h>
#include "rgb.h"


imageRGB::imageRGB() {
	f_mem=0;
	width=height=0;
	size=osize=0;
	data=NULL;
	rgb=NULL;
	colorf_=0;
	wleft=wtop=wright=wbottom=0;
}

imageRGB::imageRGB(int w,int h) {
	f_mem=0;
	data=NULL;
	colorf_=0;
	alloc_mem(w,h);
}

imageRGB::imageRGB(int w,int h,void *ptr) {
	f_mem=1;
	data=(unsigned char*)ptr;
	colorf_=0;
	alloc_mem(w,h);
}

imageRGB::imageRGB(int w,int h,void *ptr,int c) {
	f_mem=1;
	data=(unsigned char*)ptr;
	colorf_=c;
	alloc_mem(w,h);
}

imageRGB::imageRGB(int w,int h,int c) {
	f_mem=0;
	data=NULL;
	colorf_=c;
	alloc_mem(w,h);
}

void imageRGB::free(void) {
	if(!f_mem && data) {
		delete[] data;
	}
	data=NULL;
	width=height=0;
	osize=size=0;
}

void imageRGB::clear(int flag) {
	if(data) {
		memset(data,flag ? 255:0,osize);
	}
}

void imageRGB::change_size(int w,int h) {
	if(osize<w*h*point_size()) {
		alloc_mem(w,h);
	}
	else {
		width=w;
		height=h;
		size=w*h*point_size();
	}
}

unsigned char *imageRGB::alloc_mem(int w,int h) {
	if(!f_mem && data) {
		free();
	}
	width=w;
	height=h;
	osize=size=point_size()*width*height;
	if(!f_mem)data=new unsigned char[size];
	rgb=(pointRGB*)data;
	return data;
}

pointRGB imageRGB::get_pixel(int x,int y) {
	if(x<width && y<height) {
		if (colorf_){
			return rgb[y*width+x];
		}
		else{
			//uses r-channel
			return pointRGB(data[y*width+x],0,0);
		}
	}
	pointRGB null;
	return null;
}

//pointRGB *imageRGB::get_pixel_ptr(int x,int y,int w) {
//	if(x<width && y<height) {
//		if (colorf_){
//			return &rgb[y*(w==0 ? width: w)+x];
//		}
//		else{
//			return (pointRGB*)&data[y*(w==0 ? width: w)+x];
//		}
//	}
//	else return NULL;
//}

void imageRGB::set_pixel(int x,int y,pointRGB *data) {
	if (colorf_){
		rgb[y*width+x]=*data;
	}
	else{
		this->data[y*width+x]=data->r;
	}
}

void imageRGB::diff(imageRGB *d) {
	if(d->width!=width || d->height!=height) {
		return;
	}
	for(int i=0; i<size; i++) {
		if(data[i]>=d->data[i]) {
			data[i]-=d->data[i];
		}
		else {
			data[i]=(unsigned char)(d->data[i]-data[i]);
		}
	}
}

void imageRGB::diff0(imageRGB *d) {
	if(d->width!=width || d->height!=height) {
		return;
	}
	if(d->colorf_!=colorf_){
		return;
	}
	for(int i=0; i<size; i+=point_size()) {
		if(data[i]>=d->data[i]) {
			data[i]-=d->data[i];
		}
		else {
			data[i]=(unsigned char)(d->data[i]-data[i]);
		}
		if (colorf_){
			data[i+1]=data[i+2]=data[i];
		}
	}
}

void imageRGB::copy(imageRGB *src) {
	colorf_=src->colorf_;
	wleft=src->wleft;
	wtop=src->wtop;
	wright=src->wright;
	wbottom=src->wbottom;
	change_size(src->width,src->height);
	memcpy(data,src->data,size);
}

void imageRGB::copy(imageRGB &src) {
	this->copy(&src);
}

void imageRGB::setColorf(int colorf){
	colorf_=colorf;
	if (colorf==0){
		//if color mode you must call change_size() function
		size=width*height*point_size();
	}
}

void imageRGB::makeRGB(){
	if (colorf_==1){
		return;
	}
	unsigned char *save=new unsigned char[width*height];
	int save_size = width*height;
	memcpy(save,data,save_size);
	colorf_=1;	//to color mode
	change_size(width,height);
	for (int n=0;n<save_size;++n){
		data[n*3+0]=save[n];
		data[n*3+1]=save[n];
		data[n*3+2]=save[n];
	}
	delete[] save;
}

void imageRGB::makeGRAY(){
	if (colorf_==0){
		return;
	}
	unsigned char *src=data;
	unsigned char *dst=data;
	for(int i=0; i<size; i+=3, ++dst){
		*dst = (unsigned char)((306 * src[i] + 601 * src[i+1] + 117 * src[2]) >> 10);	// (>>10) = (/1024)
	}
	colorf_=0;	//to gray mode
	change_size(width,height);
}

