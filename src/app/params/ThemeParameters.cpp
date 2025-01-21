
//APP
#include <app/params/ThemeParameters.h>

//SOCLE
#include <ign/Exception.h>


namespace app{
namespace params{


	///
	///
	///
	ThemeParameters::ThemeParameters()
	{
		_initParameter( DB_CONF_FILE, "DB_CONF_FILE");
		_initParameter( COUNTRY_CODE_W, "COUNTRY_CODE_W");
		_initParameter( WORKING_SCHEMA, "WORKING_SCHEMA");
		_initParameter( LANDMASK_TABLE, "LANDMASK_TABLE");
		_initParameter( TABLE, "TABLE");
		_initParameter( TABLE_REF_SUFFIX, "TABLE_REF_SUFFIX");
		_initParameter( TABLE_UP_SUFFIX, "TABLE_UP_SUFFIX");
		_initParameter( TABLE_CD_SUFFIX, "TABLE_CD_SUFFIX");
		_initParameter( ID_REF, "ID_REF");
		_initParameter( ID_UP, "ID_UP");
		_initParameter( GEOM_MATCH, "GEOM_MATCH");
		_initParameter( ATTR_MATCH, "ATTR_MATCH");

		_initParameter( CD_DIST_THRESHOLD, "CD_DIST_THRESHOLD");
	}

	///
	///
	///
	ThemeParameters::~ThemeParameters()
	{
	}

	///
	///
	///
	std::string ThemeParameters::getClassName()const
	{
		return "app::params::ThemeParameters";
	}


}
}