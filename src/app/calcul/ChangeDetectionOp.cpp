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
#include <epg/tools/MultiLineStringTool.h>

//SOCLE
#include <ign/geometry/algorithm/OptimizedHausdorffDistanceOp.h>
#include <ign/geometry/index/QuadTree.h>


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
            _verbose(verbose),
            _separator("+")
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

                //DEBUG
                // _logger->log(epg::log::DEBUG, context->getDataBaseManager().getPGSearchPath());
                // _logger->log(epg::log::DEBUG, context->getDataBaseManager().getSearchPath());

                // std::string oldSearchPath = context->getDataBaseManager().getPGSearchPath();
                // context->getDataBaseManager().setSearchPath(themeParameters->getValue(UP_SCHEMA).toString());

                // //DEBUG
                // _logger->log(epg::log::DEBUG, context->getDataBaseManager().getPGSearchPath());
                // _logger->log(epg::log::DEBUG, context->getDataBaseManager().getSearchPath());

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

                // context->getDataBaseManager().setSearchPath(oldSearchPath);

                // //DEBUG
                // _logger->log(epg::log::DEBUG, context->getDataBaseManager().getPGSearchPath());
                // _logger->log(epg::log::DEBUG, context->getDataBaseManager().getSearchPath());
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
            _computeChangeDetection();
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

            std::vector<ign::geometry::Envelope> vBbox = _getBboxes();
            for (std::vector<ign::geometry::Envelope>::const_iterator vit = vBbox.begin() ; vit != vBbox.end() ; ++vit ) {

                ign::feature::FeatureFilter filter(*vit, countryCodeName+"='"+_countryCode+"'");
                //DEBUG
                // epg::tools::FilterTools::addAndConditions(filter, "ST_intersects(geom, ST_SetSRID(ST_Envelope('LINESTRING(4032636 2935777, 4033222 2935310)'::geometry), 3035))");

                 //LOAD START
                ign::geometry::index::QuadTree<int> qTree;
                std::vector<ign::feature::Feature> vFeatures;
                qTree.ensureExtent( *vit );

                int numTargetFeatures = epg::sql::tools::numFeatures(*fsTarget, filter);
                boost::progress_display displayLoad(numTargetFeatures, std::cout, "[ load target % complete ]\n");
                ign::feature::FeatureIteratorPtr itTarget = fsTarget->getFeatures(filter);
                while (itTarget->hasNext())
                {
                    ++displayLoad;
                    ign::feature::Feature const& fTarget = itTarget->next();
                    ign::geometry::Geometry const& geomSource = fTarget.getGeometry();

                    vFeatures.push_back(fTarget);
                    qTree.insert( vFeatures.size()-1, geomSource.getEnvelope() );
                }
                //LOAD END

                int numSourceFeatures = epg::sql::tools::numFeatures(*fsSource, filter);
                boost::progress_display display(numSourceFeatures, std::cout, "[ oriented change detection % complete ]\n");

                ign::feature::FeatureIteratorPtr itSource = fsSource->getFeatures(filter);
                while (itSource->hasNext())
                {
                    ++display;
                    
                    ign::feature::Feature const& fSource = itSource->next();
                    ign::geometry::Geometry const& geomSource = fSource.getGeometry();
                    std::string idSource = fSource.getId();

                    //DEBUG
                    // if (idSource == "73c3038d-1629-43d9-a5ea-7116a8e36202") {
                    //     bool test = true;
                    // }

                    ign::feature::Feature fCd = _fsCd->newFeature();
                    fCd.setAttribute(idSourceName, ign::data::String(idSource));
                    fCd.setAttribute(countryCodeName, ign::data::String(_countryCode));

                    /// \todo voir si c'est mieux de conserver tous les matchs ou juste le meilleur
                    double distMax = distThreshold;
                    std::string idMax = "";
                    bool attEqualMax = false;

                    //NEW
                    epg::tools::MultiLineStringTool sourceMlsTool( geomSource );
                    std::set<int> sTarget;
                    qTree.query( geomSource.getEnvelope(), sTarget );
                    for( std::set<int>::const_iterator sit = sTarget.begin() ; sit != sTarget.end() ; ++sit ) {
                        ign::feature::Feature const& fTarget = vFeatures[*sit];
                        std::string idTarget = fTarget.getId();
                        double hausdorffDist = sourceMlsTool.orientedHausdorff(fTarget.getGeometry(), distThreshold);

                    // ign::feature::FeatureFilter filterTarget(countryCodeName+"='"+_countryCode+"'");
                    // epg::tools::FilterTools::addAndConditions(filterTarget, "ST_DISTANCE(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + geomSource.toString() + "'),3035)) < "+ign::data::Double(distThreshold).toString());
                    // ign::feature::FeatureIteratorPtr itTarget = fsTarget->getFeatures(filterTarget);
                    // while (itTarget->hasNext())
                    // {
                    //     ign::feature::Feature const& fTarget = itTarget->next();
                    //     ign::geometry::Geometry const& geomTarget = fTarget.getGeometry();
                    //     std::string idTarget = fTarget.getId();

                        //DEBUG
                        // if (idTarget == "73c3038d-1629-43d9-a5ea-7116a8e36202") {
                        //     bool test = true;
                        // }

                        // double hausdorffDist = ign::geometry::algorithm::OptimizedHausdorffDistanceOp::orientedDistance( geomSource, geomTarget, -1, distThreshold );
                        
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

        // supprimer les doublons si dallage
        // si id1 apparaît un nombre de fois autre que 2 on supprime objet1
        // si id2 apparaît un nombre de fois autre que 2 on ajoute objet2
        // si on a 2 fois le couple id1/id2 on regarde si stable ou modifié
        // pour les cas qui restent (couples qu'on ne trouve qu'une fois) : on supprime l'objet de la table 1 et on ajout objet de la table 2
        void ChangeDetectionOp::_computeChangeDetection() const {
            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            
            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const geomMatchName = themeParameters->getValue(GEOM_MATCH).toString();
            std::string const attrMatchName = themeParameters->getValue(ATTR_MATCH).toString();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();

            // 1ere passe:
            // On rempli les containers
            std::set<std::string> sDeleted;
            std::set<std::string> sCreated;
            std::multimap<std::string, std::pair<bool, bool>> mmMatch; // concaténer id1/id2

            ign::feature::FeatureFilter filter(countryCodeName+"='"+_countryCode+"'");
            int numFeatures = epg::sql::tools::numFeatures(*_fsCd, filter);
            boost::progress_display display(numFeatures, std::cout, "[ change detection (1/2) % complete ]\n");

            ign::feature::FeatureIteratorPtr itFeatCd = _fsCd->getFeatures(filter);
            while (itFeatCd->hasNext())
            {
                ++display;

                ign::feature::Feature const& fCd = itFeatCd->next();
                ign::data::Variant const& idRef = fCd.getAttribute(idRefName);
                ign::data::Variant const& idUp = fCd.getAttribute(idUpName);
                ign::data::Variant const& attrMatch = fCd.getAttribute(attrMatchName);
                ign::data::Variant const& geomMatch = fCd.getAttribute(geomMatchName);

                //Si idRef not defined
                if (fCd.getAttribute(idRefName).isNull())
                    sCreated.insert(idUp.toString());
                //Si idUp not defined
                else if (fCd.getAttribute(idUpName).isNull())
                    sDeleted.insert(idRef.toString());
                else {
                    //si attrMatch not defined -> le mettre à true (null lors de la comparaison t2 -> t1 car déjà calculé lors de la comparaison t1 -> t2)
                    bool isAttMatch = attrMatch.isNull() ? true : attrMatch.toBoolean();
                    mmMatch.insert(std::make_pair(idRef.toString()+_separator+idUp.toString(), std::make_pair(geomMatch.toBoolean(), isAttMatch)));
                }
            }

            // 2eme passe :
            std::vector<std::pair<std::string, std::string>> vpModified;
            // on parcours mmMatch
            // Si on trouve 2 fois id1/id2 = match
            //   Si modif on ajoute dans vpModified
            //   Si stab on ne fait rien
            // Sinon on ajoute id1 dans sDeleted et id2 dans sCreated
            std::set<std::string> sTreated;

            boost::progress_display display2(mmMatch.size(), std::cout, "[ change detection(2/2) % complete ]\n");

            std::multimap<std::string, std::pair<bool, bool>>::const_iterator mmit;
            for( mmit = mmMatch.begin() ; mmit != mmMatch.end() ; ++mmit ) {
                //DEBUG
                // if( mmit->first == "73c3038d-1629-43d9-a5ea-7116a8e36202+73c3038d-1629-43d9-a5ea-7116a8e36202") {
                //     bool test = true;
                // }

                ++display2;
                if (sTreated.find(mmit->first) != sTreated.end() ) continue;

                size_t count = 0;
                bool stability = true;
                auto range = mmMatch.equal_range(mmit->first);
                for (auto rit = range.first; rit != range.second; ++rit) {
                    ++count;
                    if (!rit->second.first || !rit->second.second ) stability = false;
                }
                if (count == 2) {
                    if ( !stability ) {
                        std::vector<std::string> vIds;
                        epg::tools::StringTools::Split(mmit->first, _separator, vIds);
                        vpModified.push_back(std::make_pair(vIds.front(), vIds.back()));
                    }
                } else {
                    std::vector<std::string> vIds;
                    epg::tools::StringTools::Split(mmit->first, _separator, vIds);
                    sDeleted.insert(vIds.front());
                    sCreated.insert(vIds.back());
                }

                sTreated.insert(mmit->first);
            }

            //DEBUG
            _updateCDTAble( vpModified, sDeleted, sCreated );
        }

        ///
        ///
        ///
        std::vector<ign::geometry::Envelope> ChangeDetectionOp::_getBboxes() const {

            return std::vector<ign::geometry::Envelope>(1,ign::geometry::LineString(ign::geometry::Point(4007120, 3018635), ign::geometry::Point(4077702, 2931477)).getEnvelope());

            std::vector<ign::geometry::Envelope> vBbox;

            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const idName = epgParams.getValue(ID).toString();
            std::string const geomName = epgParams.getValue(GEOM).toString();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();
            
            // theme parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const landTableName = themeParameters->getValue(LANDMASK_TABLE).toString();

            ign::feature::FeatureFilter filter(countryCodeName+"='"+_countryCode+"'");
            ign::feature::sql::FeatureStorePostgis* fsLand = epg::ContextS::getInstance()->getDataBaseManager().getFeatureStore(landTableName, idName, geomName);
            ign::feature::FeatureIteratorPtr itLand = fsLand->getFeatures(filter);
            ign::geometry::Envelope landBbox;
            while (itLand->hasNext())
            {
                ign::feature::Feature const& fLand = itLand->next();
                ign::geometry::Geometry const& geomLand = fLand.getGeometry();

                vBbox.push_back(geomLand.getEnvelope());
            }
            delete fsLand;

            return vBbox;
        }

        ///
        ///
        ///
        void ChangeDetectionOp::_updateCDTAble(
            std::vector<std::pair<std::string, std::string>> const& vpModified,
            std::set<std::string> const& sDeleted,
            std::set<std::string> const& sCreated
        ) const {
            //--
            epg::Context *context = epg::ContextS::getInstance();
            
            // epg parameters
            epg::params::EpgParameters const& epgParams = epg::ContextS::getInstance()->getEpgParameters();
            std::string const countryCodeName = epgParams.getValue(COUNTRY_CODE).toString();

            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string const tableName = themeParameters->getValue(TABLE).toString();
            std::string cdTableName = tableName + themeParameters->getValue(TABLE_CD_SUFFIX).toString();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();

            {
                std::ostringstream ss;
                ss << "DELETE FROM " << cdTableName << " WHERE " << countryCodeName << " = '" << _countryCode << "';";
                context->getDataBaseManager().getConnection()->update(ss.str());
            }
            
            ign::feature::Feature fCd = _fsCd->newFeature();
            fCd.setAttribute(countryCodeName, ign::data::String(_countryCode));
            
            std::vector<std::pair<std::string, std::string>>::const_iterator vpit;
            for( vpit = vpModified.begin() ; vpit != vpModified.end() ; ++vpit ) {
                fCd.setAttribute(idRefName, ign::data::String(vpit->first));
                fCd.setAttribute(idUpName, ign::data::String(vpit->second));
                _fsCd->createFeature(fCd); 
            }

            fCd.setAttribute(idUpName, ign::data::Null());
            for( std::set<std::string>::const_iterator sit = sDeleted.begin() ; sit != sDeleted.end() ; ++sit ) {
                fCd.setAttribute(idRefName, ign::data::String(*sit));
                _fsCd->createFeature(fCd);
            }

            fCd.setAttribute(idRefName, ign::data::Null());
            for( std::set<std::string>::const_iterator sit = sCreated.begin() ; sit != sCreated.end() ; ++sit ) {
                fCd.setAttribute(idUpName, ign::data::String(*sit));
                _fsCd->createFeature(fCd);
            }
        }
    }
}