/*
 * その他クラス群
 */
#pragma once

//画像の平均輝度計算
struct fn_imgst {
	unsigned int count;		//カウント
	int min_avg;	//最小平均
	int max_avg;	//最大平均
	int cur_avg;	//直近平均
	void operator()(unsigned char *img, int width, int height, int stride);
	fn_imgst();
	void clear();
};
