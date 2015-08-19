#pragma once

#include "stdafx.h"


/*��Щ��������ͷ�ļ����ɽ���Щͷ�ļ��ƶ���stdafx.h�У��Լӿ�����ٶȡ�--------------------------------------
#include "../../../../sharedproject/cryptopp562/randpool.h"
#include "../../../../sharedproject/cryptopp562/osrng.h"
#include "../../../../sharedproject/cryptopp562/rsa.h"
#include "../../../../sharedproject/cryptopp562/aes.h"
#include "../../../../sharedproject/cryptopp562/hex.h"
#include "../../../../sharedproject/cryptopp562/files.h"
#include "../../../../sharedproject/cryptopp562/filters.h"
#include "../../../../sharedproject/cryptopp562/modes.h"
#pragma comment(lib, "../../../../sharedproject/cryptopp562/Win32/Output/Debug/cryptlib.lib")
--------------------------------------------------------------------------------------------------------------*/



namespace encryptor
{
	using namespace CryptoPP;

	class rsa_encryptor
	{
	public:
		rsa_encryptor(int key_bits = 1024)							//�������������key_length��Ϊģ�����ȣ����������˽Կ
		{
			invertable_rsa_.GenerateRandomWithKeySize(rand_pool_, key_bits); 
			pub = RSAES_OAEP_SHA_Encryptor(RSA::PublicKey(invertable_rsa_));
			priv = RSAES_OAEP_SHA_Decryptor(RSA::PrivateKey(invertable_rsa_));
		}

		rsa_encryptor(const std::string& str_pub)						//��֪��Կ�ַ��������������
		{
			assert(!str_pub.empty());
			pub = RSAES_OAEP_SHA_Encryptor(StringSource(str_pub, true, new HexDecoder()));
		}

		void get_key(std::string& str_pub, std::string& str_priv)		//��ȡ��˽Կ�ַ���
		{
			HexEncoder pub_encoder(new StringSink(str_pub));
			pub.DEREncode(pub_encoder);
			pub_encoder.MessageEnd();

			HexEncoder priv_encoder(new StringSink(str_priv));
			priv.DEREncode(priv_encoder);
			priv_encoder.MessageEnd();
		}

		std::string encrypt(const std::string &plain_text)				//��Կ����
		{
			std::string cipher_text;
			int max_msg_length = pub.FixedMaxPlaintextLength();

			for (int i = plain_text.size(), j = 0; i > 0; i -= max_msg_length, j += max_msg_length)
			{
				std::string part_plain_text = plain_text.substr(j, max_msg_length);
				std::string part_cipher_text;
				StringSource(part_plain_text, true, new PK_EncryptorFilter(rand_pool_, pub, new HexEncoder(new StringSink(part_cipher_text))));
				cipher_text += part_cipher_text;
			}

			return cipher_text;
		}

		std::string decrypt(const std::string &cipher_text)				//˽Կ����
		{
			std::string plain_text;
			int cipher_text_length = priv.FixedCiphertextLength() * 2;

			for (int i = cipher_text.size(), j = 0; i > 0; i -= cipher_text_length, j += cipher_text_length)
			{
				std::string part_cipher_text = cipher_text.substr(j, cipher_text_length);
				std::string part_plain_text;
				StringSource(part_cipher_text, true, new HexDecoder(new PK_DecryptorFilter(rand_pool_, priv, new StringSink(part_plain_text))));
				plain_text += part_plain_text;
			}

			return plain_text;
		}

	private:
		rsa_encryptor(const rsa_encryptor&) = delete;
		rsa_encryptor& operator=(const rsa_encryptor&) = delete;
		
	public:
		RSAES_OAEP_SHA_Encryptor pub;
		RSAES_OAEP_SHA_Decryptor priv;

	private:
		AutoSeededRandomPool rand_pool_;
		InvertibleRSAFunction invertable_rsa_;
	};
	

	class aes_encryptor
	{
		//���include���ļ�ǰ����NEED_HEX_STR_FOMART����ʾ���ܽ��Ϊʮ�������ַ����ĸ�ʽ�����������Ϊʮ�������ַ����ĸ�ʽ���˸�ʽ�����ݲ��������룬��������ԭ����2����
		#if defined(NEED_HEX_STR_FOMART)		
		#define HEX_ENCODE new HexEncoder
		#define HEX_DECODE new HexDecoder
		#else
		#define HEX_ENCODE
		#define HEX_DECODE
		#endif

