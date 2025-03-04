#ifndef _APP_CALCUL_APPLYCHANGESOP_H_
#define _APP_CALCUL_APPLYCHANGESOP_H_

//APP
#include <app/detail/schema.h>


//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


namespace app{
namespace calcul{

	class ApplyChangesOp {

	public:

	
        /// @brief 
        ApplyChangesOp(
            std::string const& featureName,
            std::string const& countryCode,
            detail::SCHEMA refTheme,
			detail::SCHEMA upTheme,
            bool verbose = false
        );

        /// @brief 
        ~ApplyChangesOp();


		/// \brief
		static void Compute(
			std::string const& featureName,
            std::string const& countryCode,
            detail::SCHEMA refTheme,
			detail::SCHEMA upTheme,
            bool verbose = false
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsRef;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsUp;
		//--
		ign::feature::sql::FeatureStorePostgis*                  _fsCd;
		//--
		epg::log::EpgLogger*                                     _logger;
		//--
		epg::log::ShapeLogger*                                   _shapeLogger;
		//--
		std::string                                              _featureName;
		//--
		std::string                                              _countryCode;
		//--
		bool                                                     _verbose;

	private:

		//--
		void _init(
			detail::SCHEMA refTheme,
			detail::SCHEMA upTheme
		);

		//--
		void _compute() const;

		//--
		void _delete() const;

		//--
		void _update() const;

		//--
		void _add() const;

    };

}
}

#endif