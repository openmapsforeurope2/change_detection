#ifndef _APP_CALCUL_APPLYCHANGESOP_H_
#define _APP_CALCUL_APPLYCHANGESOP_H_

//APP
#include <app/detail/schema.h>


//SOCLE
#include <ign/feature/sql/FeatureStorePostgis.h>

//EPG
#include <epg/log/EpgLogger.h>


namespace app{
namespace calcul{

	/// @brief Classe à utiliser pour 'jouer' un différentiel
	class ApplyChangesOp {

	public:
	
        /// @brief Constructeur
        /// @param featureName Nom de la classe d'objet à traiter
        /// @param countryCode Code pays simple
        /// @param refSchema Schéma de la référence
        /// @param upSchema Schéma de la mise à jour
        /// @param verbose Mode verbeux
        ApplyChangesOp(
            std::string const& featureName,
            std::string const& countryCode,
            detail::SCHEMA refSchema,
			detail::SCHEMA upSchema,
            bool verbose = false
        );

        /// @brief Destructeur
        ~ApplyChangesOp();

		/// @brief Lance l'application du différentiel sur la table de référence
        /// @param featureName Nom de la classe d'objet à traiter
        /// @param countryCode Code pays simple
        /// @param refSchema Schéma de la référence
        /// @param upSchema Schéma de la mise à jour
        /// @param verbose Mode verbeux
		static void Compute(
			std::string const& featureName,
            std::string const& countryCode,
            detail::SCHEMA refSchema,
			detail::SCHEMA upSchema,
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
		std::string                                              _featureName;
		//--
		std::string                                              _countryCode;
		//--
		bool                                                     _verbose;

	private:

		//--
		void _init(
			detail::SCHEMA refSchema,
			detail::SCHEMA upSchema
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