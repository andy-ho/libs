#pragma once

#if defined(_MSC_VER) && _MSC_VER < 1800
#error "Compiler need to support c++11, please use vs2013 or above, vs2015 e.g."
#endif

#include "../based/headers_all.h"
#include "session_pool.h"

namespace sqler
{
	typedef based::except sql_error;

	#define THROW_SQL_EXCEPTION THROW_SQL_EXCEPTION0
	#define THROW_SQL_EXCEPTION0() throw(sqler::sql_error((int)get_error_no(),get_error_msg(),__FILE__,__LINE__))
	#define THROW_SQL_EXCEPTION2(ERROR_NO,ERROR_STR) throw(sqler::sql_error(ERROR_NO,ERROR_STR,__FILE__,__LINE__))

	class session
	{
	public:
		struct mysql_lib_init
		{
			mysql_lib_init()
			{
				if (init_state())
					return;

				ENSURE(mysql_library_init(0,NULL,NULL) == 0).tips("mysql lib init fail").warn(-1);

				init_state() = true;
			}

// 			~mysql_lib_init()
// 			{
// 				mysql_library_end();
// 			}

			bool& init_state()
			{
				static bool init_ok = false;
				return init_ok;
			}
		};

		mysql_lib_init& lib_init_dummy()
		{
			static mysql_lib_init dummy;
			return dummy;
		}
		
		session()
		{
			//mysql_init()�����ڵ�һ�γ�ʼ����ʱ��,�����̰߳�ȫ��,����֮�����̰߳�ȫ�ġ�c++11��׼�涨���ֲ���̬�����ʼ�����̰߳�ȫ�ģ��˴�ȷ��mysql_init��ȫ����һ�Ρ�
			//mysql_init��ȫ����һ�κ󣬺�����mysql_init���ö����̰߳�ȫ�ġ�
			//����mysql_initֱ�Ӵ�NULL������lib_init_dummy().init_state()��ԭ��1����ʱ!init_stateһ����false 2����ֹrelease�¶�lib_init_dummy�ĵ��ñ��Ż���
			my_sql_ = mysql_init((MYSQL*)(!lib_init_dummy().init_state()));
			type_conv_.set_quote_when_characters_to_str('\'');
		}

		session(MYSQL* my_sql)
			:my_sql_(my_sql),
			not_destroy_(lib_init_dummy().init_state())
		{
			//����not_destroy_ֱ�ӳ�ʼ��Ϊtrue������lib_init_dummy().init_state()��ԭ��1����ʱinit_stateһ����true 2����ֹrelease�¶�lib_init_dummy�ĵ��ñ��Ż���
			mysql_init(my_sql_);
			type_conv_.set_quote_when_characters_to_str('\'');
		}

		//�����ӳ��й��죬���ӳ����޿������ӣ���ȴ�timeout_second��
		session(session_pool* sp, unsigned timeout_second = (unsigned)-1)
		{
			index_in_pool_ = lib_init_dummy().init_state();			//��index_in_pool_��ֵû��ʵ�����ã�ֻ�Ƿ�ֹrelease�¶�lib_init_dummy�ĵ��ñ��Ż���
			my_sql_ = (sp_=sp)->get(&index_in_pool_, timeout_second);
			type_conv_.set_quote_when_characters_to_str('\'');
		}

		//�����ӳ��й��죬�򻹸����ӳأ�����ر�����
		~session()
		{
			free_all_result();

			if (sp_ && index_in_pool_ != -1)
				sp_->release(index_in_pool_);
			else if (!not_destroy_)
				close();
		}

		//ȡ���һ��sql query�Ĵ�������
		const char* get_error_msg()
		{
			return mysql_error(my_sql_);
		}

		//ȡ���һ��sql query�Ĵ�����
		unsigned get_error_no()
		{
			return mysql_errno(my_sql_);
		}

		//ȡ�е���Ŀ
		unsigned get_field_count()
		{
			ENSURE(my_sql_ != NULL).tips("be sure successfully connect to database first").warn(-1);
			return mysql_field_count(my_sql_);
		}

