//����֧���ࡣ
//����ʱ������������ʱ�ع�����commit�ύ����rollback�ع����� ���������֮ǰ��ִ����commit����rollback��������ʱ����ִ�лع�������

#pragma once

#include "session.h"

namespace sqler
{
	class transaction
	{
	public:
		transaction(session& s):
			session_(s),
			need_rollback(true)
		{
			session_.query("BEGIN;");
		}

		~transaction()
		{
			if(need_rollback)
				rollback();
		}

		void commit()
		{
			session_.query("COMMIT;");
			need_rollback = false;
		}

		void rollback()
		{
			session_.query("ROLLBACK");
			need_rollback = false;
		}

	protected:
		session& session_;
		bool need_rollback;
	};
}