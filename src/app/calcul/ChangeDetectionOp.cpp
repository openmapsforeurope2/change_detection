// APP
#include <app/calcul/ChangeDetectionOp.h>
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

//SOCLE
#include <ign/geometry/algorithm/OptimizedHausdorffDistanceOp.h>


namespace app
{
    namespace calcul
    {
        ///
        ///
        ///
        ChangeDetectionOp::ChangeDetectionOp(
            std::string borderCode,
            bool verbose
        ) : 
            _countryCode(borderCode),
            _verbose(verbose)
        {
            _init();
        }

        ///
        ///
        ///
        ChangeDetectionOp::~ChangeDetectionOp()
        {

            // _shapeLogger->closeShape("ps_cutting_ls");
        }

        ///
        ///
        ///
        void ChangeDetectionOp::Compute(
			std::string borderCode, 
			bool verbose
		) {
            ChangeDetectionOp op(borderCode, verbose);
            op._compute();
        }

        ///
        ///
        ///
        void ChangeDetectionOp::_init()
        {
            //--
            _logger = epg::log::EpgLoggerS::getInstance();
            _logger->log(epg::log::INFO, "[START] initialization: " + epg::tools::TimeTools::getTime());

            //--
            _shapeLogger = epg::log::ShapeLoggerS::getInstance();
            // _shapeLogger->addShape("ps_cutting_ls", epg::log::ShapeLogger::POLYGON);

            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            
            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const tableName = themeParameters->getValue(TABLE).toString();
            std::string refTableName = tableName + themeParameters->getValue(TABLE_REF_SUFFIX).toString();
            std::string upTableName = tableName + themeParameters->getValue(TABLE_UP_SUFFIX).toString();
            std::string cdTableName = tableName + themeParameters->getValue(TABLE_CD_SUFFIX).toString();
            std::string const geomMatchName = themeParameters->getValue(GEOM_MATCH).toString();
            std::string const attrMatchName = themeParameters->getValue(ATTR_MATCH).toString();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();
			
			if ( !context->getDataBaseManager().tableExists(cdTableName) ) {
				std::ostringstream ss;
				ss << "CREATE TABLE " << cdTableName << "("
                    << geomName << " geometry,"
					<< idName << " uuid DEFAULT gen_random_uuid(),"
                    << idRefName << " character varying(255),"
                    << idUpName << " character varying(255), "
					<< geomMatchName << " boolean,"
					<< attrMatchName << " boolean,"
					<< countryCodeName << " character varying(5)"
					<< ");"
                    << "CREATE INDEX " << cdTableName +"_"+idRefName+"_idx ON " << cdTableName << " USING btree ("<< idRefName <<");"
                    << "CREATE INDEX " << cdTableName +"_"+idUpName+"_idx ON " << cdTableName << " USING btree ("<< idUpName <<");"
                    << "CREATE INDEX " << cdTableName +"_"+countryCodeName+"_idx ON " << cdTableName << " USING btree ("<< countryCodeName <<");";

				context->getDataBaseManager().getConnection()->update(ss.str());
			} else {
                std::ostringstream ss;
                ss << "DELETE FROM " << cdTableName << " WHERE " << countryCodeName << " = '" << _countryCode << "';";
                context->getDataBaseManager().getConnection()->update(ss.str());
            }

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
        void ChangeDetectionOp::_compute() const {
            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();

            _computeOrientedChangeDetection(_fsRef, idRefName, _fsUp, idUpName);
            _computeOrientedChangeDetection(_fsUp, idUpName, _fsRef, idRefName, false);
        }

        ///
        ///
        ///
        void ChangeDetectionOp::_computeOrientedChangeDetection(
            ign::feature::sql::FeatureStorePostgis* fsSource, 
            std::string const & idSourceName,
            ign::feature::sql::FeatureStorePostgis* fsTarget, 
            std::string const & idTargetName,
            bool withAttrCompare
        ) const {
            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();

            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            double const distThreshold = themeParameters->getValue(CD_DIST_THRESHOLD).toDouble();
            std::string const geomMatchName = themeParameters->getValue(GEOM_MATCH).toString();
            std::string const attrMatchName = themeParameters->getValue(ATTR_MATCH).toString();

            ign::feature::FeatureFilter filterSource(countryCodeName+"='"+_countryCode+"'");
            int numSourceFeatures = epg::sql::tools::numFeatures(*fsSource, filterSource);
            boost::progress_display display(numSourceFeatures, std::cout, "[ oriented change detection % complete ]\n");

            ign::feature::FeatureIteratorPtr itSource = fsSource->getFeatures(filterSource);
            while (itSource->hasNext())
            {
                ++display;
                
                ign::feature::Feature const& fSource = itSource->next();
                ign::geometry::Geometry const& geomSource = fSource.getGeometry();
                std::string idSource = fSource.getId();

                ign::feature::FeatureFilter filterTarget(countryCodeName+"='"+_countryCode+"'");
				epg::tools::FilterTools::addAndConditions(filterTarget, "ST_DISTANCE(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + geomSource.toString() + "'),3035)) < "+ign::data::Double(distThreshold).toString());

                ign::feature::Feature fCd = _fsCd->newFeature();
                fCd.setAttribute(idSourceName, ign::data::String(idSource));
                fCd.setAttribute(countryCodeName, ign::data::String(_countryCode));

                /// \todo voir si c'est mieux de conserver tous les matchs ou juste le meilleur
                double distMax = distThreshold;
                std::string idMax = "";
                bool attEqualMax = false;
                ign::feature::FeatureIteratorPtr itTarget = fsTarget->getFeatures(filterTarget);
                while (itTarget->hasNext())
                {
                    ign::feature::Feature const& fTarget = itTarget->next();
                    ign::geometry::Geometry const& geomTarget = fTarget.getGeometry();
                    std::string idTarget = fTarget.getId();

                    double hausdorffDist = ign::geometry::algorithm::OptimizedHausdorffDistanceOp::orientedDistance( geomSource, geomTarget, -1, distThreshold );
                    
                    if( hausdorffDist >= 0 && hausdorffDist < distMax) {
                        distMax = hausdorffDist;
                        idMax = idTarget;
                        if ( withAttrCompare ) attEqualMax = _attributsEqual(fSource, fTarget);
                        
                        if ( distMax == 0 ) break;
                    } 
                }
                if ( idMax != "" ) {
                    if ( withAttrCompare ) fCd.setAttribute(attrMatchName, ign::data::Boolean(attEqualMax));
                    fCd.setAttribute(geomMatchName, ign::data::Boolean(distMax==0));
                    fCd.setAttribute(idTargetName, ign::data::String(idMax));
                } 
                
                _fsCd->createFeature(fCd);  
            }
        }

        ///
        ///
        ///
		bool ChangeDetectionOp::_attributsEqual(
			ign::feature::Feature const& feat1,
			ign::feature::Feature const& feat2
		) const {
            std::map<std::string, std::string> mAtt1;
            for( size_t i = 0 ; i < feat1.numAttributes() ; ++i )
            {
                if( feat1.getAttributeName( i ) == feat1.getFeatureType().getDefaultGeometryName() ) continue;
                if( feat1.getAttributeName( i ) == feat1.getFeatureType().getIdName() ) continue;
                mAtt1.insert(std::make_pair(feat1.getAttributeName( i ), feat1.getAttribute( i ).toString()));
            }

            std::map<std::string, std::string> mAtt2;
            for( size_t i = 0 ; i < feat2.numAttributes() ; ++i )
            {
                if( feat2.getAttributeName( i ) == feat2.getFeatureType().getDefaultGeometryName() ) continue;
                if( feat2.getAttributeName( i ) == feat2.getFeatureType().getIdName() ) continue;
                mAtt2.insert(std::make_pair(feat2.getAttributeName( i ), feat2.getAttribute( i ).toString()));
            }

            if ( mAtt2.size() != mAtt1.size() ) return false;

            std::map<std::string, std::string>::const_iterator mit1;
            for ( mit1 = mAtt1.begin() ; mit1 != mAtt1.end() ; ++mit1) {
                std::map<std::string, std::string>::const_iterator mit2 = mAtt2.find(mit1->first);
                if( mit2 == mAtt2.end() ) return false;
                if( mit2->second != mit1->second ) return false;
            }
            return true;

        }
    }
}