		//ȡ�����ݿ��̨
		MYSQL* get_backend_impl()
		{
			return my_sql_;
		}
		
		//ȡ�ý����
		MYSQL_RES* get_result_set()
		{
			return res_;
		}

		//�������ݿ�
		void open(const char* host, unsigned port, const char* user, const char* passwd, const char* database, 
			unsigned long client_flag = CLIENT_MULTI_STATEMENTS | CLIENT_REMEMBER_OPTIONS)
		{
			ENSURE(mysql_real_connect(my_sql_, host, user, passwd, database, port, NULL, client_flag) != NULL).tips(get_error_msg()).warn(get_error_no());
		}
		
		void set_option(const int option_type, const char* option_value_ptr)
		{
			mysql_options(my_sql_, (mysql_option)option_type, option_value_ptr);
		}

		//����sql��䡣
		//sql_str��Ҫִ�е�sql��䣬��Ҫ������ʱ��ʹ��'?'��ռλ�������Դ��ݶ���sql��䣬��';'�ָ���  
		//args...����ʵ�Ĳ�����������sql_str�е�'?'һ����������׳��쳣�� �����ڲ��Ὣ'?'�ַ��滻Ϊargs...����ʵֵ����ִ�в�ѯ��
		//demo:
		//int my_id = 1;
		//s.query("select * from table_a where id=? and name=?",my_id,"my_name");
		template<typename... Args>
		unsigned long long query(const char* sql_str, const Args&... args)
		{
			free_all_result();

			std::string formated_sql = sql_str;

			formated_sql.reserve(256);
			format_sql_str(formated_sql, 0, args...);

			ENSURE(mysql_real_query(my_sql_, formated_sql.c_str(), formated_sql.size()) == 0)(formated_sql).tips(get_error_msg()).warn(get_error_no());

			return next_result();
		}
		
		template<typename... Args>
		unsigned long long query(const std::string& sql_str, const Args&... args)
		{
			return query(sql_str.c_str(), args...);
		}

		//�洢������������ص�ǰ�������������
		//����ֵ����������������������������������û������Ϊ0��������������ݵ�Ϊ���ݵ�������ȡ��������̳�����׳��쳣��
		unsigned long long next_result()
		{
			free_former_result();
			
			if (!has_next_result_)
				return 0;

			res_ = mysql_store_result(my_sql_);

			if (mysql_next_result(my_sql_) != 0)								//Ԥ����һ��������Ƿ����
				has_next_result_ = false;

			if (res_ == NULL)
			{
				unsigned error_no = get_error_no();
				ENSURE(error_no == 0).tips(get_error_msg()).warn(error_no);	//֮ǰ��sql���������������store result ��������

				return 0;														//֮ǰ��sql��䲻���������
			}

			return mysql_num_rows(res_);										//���ؽ��������0��ʾ����������������������û�н��
		}


		void free_former_result()
		{
			if (res_ != NULL)
			{
				mysql_free_result(res_);
				res_ = NULL;
			}
		}

		void free_all_result()
		{
			free_former_result();
			
			while (has_next_result_ || mysql_next_result(my_sql_) == 0)
			{
				has_next_result_ = false;
				res_ = mysql_store_result(my_sql_);
				free_former_result();
			} ;

			has_next_result_ = true;						//��λhas_next_result_�����������query���н����
		}

		//ȡ�������һ�е�args��
		//����ֵ��true�ɹ�ȡ����falseû�н����ȡ��
		template<typename... Args>
		bool fetch_row(Args&... args)
		{
			ENSURE(res_ != NULL).tips("be sure to successfully store result first").warn(-1);

			MYSQL_ROW row = mysql_fetch_row(res_);
			if (row == NULL)								//res_��Ľ������ȡ��
				return false;

			unsigned long* field_length = mysql_fetch_lengths(res_);
			ENSURE(field_length != NULL).tips(get_error_msg()).warn(get_error_no());

			_fetch_row(row, 0, field_length, args...);

			return true;
		}


	private:
		void close()
		{
			if (my_sql_ != NULL)
			{
				free_all_result();
				mysql_close(my_sql_);
				mysql_library_end();
			}
		}

