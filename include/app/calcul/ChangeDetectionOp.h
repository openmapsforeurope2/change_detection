#ifndef _APP_CALCUL_CHANGEDETECTIONROP_H_
#define _APP_CALCUL_CHANGEDETECTIONROP_H_

//BOOST
#include <boost/tuple/tuple.hpp>

//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>

//EPG
#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>


namespace app{
namespace calcul{

	class ChangeDetectionOp {

	public:

        /// @brief Constructeur
        /// @param featureName Nom de la classe d'objet à traiter
        /// @param countryCode Code pays simple
        /// @param verbose Mode verbeux
        ChangeDetectionOp(
            std::string const& feature,
            std::string const& countryCode,
            bool verbose
        );

        /// @brief Destructeur
        ~ChangeDetectionOp();


		/// @brief Lance le calcul du différentiel entra la table de référence et la table mise à jour.
		/// Le calcul est réalisé selon un dallage. La taille des dalles est paramétrable.
		/// @param featureName Nom de la classe d'objet à traiter
        /// @param countryCode Code pays simple
        /// @param verbose Mode verbeux
		static void Compute(
			std::string const& feature,
            std::string const& countryCode,
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
		std::string                                              _featureName;
		//--
		std::string                                              _countryCode;
		//--
		bool                                                     _verbose;
		//--
		std::string                                              _separator;
		//
		std::set<std::string>                                    _sIgnoredFields;
		//
		std::set<std::string>                                    _sMatchingFields;

	private:

		//--
		void _init();

		//--
		void _compute() const;

		//--
		void _removeDuplicates(std::string const& uniqueKey, bool whereAttMatchIsNull = false) const;

		//--
		void _computeOrientedChangeDetection(
            ign::feature::sql::FeatureStorePostgis* fsSource, 
            std::string const & idSourceName,
            ign::feature::sql::FeatureStorePostgis* fsTarget, 
            std::string const & idTargetName,
			bool withAttrCompare = true
		) const;

		//--
		std::pair<bool, bool> _attributsEqual(
			ign::feature::Feature const& feat1,
			ign::feature::Feature const& feat2
		) const;

		//--
		void _computeChangeDetection() const;

		//--
		void _explodeBbox(
            ign::geometry::Envelope const& bbox,
            std::vector<ign::geometry::Envelope> & vBbox
        ) const;

		//--
		std::vector<ign::geometry::Envelope> _getBboxes() const;

		//--
		void _updateCDTAble(
            std::vector<boost::tuple<std::string, std::string, bool, bool, bool>> const& vtModified,
            std::set<std::string> const& sDeleted,
            std::set<std::string> const& sCreated
        ) const;

    };

}
}

#endif