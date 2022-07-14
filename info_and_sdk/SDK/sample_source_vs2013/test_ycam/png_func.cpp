#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <png.h>
#include "rgb.h"
#include "png_func.h"

static int write_png(imageRGB *img, png_voidp io_ptr, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn)
{
	png_structp png_ptr;
	png_infop   info_ptr;
	{
		// png_struct
		png_ptr = png_create_write_struct(
			PNG_LIBPNG_VER_STRING,
			NULL,
			NULL,
			NULL);
		if (png_ptr == NULL){
			return -1;
		}

		// png_info
		info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == NULL){
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			return -1;
		}

		png_set_write_fn(png_ptr, io_ptr, write_data_fn, output_flush_fn);
	}

	// âÊëúèÓïÒÇÃéÊìæ
	png_uint_32 width = img->width;
	png_uint_32 height = img->height;
	int bit_depth = 8;
	int color_type = img->colorf() ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY;
	int interlace_type = PNG_INTERLACE_NONE;
	int compression_type = PNG_COMPRESSION_TYPE_BASE;
	int filter_type = PNG_FILTER_TYPE_BASE;
	{
		png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
			interlace_type, compression_type, filter_type);
		png_write_info(png_ptr, info_ptr);
	}

	const int stride = img->width * (img->colorf() ? 3 : 1);
	std::vector<png_bytep> row_pointers;
	{
		unsigned char *p = img->data;
		for (int y = 0; y<img->height; y++){
			row_pointers.push_back(p);
			p += stride;
		}
	}

	//png_set_bgr(png_ptr);
	png_write_image(png_ptr, &row_pointers[0]);

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return 0;
}

static imageRGB read_png(png_voidp io_ptr, png_rw_ptr read_data_fn)
{
	png_structp png_ptr;
	png_infop   info_ptr;
	{
		// png_struct
		png_ptr = png_create_read_struct(
			PNG_LIBPNG_VER_STRING,
			NULL,
			NULL,
			NULL);
		if (png_ptr == NULL){
			return imageRGB();
		}
		// png_info
		info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == NULL){
			png_destroy_read_struct(&png_ptr, NULL, NULL);
			return imageRGB();
		}
		png_set_read_fn(png_ptr, io_ptr, read_data_fn);
	}

	// âÊëúèÓïÒÇÃéÊìæ
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	int compression_type, filter_type;
	{
		png_read_info(png_ptr, info_ptr);
		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
			&interlace_type, &compression_type, &filter_type);
	}

	const int size = width * height;
	const int colorf = (color_type == PNG_COLOR_TYPE_GRAY || (color_type & PNG_COLOR_MASK_PALETTE)) ? 0 : 1;
	imageRGB img(width, height, colorf);

	const int stride = img.width * (img.colorf() ? 3 : 1);
	std::vector<png_bytep> row_pointers;
	{
		unsigned char *p = img.data;
		for (int y = 0; y<img.height; y++){
			row_pointers.push_back(p);
			p += stride;
		}
	}

	//png_set_bgr(png_ptr);
	png_read_image(png_ptr, &row_pointers[0]);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

	return img;
}
////////// use stream
static void fn_stread(png_structp png_ptr, png_bytep data, png_size_t length){
	std::istream *is = (std::istream*)png_get_io_ptr(png_ptr);
	is->read((char*)data, length);
}

static void fn_stwrite(png_structp png_ptr, png_bytep data, png_size_t length){
	std::ostream *os = (std::ostream*)png_get_io_ptr(png_ptr);
	os->write((const char*)data, length);
}

static void fn_stflush(png_structp png_ptr){
	std::ostream *os = (std::ostream*)png_get_io_ptr(png_ptr);
	os->flush();
}

// raw->png(stream)
int write_PNG(std::ostream &os, imageRGB *img){
	return write_png(img, &os, fn_stwrite, fn_stflush);
}

// png(stream)->raw
imageRGB read_PNG(std::istream &is){
	return read_png(&is, fn_stread);
}
////////// use stream

////////// use primitive pointer
static void fn_ptread(png_structp png_ptr, png_bytep data, png_size_t length){
	pngMem *mem = (pngMem*)png_get_io_ptr(png_ptr);
	memcpy(data, mem->data + mem->size, length);
	mem->size += (unsigned long)length;
}

static void fn_ptwrite(png_structp png_ptr, png_bytep data, png_size_t length){
	pngMem *mem = (pngMem*)png_get_io_ptr(png_ptr);
	memcpy(mem->data + mem->size, data, length);
	mem->size += (unsigned long)length;
}
static void fn_ptflush(png_structp png_ptr){}

// raw->png(memory)
int write_PNG(pngMem *mem, imageRGB *img){
	mem->size = 0;
	return write_png(img, mem, fn_ptwrite, fn_ptflush);
}

// raw->png(file)
int write_PNG(const char *path, imageRGB *img){
	std::ofstream ofs(path, std::ios::binary);
	if (!ofs)return -1;
	return write_PNG(ofs, img);
}


// png(memory)->raw
imageRGB read_PNG(pngMem *mem){
	mem->size = 0;
	return read_png(mem, fn_ptread);
}

//png(file)->raw
imageRGB read_PNG(const char *path){
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) return imageRGB();
	return read_PNG(ifs);
}
