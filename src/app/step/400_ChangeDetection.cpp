#include <app/step/400_ChangeDetection.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
// #include <ome2/utils/CopyTableUtils.h>

//APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/ChangeDetectionOp.h>


namespace app {
namespace step {

	///
	///
	///
	void ChangeDetection::init()
	{
		// addWorkingEntity(AREA_TABLE_INIT);
	}

	///
	///
	///
	void ChangeDetection::onCompute( bool verbose = false )
	{
		app::params::ThemeParameters* themeParameters = app::params::ThemeParametersS::getInstance();
		std::string countryCodeW = themeParameters->getParameter(COUNTRY_CODE_W).getValue().toString();

		app::calcul::ChangeDetectionOp::Compute(countryCodeW, verbose);
	}

}
}