		//fetch_row�ľ���ʵ�ֺ�����
		template<typename Arg>
		void _fetch_row(MYSQL_ROW& row, int field_index, unsigned long* field_len_array, Arg& dest)
		{
			long field_len = field_len_array[field_index];
			const char* src;
			field_len == 0 ? src = NULL : src = row[field_index];

			type_conv_.to_type(dest, src, field_len);
		}


		//fetch_row�ľ���ʵ�ֺ����������е���ֵ���ת����dests��
		template<typename Arg, typename... Args>
		void _fetch_row(MYSQL_ROW& row, int field_index, unsigned long* field_len_array, Arg& dest, Args&... dests)
		{
			long field_len = field_len_array[field_index];
			const char* src;
			field_len == 0 ? src = NULL : src = row[field_index];
			
			type_conv_.to_type(dest, src, field_len);
			_fetch_row(row, ++field_index, field_len_array, dests...);
		}


		//��ʽ��sql str��Ϊ���������sql str��׼����
		template<typename int=0>
		void format_sql_str(std::string& sql_str, int last_read_pos){}


		//��ʽ��sql str����'?'�滻Ϊarg��ֵ
		//��Ȼƥ�䵽����������argһ�������һ�����������'?'��arg�����Ƿ�ƥ�䡣 ����arg��������'?'���������ƥ��
		template<typename Arg>
		void format_sql_str(std::string& sql_str, int last_read_pos, const Arg& arg)
		{
			size_t pos = sql_str.find("?", last_read_pos);
			ENSURE(pos != sql_str.npos)(sql_str).tips("placeholders' count in sql str not compatiable with real parameters").warn(-1);

			const char* sql_param = type_conv_.pointer_of_str(type_conv_.to_str(arg));
			sql_str.replace(pos, 1, sql_param);
			pos += strlen(sql_param);

			//���ˣ�arg...ȫ���滻��ɣ��������'?'����'?'��arg...�ĸ�����һ��
			ENSURE(sql_str.find("?", pos) == sql_str.npos)(sql_str).tips("placeholders' count in sql str not compatiable with real parameters").warn(-1);
		}


		//��ʽ��sql str����'?'�滻Ϊarg��ֵ
		//����'?'��arg�����Ƿ�ƥ�䡣 ��Ȼƥ�䵽����������argһ�����ڣ����������'?'���������ƥ��
		//�ݹ��滻args�е�ÿһ��������ֱ�����һ�����������һ������ʱ����ƥ��Ϊformat_sql_str(std::string& sql_str, int last_read_pos, const Arg& arg)��
		template<typename Arg, typename... Args>
		void format_sql_str(std::string& sql_str, int last_read_pos, const Arg& arg, const Args&... args)
		{
			size_t pos = sql_str.find("?", last_read_pos);
			ENSURE(pos != sql_str.npos)(sql_str).tips("placeholders' count in sql str not compatiable with real parameters").warn(-1);

			const char* sql_param = type_conv_.pointer_of_str(type_conv_.to_str(arg));
			sql_str.replace(pos, 1, sql_param);
			pos += strlen(sql_param);				

			format_sql_str(sql_str, pos, args...);
		}

	public:
		based::str_type_convert type_conv_;
		session_pool* sp_;
		int index_in_pool_ = -1;
		bool not_destroy_ = false;
		MYSQL* my_sql_ = NULL;
		MYSQL_RES* res_ = NULL;
		bool has_next_result_ = true;
	};
}

//��ʾ��
// #include "sqler.h"
// int main()
// {
// 	sql::session s;
// 	try{
// 		s.open("192.168.1.222", 3306, "root", "111111", "chuwugui");
// 		s.query("set names gbk;");
// 		s.query("select id,charge_rate,short_msg_template_postman_fetch from system_config where id = ?", 1);
// 		s.store_result();
// 
// 		int id;
// 		std::string charge_rate;
// 		std::string short_msg_template_postman_fetch;
// 		s.fetch_row(id, charge_rate, short_msg_template_postman_fetch);
// 	}
// 	catch (sql::sql_error& er)
// 	{
// 
// 	}
// }