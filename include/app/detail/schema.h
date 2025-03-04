#ifndef _APP_CALCUL_DETAIL_SCHEMA_H_
#define _APP_CALCUL_DETAIL_SCHEMA_H_

#include <string>

namespace app{
namespace detail{

    //--
    enum SCHEMA
	{
		PROD  = 1,
		UP    = PROD  << 1,
		REF   = UP    << 1,
		WORK  = REF   << 1
    };

    //--
    std::string getSuffix(SCHEMA schema);
}
}

#endif