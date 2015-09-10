//����һ����֤��ʶ���࣬�ṩ�˶�����֤��ͼƬ������ԡ� ÿ�����Է��������ṩ��ͬ��Ĭ�ϲ����������������������Ҳ����������ϣ���ʵ�ָ��õ�ͼƬ����Ч����
//����ͼ�������opencv����ȡ�����ַ�ͼƬУ���󣬽���tesseract-ocrʶ��
//do_recognize������һ���Դ���demo������˻ҶȻ����Զ���ֵ�ָ�˲���ȥ�߿��ַ��ָ�ַ�У�����ַ�����Ȳ��ԣ�Ȼ�󽫴���õ�ͼƬ����ocr����ʶ��
//ʹ���߿��Ը���ͼƬ�����ԣ�ѡ����ʵĲ��ԣ���ͼƬ�ϵ����������������
//�ṩ�������ܡ�
//��ǰ���뱳���Աȶȴ󡢲�ճ����Ӣ�ģ��ָ���У���Ĵ���Ч���ȽϺã����ַ�ճ���ģ�����ʱ��ȽϽ�����û�ṩճ���ַ��ָ�ķ��������������ơ�
//���ĵ���Ҫ�޸����е�ocr���������л�Ϊ����ʶ��������֤���е������ַ��ܶ��Ǳ��εģ���ʹ���ø��ַ������ַ����������
//��tesseract-ocr�ṩ�������ֿ⣬�Ա��ε�����ʶ���ʺܵͣ�����Ҫ���ݴ��������֤���ַ�������ѵ��ocr�ֿ⣬�����׼ȷ�ʡ�
//���������������ͼƬ������ԣ�����ǿ�ַ�ͼƬ�����ȣ����tesseract ocrʶ���ʡ�

#pragma once

#pragma warning(push)
#pragma warning(disable:4819)
#include <opencv2/opencv.hpp>
#pragma warning(pop)
#ifdef _DEBUG
	#pragma comment(lib,"opencv_world300d.lib")
	#pragma  comment(lib,"libtesseract302d.lib")
#else
	#pragma comment(lib,"opencv_world300.lib")
	#pragma  comment(lib,"libtesseract302.lib")
#endif

#include "strngs.h"
#include "baseapi.h"

#include "../based/headers_all.h"

#ifdef DEBUG_SHOW_IMG
	#define SHOW_IMG(IMG_WINDOW_NAME,IMG_NAME)		cvShowImage(IMG_WINDOW_NAME, IMG_NAME)
#else
	#define SHOW_IMG(IMG_WINDOW_NAME,IMG_NAME)
#endif


