// Copyright 2018 Jacob Chesley.
// See LICENSE.TXT in root of project for license information.

#ifdef CHECK_MEMORY_LEAK
	#define CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>

	#ifdef _DEBUG
		#ifndef DBG_NEW
			#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
			#define new DBG_NEW
		#endif
	#endif  // _DEBUG
#endif