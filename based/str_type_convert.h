#pragma once

#if defined(_MSC_VER) && _MSC_VER < 1800
#error "Compiler need to support c++11, please use vs2013 or above, vs2015 e.g."
#endif

#include "headers_dependency.h"

namespace based
{
	//�ַ������������͵�ת��
	class str_type_convert
	{
	public:
		//��װ���飬ʹ�����ܷ���ch[n]����
		struct char_array_type
		{
			char array[32];
		};

		template<typename T>
		bool to_type(T& dest, const char* src, unsigned long src_length=0)
		{
			if (src == NULL || (src_length == 0 && (src_length = strlen(src)) == 0))
			{
				dest = T{};
				return false;
			}

			return _to_type(dest, src, src_length),true;
		}
		
		//�����ַ������ʵı�ת��Ŀ�꣬ת�����Ƿ�׷�����ţ������Ż�˫���ţ�
		void set_quote_when_characters_to_str(char quote = '\0')
		{
			quote_when_characters_to_str_ = quote;
		}

		//��srcת��Ϊ�ַ��������ص�char_array_type��
		template <typename T>
		char_array_type to_str(const T& src)
		{
			return _to_str(src);
		}

		//to_str��ָ��ƫ�ػ��棬��srcΪָ������ʱ������ƥ��
		template<typename T>
		char_array_type to_str(T* src, int len = sizeof(size_t))
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "0x%08X", src);
			return str;
		}

		//to_str���ַ���ָ���ػ��棬��srcΪ�ַ���ָ��ʱ������ƥ��
		const char* to_str(char* src, int src_len = 0)
		{
			static int buf_len = 256;
			static std::unique_ptr<char> ptr(new char[buf_len]);

			if(src_len == 0)
				src_len = strlen(src);

			if (quote_when_characters_to_str_ != '\0')
			{
				if (buf_len < src_len + 2 + 1)
					buf_len = src_len + 2 + 1;

				ptr.reset(new char[buf_len]);

				ptr.get()[0] = quote_when_characters_to_str_;
				memcpy(ptr.get() + 1, src, src_len);
				ptr.get()[src_len + 1] = quote_when_characters_to_str_;
				ptr.get()[src_len + 2] = 0;
			}
			else
			{
				if (buf_len < src_len + 1)
					buf_len = src_len + 1;

				ptr.reset(new char[buf_len]);
				strcpy_s(ptr.get(), buf_len,src);
			}
	
			return ptr.get();
		}

		//to_str���ַ���ָ���ػ��棬��pΪconst�ַ���ָ��ʱ������ƥ��
		const char* to_str(const char* src, int src_len = 0)
		{
			return to_str((char*)src,src_len);
		}

		//��������ΪsrcΪstd::string�ַ���ʱ���ã���ʱ������ƥ�䱾������
		const char* to_str(const std::string& src)
		{
			return to_str(src.c_str());
		}

		//������ַ���ָ�룬���辭��ת����ֱ�ӷ���
		const char* pointer_of_str(const char* str)
		{
			return str;
		}

		//�����char_array_type������������תΪ�ַ���������ݴ洢�����ڡ� so������ĵ�ַ����ת�������ݵĵ�ַ��
		const char* pointer_of_str(const char_array_type& str)
		{
			return (const char*)&str;
		}

	protected:
		//str_to_type�ľ���ʵ�ֺ������ػ�����������������ͣ�������ʾʧ�ܡ�ȷʵ��Ҫ�ģ����ֶ�������Ӧ���͵��ػ�ʵ�֡�
		template<typename T>
		void _to_type(T& dest, const char* src, unsigned long src_length)
		{
			static_assert(false, "no implementation for type T by default, add your own specialization version for T first.");
		}

		//_type_to_str��int�ػ��汾��ת��int����Ϊ�ַ���
		template<> void _to_type(int& dest, const char* src, unsigned long src_length)
		{
			dest = (int)strtol(src, NULL, 10);
		}

		template<> void _to_type(unsigned& dest, const char* src, unsigned long src_length)
		{
			dest = (unsigned)strtoul(src, NULL, 10);
		}

		template<> void _to_type(long& dest, const char* src, unsigned long src_length)
		{
			dest = strtol(src, NULL, 10);
		}

		template<> void _to_type(unsigned long& dest, const char* src, unsigned long src_length)
		{
			dest = strtoul(src, NULL, 10);
		}

		template<> void _to_type(long long& dest, const char* src, unsigned long src_length)
		{
			dest = strtoll(src, NULL, 10);
		}

		template<> void _to_type(unsigned long long& dest, const char* src, unsigned long src_length)
		{
			dest = strtoull(src, NULL, 10);
		}

		template<> void _to_type(double& dest, const char* src, unsigned long src_length)
		{
			dest = strtold(src, NULL);
		}

		template<> void _to_type(float& dest, const char* src, unsigned long src_length)
		{
			dest = strtof(src, NULL);
		}

		template<> void _to_type(char& dest, const char* src, unsigned long src_length)
		{
			dest = src[0];
		}

		template<> void _to_type(bool& dest, const char* src, unsigned long src_length)
		{
			dest = *(const bool*)src;
		}

		template<> void _to_type(std::string& dest, const char* src, unsigned long src_length)
		{
			dest = std::string(src, src_length);
		}

		template<> void _to_type(char*& dest, const char* src, unsigned long src_length)
		{
			#pragma warning(push)
			#pragma warning(disable:4996)
			strcpy(dest, src);
			#pragma warning(pop)
		}

		template<> void _to_type(std::tm& dest, const char* src, unsigned long src_length)
		{
			memset((char*)&dest, 0, sizeof(std::tm));

			char* buf = (char*)src;
			long ele[6];

			for (int i = 0; i < 6; i++)
			{
				ele[i] = strtol(buf, (char**)&buf, 10);
				if (src == buf || *buf == 0)
					break;
				else
					++buf;
			}

			dest.tm_year = ele[0] - 1900;
			dest.tm_mon = ele[1] - 1;
			dest.tm_mday = ele[2];
			dest.tm_hour = ele[3];
			dest.tm_min = ele[4];
			dest.tm_sec = ele[5];

			std::mktime(&dest);
		}

	protected:
		//type_to_str�ľ���ʵ�ֺ������ػ�����������������ͣ�������ʾʧ�ܡ�ȷʵ��Ҫ�ģ����ֶ�������Ӧ���͵��ػ�ʵ�֡�
		template <typename T>
		char_array_type _to_str(const T& src)
		{
			static_assert(false, "no implementation for type T by default, add your own specialization for T first.");
		}

		//_type_to_str��int�ػ��汾��ת��int����Ϊ�ַ���
		template<> char_array_type _to_str(const int& src)
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "%d", src);
			return str;
		}

		template<> char_array_type _to_str(const unsigned& src)
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "%u", src);
			return str;
		}

		template<> char_array_type _to_str(const long& src)
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "%ld", src);
			return str;
		}

		template<> char_array_type _to_str(const unsigned long& src)
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "%lu", src);
			return str;
		}

		template<> char_array_type _to_str(const long long& src)
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "%lld", src);
			return str;
		}

		template<> char_array_type _to_str(const unsigned long long& src)
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "%llu", src);
			return str;
		}

		template<> char_array_type _to_str(const double& src)
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "%lf", src);
			return str;
		}

		template<> char_array_type _to_str(const float& src)
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "%f", src);
			return str;
		}

		template<> char_array_type _to_str(const char& src)
		{
			char_array_type str;

			if(quote_when_characters_to_str_ != '\0')
				sprintf_s((char*)&str, sizeof(str), "%c%c%c", quote_when_characters_to_str_,src, quote_when_characters_to_str_);
			else
				sprintf_s((char*)&str, sizeof(str), "%c", src);

			return str;
		}

		template<> char_array_type _to_str(const bool& src)
		{
			char_array_type str;
			sprintf_s((char*)&str, sizeof(str), "%d", src);
			return str;
		}

		template<> char_array_type _to_str(const tm& src)
		{
			char_array_type str;

			if (quote_when_characters_to_str_ != '\0')
				sprintf_s((char*)&str, sizeof(str), "%c%04d-%02d-%02d %02d:%02d:%02d%c",
					quote_when_characters_to_str_, src.tm_year + 1900, src.tm_mon + 1, src.tm_mday, src.tm_hour, src.tm_min, src.tm_sec, quote_when_characters_to_str_);
			else
				sprintf_s((char*)&str, sizeof(str), "%04d-%02d-%02d %02d:%02d:%02d",
					src.tm_year + 1900, src.tm_mon + 1, src.tm_mday, src.tm_hour, src.tm_min, src.tm_sec);

			return str;
		}

	protected:
		char quote_when_characters_to_str_ = '\0';
	};
}


//example:
// int main()
// {
// 	based::str_type_convert conv;
// 	printf("%s\n", conv.pointer_of_str(conv.to_str(1.23)));
// 	printf("%s\n", conv.pointer_of_str(conv.to_str("1.23")));
// 	printf("%s\n", conv.pointer_of_str(conv.to_str(1)));
// 	printf("%s\n", conv.pointer_of_str(conv.to_str((long long)1)));
// 
// 	time_t now = time(NULL);
// 	printf("%s\n", conv.pointer_of_str(conv.to_str(*localtime(&now))));
// 
// 	int a1;
// 	conv.to_type(a1, "123");
// 	std::cout << a1 << std::endl;
// 
// 	double a2;
// 	conv.to_type(a2, "");
// 	std::cout << a2 << std::endl;
// 
// 	std::tm a3;
// 	conv.to_type(a3, "2015-08-18 15:29:00");
// 	std::cout << a3.tm_year + 1900 << "\t" << a3.tm_hour << std::endl;
// 
// 	return 0;
// }