class img_recognize
{
public:
	//��֤��ͼ��ʶ��filename����֤��ͼƬ��·��; char_count_needed��Ҫʶ����ַ�����; method:true,�ָ����ַ�ͼƬУ����ֱ��ʶ��,false�������Ϊ��ͼ��ʶ��; debug_mode:�Ƿ��������ģʽ
	//������ϸ�����������ʵ�ֲ�ͬ��ͼƬ�������̣�Ҳ����Ϊÿ�������ṩ��ͬ��Ĭ�ϲ�����������������ʵ�ָ��õĴ���Ч����
	virtual std::string do_recognize(char* filename, int char_count_needed = 4, bool recg_one_by_one = false, bool debug_mode = true)
	{
		std::string final_str;

		//����ͼ��
		IplImage *img = cvLoadImage(filename);
		ENSURE(img && img->imageData)(img).warn();
		SCOPE_GUARD([&] {cvReleaseImage(&img);});

		//ת�Ҷ�ͼ
		IplImage* img_gray = cvCreateImage(cvGetSize(img), IPL_DEPTH_8U, 1);
		SCOPE_GUARD([&] {cvReleaseImage(&img_gray);});
		cvCvtColor(img, img_gray, CV_BGR2GRAY);	
		SHOW_IMG("after gray", img_gray);

		//�Զ���ֵ�ָ����ֵͼ��
		threshold_otsu(img_gray,2);
		SHOW_IMG("after threshold", img_gray);

		//��ֵ�˲�ȥ��
		cv::Mat mat_blur = cv::cvarrToMat(img_gray);
		//medianBlur(mat_blur, mat_blur, 5);
		SHOW_IMG("after blur", img_gray);

		//ȥ�߿�
		move_border(img_gray);
		SHOW_IMG("after move border", img_gray);

		//�ַ��ָ�
		IplImage** splitted_imgs = split_char_by_shadow(img_gray, char_count_needed);
		SCOPE_GUARD([&] {
			for (int i = 0;i < char_count_needed;i++)
				cvReleaseImage(splitted_imgs+i);
			delete[] splitted_imgs;
		});
		if (char_count_needed == 0)
			return "";

		//�Էָ����ÿ���ַ�ͼ����У�������ʶ�𣬻�У��������Ϊ������ͼ��һ����ʶ��
		IplImage** rectified_imgs = new IplImage*[char_count_needed];
		for (int i = 0;i < char_count_needed;i++)
		{
			SHOW_IMG((std::string("before rotate rectify ") + std::to_string(i)).c_str(), splitted_imgs[i]);
			rectified_imgs[i] = rectify_rotate_char_img(splitted_imgs[i]);
			SHOW_IMG((std::string("after rotate rectify ") + std::to_string(i)).c_str(), rectified_imgs[i]);

			if (recg_one_by_one)
			{
				char* dst_path = "f:\\aaa.png";
				cvSaveImage(dst_path, rectified_imgs[i]);
				final_str += ocr(dst_path);
			}
		}
		SCOPE_GUARD([&] {
			for (int i = 0;i < char_count_needed;i++)
				cvReleaseImage(rectified_imgs+i);
			delete[] rectified_imgs;
		});

		if (!recg_one_by_one)
		{
			IplImage* dst = combine_char_image(rectified_imgs, char_count_needed);
			SCOPE_GUARD([&] {cvReleaseImage(&dst);});

			SHOW_IMG("final img", dst);
			char* dst_path = "f:\\aaa.png";
			cvSaveImage(dst_path, dst);
			final_str = ocr(dst_path);
		}
		
		return final_str;
	}

