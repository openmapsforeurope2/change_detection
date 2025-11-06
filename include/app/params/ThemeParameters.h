#ifndef _APP_PARAMS_THEMEPARAMETERS_H_
#define _APP_PARAMS_THEMEPARAMETERS_H_

//STL
#include <string>

//EPG
#include <epg/params/ParametersT.h>
#include <epg/SingletonT.h>



	enum TH_PARAMETERS{
		DB_CONF_FILE,
		LANDMASK_TABLE,
		REF_SCHEMA,
		UP_SCHEMA,
		THEME_SCHEMA,
		WK_SCHEMA,
		TABLE_REF_SUFFIX,
		TABLE_UP_SUFFIX,
		TABLE_CD_SUFFIX,
		TABLE_WK_SUFFIX,
		ID_REF,
		ID_UP,
		GEOM_MATCH,
		ATTR_MATCH,
		MATCHING_ATTR_MATCH,

		CD_DIST_THRESHOLD,
		CD_EGAL_DIST_THRESHOLD,
		CD_BBOX_MAX_SIDE_LENGTH,
		CD_FIELDS_SEPARATOR,
		CD_IGNORED_COMMON_FIELDS,
		CD_IGNORED_THEME_FIELDS,
		CD_MATCHING_COMMON_FIELDS,
		CD_MATCHING_THEME_FIELDS
	};

namespace app{
namespace params{

	class ThemeParameters : public epg::params::ParametersT< TH_PARAMETERS >
	{
		typedef  epg::params::ParametersT< TH_PARAMETERS > Base;

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