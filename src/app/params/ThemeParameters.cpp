
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
		_initParameter( LANDMASK_TABLE, "LANDMASK_TABLE");
		_initParameter( THEME_W, "THEME_W");
		_initParameter( REF_SCHEMA, "REF_SCHEMA");
		_initParameter( UP_SCHEMA, "UP_SCHEMA");
		_initParameter( AU_SCHEMA, "AU_SCHEMA");
		_initParameter( IB_SCHEMA, "IB_SCHEMA");
		_initParameter( HY_SCHEMA, "HY_SCHEMA");
		_initParameter( TN_SCHEMA, "TN_SCHEMA");
		_initParameter( WK_SCHEMA, "WK_SCHEMA");
		_initParameter( TABLE_REF_SUFFIX, "TABLE_REF_SUFFIX");
		_initParameter( TABLE_UP_SUFFIX, "TABLE_UP_SUFFIX");
		_initParameter( TABLE_CD_SUFFIX, "TABLE_CD_SUFFIX");
		_initParameter( TABLE_WK_SUFFIX, "TABLE_WK_SUFFIX");
		_initParameter( ID_REF, "ID_REF");
		_initParameter( ID_UP, "ID_UP");
		_initParameter( GEOM_MATCH, "GEOM_MATCH");
		_initParameter( ATTR_MATCH, "ATTR_MATCH");

		_initParameter( CD_DIST_THRESHOLD, "CD_DIST_THRESHOLD");
		_initParameter( CD_EGAL_DIST_THRESHOLD, "CD_EGAL_DIST_THRESHOLD");
		_initParameter( CD_BBOX_MAX_SIDE_LENGTH, "CD_BBOX_MAX_SIDE_LENGTH");
		_initParameter( CD_IGNORED_FIELDS_SEPARATOR, "CD_IGNORED_FIELDS_SEPARATOR");
		_initParameter( CD_IGNORED_COMMON_FIELDS, "CD_IGNORED_COMMON_FIELDS");
		_initParameter( CD_IGNORED_TN_FIELDS, "CD_IGNORED_TN_FIELDS");
		_initParameter( CD_IGNORED_HY_FIELDS, "CD_IGNORED_HY_FIELDS");
		_initParameter( CD_IGNORED_AU_FIELDS, "CD_IGNORED_AU_FIELDS");
		_initParameter( CD_IGNORED_IB_FIELDS, "CD_IGNORED_IB_FIELDS");
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