	//�����������ȡ��ͼƬ��save_full_path�������չ�����С�start_x,start_yҪ��ȡ����ʼ���꣬end_x��end_yҪ��ȡ����ֹ���ꡣ���궼���������Ļ���Ͻǣ����Ͻ�����0��0����
	void copy_screen(const char* save_full_path, int start_x, int start_y, int end_x, int end_y)
	{
	#ifdef WIN32
		HDC dc_screen = ::GetDC(NULL);
		ENSURE_WIN32(dc_screen != NULL).warn();
		SCOPE_GUARD([&] { ReleaseDC(NULL,dc_screen); });

		int screen_width = GetDeviceCaps(dc_screen, HORZRES);
		int screen_height = GetDeviceCaps(dc_screen, VERTRES);
		start_x < 0 ? start_x = 0 : void();
		start_y < 0 ? start_y = 0 : void();
		end_x > screen_width ? end_x = screen_width : void();
		end_y > screen_height ? end_y = screen_height : void();

		HDC dc_mem = CreateCompatibleDC(dc_screen);
		SCOPE_GUARD([&] { DeleteDC(dc_mem); });

		HBITMAP comp_bmp = CreateCompatibleBitmap(dc_screen, end_x - start_x + 1, end_y - start_y + 1);
		SCOPE_GUARD([&] { DeleteObject(comp_bmp); });

		HBITMAP old_bmp = (HBITMAP)SelectObject(dc_mem, comp_bmp);
		BitBlt(dc_mem, 0, 0, end_x - start_x + 1, end_y - start_y + 1, dc_screen, start_x, start_y, SRCCOPY);
		comp_bmp = (HBITMAP)SelectObject(dc_mem, old_bmp);

		BITMAPINFOHEADER bih{};
		bih.biSize = sizeof(BITMAPINFOHEADER);
		bih.biWidth = end_x - start_x + 1;
		bih.biHeight = end_y - start_y + 1;
		bih.biPlanes = 1;
		bih.biBitCount = 32;
		bih.biCompression = BI_RGB;
		int line_bytes = (bih.biWidth*bih.biBitCount + bih.biBitCount-1)/ bih.biBitCount * 4;
		char* bmp_data = new char[line_bytes*bih.biHeight];
		SCOPE_GUARD([&] { delete[] bmp_data; });

		ENSURE_WIN32(GetDIBits(dc_mem, comp_bmp, 0, bih.biHeight, bmp_data, (BITMAPINFO*)&bih, DIB_RGB_COLORS) != 0).warn();

		IplImage* dst = cvCreateImage(cvSize(bih.biWidth,bih.biHeight),IPL_DEPTH_8U,4);
		SCOPE_GUARD([&] { cvReleaseImage(&dst); });

		for (int i = 0;i < bih.biHeight;i++)
			for (int j = 0;j < bih.biWidth;j++)
			{
				dst->imageData[i*line_bytes + j * 4 + 0] = bmp_data[(bih.biHeight-i-1)*line_bytes + j * 4 + 0];
				dst->imageData[i*line_bytes + j * 4 + 1] = bmp_data[(bih.biHeight-i-1)*line_bytes + j * 4 + 1];
				dst->imageData[i*line_bytes + j * 4 + 2] = bmp_data[(bih.biHeight-i-1)*line_bytes + j * 4 + 2];
				dst->imageData[i*line_bytes + j * 4 + 3] = bmp_data[(bih.biHeight-i-1)*line_bytes + j * 4 + 3];
			}

		std::string str_path = save_full_path;
		std::replace(str_path.begin(), str_path.end(), '/', '\\');
		str_path.erase(str_path.rfind('\\'));
		based::os::make_dir_recursive(str_path.c_str());
		cvSaveImage(save_full_path, dst);
	#else
		#error "Please add corresponding platform's code."
	#endif
	}

	//���������浽save_full_path�������չ�����С�start_x,start_yҪ��ȡ����ʼ���꣬width��heightҪ��ȡ�Ŀ�Ⱥ͸߶ȡ������������Ļ���Ͻǣ����Ͻ�����0��0����
	void copy_screen(const char* save_full_path, int start_x, int start_y, short width, short height)
	{
		copy_screen(save_full_path, start_x, start_y, start_x + width, start_y + height);
	}

protected:
	//ȥ�߿�src:256ɫ��ͨ���Ҷ�ͼ, x��x���� y��y����
	void move_border(IplImage* src, int x = 2, int y = 2)
	{
		ENSURE(src->nChannels == 1)(src->nChannels).warn();

		if (src->width < y || src->height < x)
			return;

		int width = src->width;
		int height = src->height;
		int line_bytes = (width*src->nChannels + 3) / 4 * 4;
		char* src_data = src->imageData;

		for (int i = 0;i < height;i++)
			for (int j = 0;j < width;j++)
			{
				if (!(i >= y && i <= height - y && j >= x && j <= width - x))
					src_data[i*line_bytes + j] = (char)0xff;
			}
	}

