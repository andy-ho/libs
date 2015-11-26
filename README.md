# libs(一组常用的C++组件库)

## based
- `charset_convert.h`   
  一个字符集转换的类;   
- `except.h`   
 一个异常工具库;   
- `os.h`   
 一个跟系统有关的常用功能api的封装;
- `scope_guard.h`   
 这是一个资源守卫类，负责及时、准确的释放不再需要的资源;   
- `str_type_convert.h`   
 一个字符串与其他类型互相转换的类;   
- `string_more.h`
 一个字符串操作的功能增强类;   

## encryptor   
- `encryptor.h`   
 rsa、aes加解密封装;   

## img_recognize   
- `img_recognize.h`   
 验证码识别类;   

## sqler（C++数据库连接工具）   
- `session.h`   
c++数据库访问层，初衷在于提供简洁接口的同时，不失灵活性;   
- `session_pool.h`   
 数据库连接池。为session.h提供支持;   
- `transaction.h`   
 事务支持类;   
- `sqler.h`   
 使用时包含此头文件即可;   
### 代码示例   
```
#include "session.h"
int main()
{
	sqler::session s;
	try {
		s.open("host_ip", 3306, "user", "password", "db_name");

		int id = 10;
		std::string cfg_type = "cfg_back";
		double charge_rate;
		std::string short_msg;
		//查询所有id<10且类型为"cfg_back"的记录，保存到本地变量charge_rate，short_msg中。
		//表中，id为整数型，type为字符串型，charge_rate为浮点型，short_msg为字符串型
		s.query("select charge_rate,short_msg from system_config where id < ? and type = ?", id, cfg_type);
		while (s.fetch_row(charge_rate, short_msg))
		{
			//do something..
			std::cout << charge_rate << short_msg;
		}
	}
	catch (sqler::sql_error& er)
	{
		std::cout << er.what();
	}
}
```
