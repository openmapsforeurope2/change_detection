#ifndef _APP_PARAMS_THEMEPARAMETERS_H_
#define _APP_PARAMS_THEMEPARAMETERS_H_

//STL
#include <string>

//EPG
#include <epg/params/ParametersT.h>
#include <epg/SingletonT.h>



	enum HY_PARAMETERS{
		DB_CONF_FILE,
		LANDMASK_TABLE,
		THEME_W,
		REF_SCHEMA,
		UP_SCHEMA,
		AU_SCHEMA,
		IB_SCHEMA,
		HY_SCHEMA,
		TN_SCHEMA,
		WK_SCHEMA,
		TABLE_REF_SUFFIX,
		TABLE_UP_SUFFIX,
		TABLE_CD_SUFFIX,
		TABLE_WK_SUFFIX,
		ID_REF,
		ID_UP,
		GEOM_MATCH,
		ATTR_MATCH,

		CD_DIST_THRESHOLD,
		CD_EGAL_DIST_THRESHOLD,
		CD_BBOX_MAX_SIDE_LENGTH,
		CD_IGNORED_FIELDS_SEPARATOR,
		CD_IGNORED_COMMON_FIELDS,
		CD_IGNORED_TN_FIELDS,
		CD_IGNORED_HY_FIELDS,
		CD_IGNORED_AU_FIELDS,
		CD_IGNORED_IB_FIELDS
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