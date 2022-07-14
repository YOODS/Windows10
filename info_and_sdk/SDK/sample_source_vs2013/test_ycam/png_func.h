#pragma once

//png�f�[�^�i�[�p
struct pngMem {
	unsigned char *data;	//png�f�[�^        write(out):�Ăяo�����ŗ̈�m��,read(in)
	unsigned long size;		//png�f�[�^�T�C�Y  write(out),read(out)
	pngMem():data(0), size(0){};
};

//encoder
int write_PNG(std::ostream &os, imageRGB *img);	//raw->png(stream)
int write_PNG(pngMem *mem, imageRGB *img);		//raw->png(memory)
int write_PNG(const char *path, imageRGB *img);	//raw->png(file)
//decoder
imageRGB read_PNG(std::istream &is);			//png(stream)->raw
imageRGB read_PNG(pngMem *mem);					//png(memory)->raw
imageRGB read_PNG(const char *path);			//png(file)->raw
