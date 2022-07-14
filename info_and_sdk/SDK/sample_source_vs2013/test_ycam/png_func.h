#pragma once

//pngデータ格納用
struct pngMem {
	unsigned char *data;	//pngデータ        write(out):呼び出し側で領域確保,read(in)
	unsigned long size;		//pngデータサイズ  write(out),read(out)
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
