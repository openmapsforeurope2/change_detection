// APP
#include <app/calcul/ApplyChangesOp.h>
#include <app/params/ThemeParameters.h>

// BOOST
#include <boost/progress.hpp>

// EPG
#include <epg/Context.h>
#include <epg/params/EpgParameters.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/sql/DataBaseManager.h>
#include <epg/tools/StringTools.h>
#include <epg/tools/TimeTools.h>
#include <epg/tools/FilterTools.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        ApplyChangesOp::ApplyChangesOp(
            std::string const& featureName,
            std::string const& countryCode,
            detail::SCHEMA refSchema,
			detail::SCHEMA upSchema,
            bool verbose
        ) : 
            _featureName(featureName),
            _countryCode(countryCode),
            _verbose(verbose)
        {
            _init(refSchema, upSchema);
        }

        ///
        ///
        ///
        ApplyChangesOp::~ApplyChangesOp()
        {
        }

        ///
        ///
        ///
        void ApplyChangesOp::Compute(
			std::string const& featureName,
            std::string const& countryCode,
            detail::SCHEMA refSchema,
			detail::SCHEMA upSchema,
            bool verbose
		) {
            ApplyChangesOp op(featureName, countryCode, refSchema, upSchema, verbose);
            op._compute();
        }

        ///
        ///
        ///
        void ApplyChangesOp::_init(
            detail::SCHEMA refSchema,
			detail::SCHEMA upSchema
        )
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();
            
            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string refTableName = _featureName + detail::getSuffix(refSchema);
            std::string upTableName = _featureName + detail::getSuffix(upSchema);
            std::string cdTableName = _featureName + themeParameters->getValue(TABLE_CD_SUFFIX).toString();

            //--
            _fsRef = context->getDataBaseManager().getFeatureStore(refTableName, idName, geomName);
            _fsUp = context->getDataBaseManager().getFeatureStore(upTableName, idName, geomName);
            _fsCd = context->getDataBaseManager().getFeatureStore(cdTableName, idName, geomName);

            //--
            _logger->log(epg::log::INFO, "[END] initialization: " + epg::tools::TimeTools::getTime());
        };

        ///
        ///
        ///
        void ApplyChangesOp::_compute() const {
            _delete();
            _update();
            _add();
        }

         ///
        ///
        ///
        void ApplyChangesOp::_add() const {
            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();

            //app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();

            ign::feature::FeatureFilter filterCd(idRefName+" IS NULL");
            epg::tools::FilterTools::addAndConditions(filterCd, countryCodeName+"='"+_countryCode+"'");

            int numCDFeatures = epg::sql::tools::numFeatures(*_fsCd, filterCd);
            boost::progress_display display(numCDFeatures, std::cout, "[ apply changes [add] % complete ]\n");

            ign::feature::FeatureIteratorPtr itCd = _fsCd->getFeatures(filterCd);
            while (itCd->hasNext())
            {
                ++display;
                
                ign::feature::Feature const& fCd = itCd->next();
                std::string const& idUp = fCd.getAttribute(idUpName).toString();

                ign::feature::Feature fUp;
                _fsUp->getFeatureById(idUp, fUp);

                _fsRef->createFeature(fUp);
            }
        }

        ///
        ///
        ///
        void ApplyChangesOp::_update() const {
            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();

            //app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();

            ign::feature::FeatureFilter filterCd(idRefName+" IS NOT NULL AND "+idUpName+" IS NOT NULL");
            epg::tools::FilterTools::addAndConditions(filterCd, countryCodeName+"='"+_countryCode+"'");

            int numCDFeatures = epg::sql::tools::numFeatures(*_fsCd, filterCd);
            boost::progress_display display(numCDFeatures, std::cout, "[ apply changes [update] % complete ]\n");

            ign::feature::FeatureIteratorPtr itCd = _fsCd->getFeatures(filterCd);
            while (itCd->hasNext())
            {
                ++display;
                
                ign::feature::Feature const& fCd = itCd->next();
                std::string const& idRef = fCd.getAttribute(idRefName).toString();
                std::string const& idUp = fCd.getAttribute(idUpName).toString();

                ign::feature::Feature fUp;
                _fsUp->getFeatureById(idUp, fUp);

                fUp.setId(idRef);

                _fsRef->modifyFeature(fUp);
            }
        }

        ///
        ///
        ///
        void ApplyChangesOp::_delete() const {
            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();

            //app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();

            ign::feature::FeatureFilter filterCd(idUpName+" IS NULL");
            epg::tools::FilterTools::addAndConditions(filterCd, countryCodeName+"='"+_countryCode+"'");

            int numCDFeatures = epg::sql::tools::numFeatures(*_fsCd, filterCd);
            boost::progress_display display(numCDFeatures, std::cout, "[ apply changes [delete] % complete ]\n");

            ign::feature::FeatureIteratorPtr itCd = _fsCd->getFeatures(filterCd);
            while (itCd->hasNext())
            {
                ++display;
                
                ign::feature::Feature const& fCd = itCd->next();
                std::string const& idRef = fCd.getAttribute(idRefName).toString();

                _fsRef->deleteFeature(idRef);
            }
        }

    }
}