	//ʹ�����ӷŴ����Ӻ��otsu�㷨��Ѱ�������ֵ�����Դ���ֵ����ֵ������src:256ɫ��ͨ���Ҷ�ͼ boost_scale:�Ŵ�����(1--n)��Ĭ��3, ���������ֵ
	int threshold_otsu(IplImage* src, double boost_scale = 3)
	{
		ENSURE(src->nChannels == 1 && boost_scale > 0)(src->nChannels)(boost_scale).warn();

		unsigned int h[256]{};
		double g[256]{};
		int height = src->height;
		int width = src->width;
		int line_bytes = (width * src->nChannels + 3) / 4 * 4;
		unsigned char* src_data = (unsigned char*)src->imageData;

		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
			h[*(src_data + i*line_bytes + j)]++;

		for (int i = 0;i < 256;i++)
		{
			double fore_count = 0, bkg_count = 0;
			double u0 = 0, u1 = 0, u = 0;
			for (int j = 0;j < 256;j++)
			{
				if (j < i)
				{
					fore_count += h[j] * boost_scale;		//��otsu�����ϷŴ�3��
					u0 += j*h[j];
				}
				else
				{
					bkg_count += h[j] * boost_scale;		//��otsu�����ϷŴ�3��
					u1 += j*h[j];
				}
			}

			if (fore_count != 0)
				u0 /= fore_count;
			double w0 = fore_count / (height*width);

			if (bkg_count != 0)
				u1 /= bkg_count;
			double w1 = 1 - w0;

			g[i] = w0*w1*(u1 - u0)*(u1 - u0);
		}

		int idx = 0;
		double gmax = 0;
		for (int i = 0; i<256; i++)
			if (g[i]>gmax)
			{
				gmax = g[i];
				idx = i;
			}

		for (int i = 0; i < height; i++)
			for (int j = 0; j<width; j++)
			*(src_data + i*line_bytes + j) = *(src_data + i*line_bytes + j)>idx ? 0xff : 0;

		return idx;
	}

	//����������ɢ�㣺src:256ɫ��ͨ���Ҷ�ͼ, scale:�������Ӵ�С(1 3 5 7)������������, threshold:ǿ��(0.0--1.0)
	//������ʱ�������x,yΪ���Ŀ��Ϊscale�Ĵ��ڵ�ƽ������ֵ���������threshold������Ϊ����ɢ��㣬����Ϊ��ɫ��
	void move_discrete_point(IplImage* src, int scale, double threshold)
	{
		ENSURE(src->nChannels == 1 && scale > 0 && scale % 2 == 1)(src->nChannels)(scale).warn();

		if (src->width < scale || src->height < scale)
			return;

		char* src_data = src->imageData;
		int height = src->height;
		int width = src->width;
		int line_bytes = (width * src->nChannels + 3) / 4 * 4;
		scale = (scale - 1) / 2;
		unsigned char* dst_data = new unsigned char[line_bytes*height];
		SCOPE_GUARD([&] { delete[] dst_data; });

		for (int i = 0;i < height;i++)
			for (int j = 0;j < width;j++)
			dst_data[i*line_bytes + j] = 0xff;

		for (int i = scale;i < height - scale;i++)
			for (int j = scale;j < width - scale;j++)
			{
				double scale_value_sum = 0;
				for (int k = i - scale;k <= i + scale;k++)
					for (int n = j - scale;n <= j + scale;n++)
					scale_value_sum += (unsigned char)(0xff - src_data[k*line_bytes + n]);

				if (scale_value_sum < ((double)((unsigned char)((unsigned char)0xff - src_data[i*line_bytes + j]) * (scale * 2 + 1)*(scale * 2 + 1)))*threshold)
					dst_data[i*line_bytes + j] = 0xff;
				else
					dst_data[i*line_bytes + j] = src_data[i*line_bytes + j];
			}

		for (int i = scale;i < height - scale;i++)
			for (int j = scale;j < width - scale;j++)
			src_data[i*line_bytes + j] = dst_data[i*line_bytes + j];
	}

	//����ͼ����x���ͶӰ����ɫ������Ӱ�ӣ�src:��ֵͼ��; buf_len:����ͶӰ����ĳ���; ����ֵΪͶӰ��������ַ����������ͷ�
	char* shadow_x(IplImage* src, int& buf_len)
	{
		ENSURE(src->nChannels == 1)(src->nChannels).warn();

		int width = src->width;
		int height = src->height;
		char* data = src->imageData;
		int line_bytes = (width * src->nChannels + 3) / 4 * 4;

		buf_len = width;
		char* buf_shadow = new char[buf_len];
		for (int i = 0;i < height;i++)
		{
			for (int j = 0;j < width;j++)
			{
				if ((unsigned char)*(data + i*line_bytes + j) == (unsigned char)0)
					buf_shadow[j] = 1;
			}
		}

		return buf_shadow;
	}

