/*
 * ���̑��N���X�Q
 */
#pragma once

//�摜�̕��ϋP�x�v�Z
struct fn_imgst {
	unsigned int count;		//�J�E���g
	int min_avg;	//�ŏ�����
	int max_avg;	//�ő啽��
	int cur_avg;	//���ߕ���
	void operator()(unsigned char *img, int width, int height, int stride);
	fn_imgst();
	void clear();
};
