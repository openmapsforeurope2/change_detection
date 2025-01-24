#ifndef _APP_CALCUL_CHANGEDETECTIONROP_H_
#define _APP_CALCUL_CHANGEDETECTIONROP_H_

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


namespace app{
namespace calcul{

	class ChangeDetectionOp {

	public:

	
        /// @brief 
        /// @param borderCode 
        /// @param verbose 
        ChangeDetectionOp(
            std::string borderCode,
            bool verbose
        );

        /// @brief 
        ~ChangeDetectionOp();


		/// \brief
		static void Compute(
			std::string borderCode, 
			bool verbose
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
		std::string                                              _countryCode;
		//--
		bool                                                     _verbose;
		//--
		std::string                                              _separator;

	private:

		//--
		void _init();

		//--
		void _compute() const;

		//--
		void _computeOrientedChangeDetection(
            ign::feature::sql::FeatureStorePostgis* fsSource, 
            std::string const & idSourceName,
            ign::feature::sql::FeatureStorePostgis* fsTarget, 
            std::string const & idTargetName,
			bool withAttrCompare = true
		) const;

		//--
		bool _attributsEqual(
			ign::feature::Feature const& feat1,
			ign::feature::Feature const& feat2
		) const;

		//--
		void _computeChangeDetection() const;

		//--
		void _updateCDTAble(
            std::vector<std::pair<std::string, std::string>> const& vpModified,
            std::set<std::string> const& sDeleted,
            std::set<std::string> const& sCreated
        ) const;

    };

}
}

#endif