	//ͶӰ���ָ��ַ���src:256ɫ��ͨ���Ҷ�ͼ, dst_char_count:������Ҫȡ�õ��ַ�����������ʵ�ʷָ�����ַ��ĸ����� ���طָ�õ�ͼƬ����ָ�룬��������ͷ�
	IplImage** split_char_by_shadow(IplImage* src, int& dst_char_count)
	{
		ENSURE(src->nChannels == 1)(src->nChannels).warn();

		int real_char_count = 0;
		int height = src->height;
		int width = src->width;
		int line_bytes = (width * src->nChannels + 3) / 4 * 4;
		char* src_data = src->imageData;

		struct character
		{
			int begin;
			int end;
		};

		character* real_chars = new character[width]{};	
		char* first_line = shadow_x(src, width);
		SCOPE_GUARD([&] { delete[] real_chars; });
		SCOPE_GUARD([&] {delete[] first_line;});

		bool continous = false;
		for (int i = 0;i < width;i++)
		{
			if (first_line[i] == 1)
			{
				if (!continous)
				{
					real_char_count++;
					real_chars[real_char_count - 1].begin = i;
					continous = true;
				}
			}
			else
			{
				if (continous)
				{
					real_chars[real_char_count - 1].end = i - 1;
					continous = false;
				}
			}
		}
		if (real_chars[real_char_count - 1].end == 0)
			if (--real_char_count < 0)
			{
				real_char_count = 0;
				return NULL;
			}
		if (real_char_count < dst_char_count)
			dst_char_count = real_char_count;

		character* dst_chars = new character[dst_char_count];
		SCOPE_GUARD([&] { delete[] dst_chars; });

		for (int i = 0;i < dst_char_count;i++)
			dst_chars[i] = real_chars[i];

		for (int i = dst_char_count;i < real_char_count;i++)
		{
			int min_width = real_chars[i].end - real_chars[i].begin;
			int min_index = i;
			for (int j = 0;j < dst_char_count;j++)
			{
				if (dst_chars[j].end - dst_chars[j].begin < min_width)
				{
					min_width = dst_chars[j].end - dst_chars[j].begin;
					min_index = j;
				}
			}
			if (min_index == i)
				continue;

			for (int n = min_index + 1;n < dst_char_count;n++)
				dst_chars[n-1] = dst_chars[n];
			dst_chars[dst_char_count-1] = real_chars[i];
		}

		IplImage** dst_img = new IplImage*[dst_char_count];
		for (int i = 0;i < dst_char_count;i++)
		{
			int dst_img_width = dst_chars[i].end - dst_chars[i].begin + 1;
			int dst_img_line_bytes = (dst_img_width*src->nChannels + 3) / 4 * 4;
			dst_img[i] = cvCreateImage(cvSize(dst_img_width, height), src->depth, src->nChannels);
			char* dst_img_data = dst_img[i]->imageData;

			for (int j = 0;j < height;j++)
				for (int k = 0;k < dst_img_width;k++)
				{
					dst_img_data[j*dst_img_line_bytes + k] = src_data[j*line_bytes + dst_chars[i].begin + k];
				}
		}

		return dst_img;
	}

