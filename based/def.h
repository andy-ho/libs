#pragma once

//������������: NAMES_CAT(OBJ,A)�õ�OBJA
#define NAMES_CAT_IMPL(NAME1,NAME2) NAME1##NAME2
#define NAMES_CAT(NAME1, NAME2) NAMES_CAT_IMPL(NAME1,NAME2)

#if defined(_UNICODE) || defined(UNICODE)
	#define tstring std::wstring
#else
	#define tstring std::string
#endif