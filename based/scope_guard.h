//����һ����Դ�����࣬����ʱ��׼ȷ���ͷŲ�����Ҫ����Դ�� �Ӵ���Ҳ���÷�����Դ�ú�ʱ���٣������ǰ���Ҫ�������ں����߼��ı�д��
//�÷��μ����ļ���demo�Լ���except.h�����ṩ��demo��

#pragma once

#if defined(_MSC_VER) && _MSC_VER < 1800
	#error "Compiler need to support c++11, please use vs2013 or above, vs2015 e.g."
#endif

#include "headers_dependency.h"
#include "def.h"

namespace based
{
	class scope_guard
	{
	public:
		scope_guard(const std::function<void()>& on_exit = nullptr)
			:on_exit_(on_exit)
		{}

		scope_guard(scope_guard const&) = delete;
		scope_guard& operator=(scope_guard const&) = delete;

		~scope_guard()
		{
			if (on_exit_)
			{
				on_exit_();
			}
		}

		void cancel()
		{
			on_exit_ = nullptr;
		}

	protected:
		std::function<void()> on_exit_;
	};
}

#define SCOPE_GUARD(callback) based::scope_guard NAMES_CAT(scope_guard_,__LINE__)(callback)



//example:
//#include "scope_guard.h"
//void fun()
//{
//	FILE* file = NULL;
//	bool success = false;
//
//	SCOPE_GUARD([&] {
//		if (file)
//			fclose(file);
//
//		if (!success)
//			std::cout << "fail" << std::endl;
//	});
//
//	file = fopen("c:\\a", "r+");
//	if (file == NULL)
//		return;
//
//	char buf[10];
//	if (fread(buf, 1, 10, file) != 10)
//		return;
//
//	if (fwrite(buf, 1, 10, file) != 10)
//		return;
//
//	success = true;
//	std::cout << "success" << std::endl;
//}
//
//int main()
//{
//	fun();
//  return 0;
//}