	//��תͼ��src:256ɫ��ͨ���Ҷ�ͼ, angle:��ת�Ƕȣ�����Ϊ��ʱ�룬����Ϊ˳ʱ��, ������ת���ͼ����Ҫ�������ͷ�
	IplImage* rotate_image(IplImage* src, int angle)
	{
		ENSURE(src->nChannels == 1)(src->nChannels).warn();

		int positive_angle = abs(angle) % 180;
		if (positive_angle > 90)
			positive_angle = 90 - (positive_angle % 90);

		double dst_width;
		double dst_height = src->height;
		get_height_width_after_rotate(src,angle,NULL, &dst_width);
		if (dst_width - (int)dst_width > 0.000001)
			dst_width = (int)dst_width + 1;

		int tempLength = (int)sqrt((double)src->width * src->width + src->height * src->height) + 10;
		int tempX = (int)((tempLength + 1) / 2.0 - src->width / 2.0);
		int tempY = (int)((tempLength + 1) / 2.0 - src->height / 2.0);

		IplImage* dst = cvCreateImage(cvSize((int)(dst_width+4), (int)dst_height), src->depth, src->nChannels);
		cvSet(dst, 0xff);
		IplImage* temp = cvCreateImage(cvSize(tempLength, tempLength), src->depth, src->nChannels);
		cvSet(temp, 0xff);

		cvSetImageROI(temp, cvRect(tempX, tempY, src->width, src->height));
		cvCopy(src, temp, NULL);
		cvResetImageROI(temp);

		float m[6];
		m[0] = (float)cos(-angle * CV_PI / 180.);
		m[1] = (float)sin(-angle * CV_PI / 180.);
		m[3] = -m[1];
		m[4] = m[0];
		m[2] = temp->width * 0.5f;
		m[5] = temp->height * 0.5f;

		CvMat M = cvMat(2, 3, CV_32F, m);
		cvGetQuadrangleSubPix(temp, dst, &M);
		cvReleaseImage(&temp);

		return dst;
	}

	//����ͼ����תangle�ǶȺ�Ŀ�͸ߣ�src:��ֵͼ��; angle����Ϊ��ʱ����ת������Ϊ˳ʱ����ת�����new_height��new_width��ΪNULL,�򽫿�߱�����new_height��new_width��
	void get_height_width_after_rotate(IplImage* src, int angle,double* new_height,double* new_width)
	{
		ENSURE(src->nChannels == 1)(src->nChannels).warn();

		int height = src->height;
		int width = src->width;
		int line_bytes = (width*src->nChannels + 3) / 4 * 4;
		char* src_data = src->imageData;

		double min_x = std::numeric_limits<double>::max();
		double max_x = std::numeric_limits<double>::min();
		double min_y = std::numeric_limits<double>::max();
		double max_y = std::numeric_limits<double>::min();

		for (int j = 0;j < height;j++)
		{
			for (int k = 0;k < width;k++)
			{
				if (src_data[j*line_bytes + k] == 0)
				{
					double pix_new_x = (k - width*0.5f)*cos(angle / 180. * CV_PI) - (height*0.5f - j)*sin(angle / 180. * CV_PI);
					double pix_new_y = (k - width*0.5f)*sin(angle / 180. * CV_PI) + (height*0.5f - j)*cos(angle / 180. * CV_PI);
					if (pix_new_x < min_x)
						min_x = pix_new_x;
					if (pix_new_x > max_x)
						max_x = pix_new_x;
					if (pix_new_y < min_y)
						min_y = pix_new_y;
					if (pix_new_y > max_y)
						max_y = pix_new_y;
				}
			}
		}
		if(new_width != NULL)
			*new_width = max_x - min_x;
		if(new_height != NULL)
			*new_height = max_y - min_y;
	}

