// APP
#include <app/detail/schema.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace detail{

    //--
    std::string getSuffix(SCHEMA schema){
        params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
        switch ( schema )
        {
            case PROD:
                return "";
            case UP:
                return themeParameters->getValue(TABLE_UP_SUFFIX).toString();
            case REF:
                return themeParameters->getValue(TABLE_REF_SUFFIX).toString();
            case WORK:
                return themeParameters->getValue(TABLE_WK_SUFFIX).toString();
        }
        return "";
    }
}
}