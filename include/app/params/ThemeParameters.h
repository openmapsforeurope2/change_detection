#ifndef _APP_PARAMS_THEMEPARAMETERS_H_
#define _APP_PARAMS_THEMEPARAMETERS_H_

//STL
#include <string>

//EPG
#include <epg/params/ParametersT.h>
#include <epg/SingletonT.h>



	enum HY_PARAMETERS{
		DB_CONF_FILE,
		COUNTRY_CODE_W,
		WORKING_SCHEMA,
		LANDMASK_TABLE,
		TABLE,
		TABLE_REF_SUFFIX,
		TABLE_UP_SUFFIX,
		TABLE_CD_SUFFIX,
		ID_REF,
		ID_UP,
		GEOM_MATCH,
		ATTR_MATCH,

		CD_DIST_THRESHOLD
	};

namespace app{
namespace params{

	class ThemeParameters : public epg::params::ParametersT< HY_PARAMETERS >
	{
		typedef  epg::params::ParametersT< HY_PARAMETERS > Base;

		public:

			/// \brief
			ThemeParameters();

			/// \brief
			~ThemeParameters();

			/// \brief
			virtual std::string getClassName()const;

	};

	typedef epg::Singleton< ThemeParameters >   ThemeParametersS;

}
}

#endif