	//���ַ�ͼ������תУ����src:256ɫ��ͨ���Ҷ�ͼ��max_angle_left��������Ƕȣ�max_angle_right��������Ƕȣ�������������ͼ����������ͷ�
	IplImage* rectify_rotate_char_img(IplImage* src, int max_angle_left = -20, int max_angle_right = 20)
	{
		ENSURE(src->nChannels == 1)(src->nChannels).warn();

		int height = src->height;
		int width = src->width;
		int line_bytes = (width*src->nChannels + 3) / 4 * 4;
		char* src_data = src->imageData;
		double min_diagonal_len_2 = std::numeric_limits<double>::max();
		int right_angle = 0;

		for (int i = max_angle_left; i <= max_angle_right; i += 1)
		{
			double new_height;
			double new_width;
			get_height_width_after_rotate(src,i,&new_height,&new_width);
			
			double diagonal_len_2 = new_width*new_width + new_height*new_height;
			if (diagonal_len_2 < min_diagonal_len_2)
			{
				min_diagonal_len_2 = diagonal_len_2;
				right_angle = i;
			}
		}

		if (right_angle == 0)
		{
			IplImage* dst = cvCreateImage(cvSize(src->width+2, src->height+2), src->depth, src->nChannels);
			cvSet(dst,0xff);
			cvSetImageROI(dst,cvRect(1,1,src->width,src->height));
			cvCopy(src,dst);
			cvResetImageROI(dst);
			return dst;
		}
		return rotate_image(src, right_angle);
	}

	//������ϵ����ַ�ͼ��:src_char_img:Ҫ��ϵ��ַ�ͼ��ָ�����飬dst_char_count:ͼ��ĸ�����fill_color����϶�����ɫ��space_x:x�����϶,space_y:y�����϶������������ͼ����������ͷ�
	IplImage* combine_char_image(IplImage** src_char_img, int dst_char_count, unsigned char fill_color = 0xff, int space_x = 4, int space_y = 1)
	{
		ENSURE(dst_char_count>0)(dst_char_count).warn();

		int height = -1;
		int width = (dst_char_count + 1)*space_x;
		for (int i = 0;i < dst_char_count;i++)
		{
			width += src_char_img[i]->width;
			if (src_char_img[i]->height > height)
				height = src_char_img[i]->height;
		}
		height += (space_y * 2);

		IplImage* dst = cvCreateImage(cvSize(width, height), src_char_img[0]->depth, src_char_img[0]->nChannels);
		cvSet(dst, fill_color);

		int roi_x = space_x;
		int roi_y = space_y;
		for (int i = 0;i < dst_char_count;i++)
		{
			cvSetImageROI(dst, cvRect(roi_x, roi_y, src_char_img[i]->width, src_char_img[i]->height));
			cvCopy(src_char_img[i], dst);
			cvResetImageROI(dst);
			roi_x += (src_char_img[i]->width + space_x);
		}

		return dst;
	}

	//��img_path��Ӧ��ͼƬ����ocrʶ��, method:ʶ��ʽ, ����ʶ�����ַ���
	virtual std::string ocr(const char* img_path, int method = 0)
	{
		tesseract::TessBaseAPI  api;

		//�������Կ⣬���ļ��壺chi_sim;Ӣ�ģ�eng��Ҳ�����Լ�ѵ�����Կ�
		api.Init(NULL, "eng", tesseract::OEM_DEFAULT);
		api.SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

		STRING text_out = "";
		if (!api.ProcessPages(img_path, NULL, 0, &text_out))
		{
			std::cout << "ocr exception" << std::endl;
		}

		std::string str_text_u8 = text_out.string();
		std::string str_text = based::charset_convert().utf8_string_to_string(str_text_u8);
		based::string_more::replace_all<std::string>(str_text, "\n", "");

		return str_text;
	}
};

// example:
// #include "img_recognize.h"
// int main(int argc, char* argv[])
// {
// 	std::cout << "Please get ready,then press enter." << std::endl;
// 	getchar();
// 
//  //��֤������Ļ�е���ʼλ�ã��������Ļ���Ͻǣ����У����Ͻ�����Ϊ0��0����600,374�����Ϊ413���߶�Ϊ120
// 	std::string& screen_save_path = based::os::get_exe_path_without_backslash<char>() + "\\copy_screen.bmp";
// 	img_recognize().copy_screen(screen_save_path.c_str(),600, 374, (short)413, (short)120);
// 	std::cout << img_recognize().do_recognize((char*)screen_save_path.c_str()) << std::endl;
// 
// 	cvWaitKey(0);
// 
// 	getchar();
// 	return 0;
// }