	public:
		aes_encryptor(int key_length = AES::DEFAULT_KEYLENGTH)			//������������������key��iv
		{
			AutoSeededRandomPool rand_pool_;

			key_length_ = key_length;
			key_ = new byte[key_length_];
			rand_pool_.GenerateBlock(key_, key_length_);

			iv_ = new byte[AES::BLOCKSIZE];
			rand_pool_.GenerateBlock(iv_, AES::BLOCKSIZE);
		}

		aes_encryptor(byte* key, byte* iv, int key_length = AES::DEFAULT_KEYLENGTH, int iv_length = AES::BLOCKSIZE)	//��֪key��iv�����������
		{
			key_length_ = key_length;
			key_ = new byte[key_length_];
			iv_ = new byte[iv_length];
			memcpy(key_, key, key_length);
			memcpy(iv_,iv,iv_length);
		}

		~aes_encryptor()
		{
			delete key_;
			delete iv_;
		}

		std::string encrypt(const std::string& plain_text)			//����
		{
			std::string cipher_text;
			StringSource(plain_text, true, 
				new StreamTransformationFilter(CBC_Mode<AES>::Encryption(key_, key_length_, iv_), HEX_ENCODE(new StringSink(cipher_text))));
		
			return cipher_text;
		}
		
		std::string decrypt(const std::string& cipher_text)			//����
		{
			std::string plain_text;
			StringSource(cipher_text, true, 
				HEX_DECODE(new StreamTransformationFilter(CBC_Mode<AES>::Decryption(key_, key_length_, iv_), new StringSink(plain_text))));
		
			return plain_text;
		}
		
		void encrypt_file(const std::string& in_file_name, const std::string& out_file_name)		//�����ļ�
		{
			FileSource(in_file_name.c_str(), true, 
				new StreamTransformationFilter(CBC_Mode<AES>::Encryption(key_, key_length_, iv_), new FileSink(out_file_name.c_str())));
		}
		
		void decrypt_file(const std::string& in_file_name, const std::string& out_file_name)		//�����ļ�
		{
			FileSource(in_file_name.c_str(), true,
				new StreamTransformationFilter(CBC_Mode<AES>::Decryption(key_, key_length_, iv_), new FileSink(out_file_name.c_str())));
		}

	private:
		aes_encryptor(const aes_encryptor&) = delete;
		aes_encryptor& operator=(const aes_encryptor&) = delete;

	public:
		byte * key_ = nullptr;
		byte * iv_ = nullptr;
		int key_length_ = 0;
	};
};




// ʹ�÷�����
// int main(int argc, char *argv[])
// {
// 
// 	char buf[10]{};
// 	buf[0] = 1;
// 	buf[3] = 1;
// 	buf[5] = 1;
// 
// // 	std::string plain_text = "PLC�ͻ��ˣ����ɼ�����150���ڵ��״̬���͵�WCS�������ˣ�Ȼ����������ڴ��ж�150���ڵ���ɵ���������������������е�ÿ���ڵ㣬���ڴ��а���һ���Ĺ��򣬾��߳��Ƿ�Ҫ���ƸĽڵ㡢�������ơ������ڴ���ٶȷǳ��죬�����Ļ������ⲻ��"
// // 		"�ٿ�������Ĳ��֣���Ȼ��2000�����䣬��ʵ�����й����У�ÿ�������ƽڵ�֮��Ķ�������״̬��һ�µġ��ص�������150���ڵ��ϵ����䣬��״̬���ܻ��ܽڵ���ƶ��ı䡣����һ�������ص���������ĸ������ٵ�150�����ҡ�"
// // 		"���⣬����ڱ�����";
// 
// 	std::string plain_text(buf,10);
// 
// 	auto p = plain_text.data();
// 	/*rsa----------------------------------------------------------------------------------
// 	encryptor::rsa_encryptor rsa;
// 	
// 	std::string cipher_text = rsa.encrypt(plain_text);
// 	std::cout << cipher_text << std::endl;
// 
// 	plain_text = rsa.decrypt(cipher_text);
// 	std::cout << plain_text << std::endl;
// 	------------------------------------------------------------------------------rsa*/
// 
// 	///*aes---------------------------------------------------------------------------------
// 	encryptor::aes_encryptor aes;
// 	std::string cipher_text = aes.encrypt(plain_text);
// 	std::cout << cipher_text << std::endl;
// 	
// 	plain_text = "";
// 	plain_text = aes.decrypt(cipher_text);
// 	p = plain_text.data();
// 	std::cout << plain_text << std::endl;
// 	//---------------------------------------------------------------------------------aes*/
// 
// 	return 0;
// }