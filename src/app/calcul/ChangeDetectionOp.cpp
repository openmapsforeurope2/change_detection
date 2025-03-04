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
            std::string const& featureName,
            std::string const& countryCode,
            bool verbose
        ) : 
            _featureName(featureName),
            _countryCode(countryCode),
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
			std::string const& feature,
            std::string const& countryCode,
			bool verbose
		) {
            ChangeDetectionOp op(feature, countryCode, verbose);
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
            std::string const themeName = themeParameters->getValue(THEME_W).toString();
            std::string refTableName = _featureName + themeParameters->getValue(TABLE_REF_SUFFIX).toString();
            std::string upTableName = _featureName + themeParameters->getValue(TABLE_UP_SUFFIX).toString();
            std::string cdTableName = _featureName + themeParameters->getValue(TABLE_CD_SUFFIX).toString();
            std::string const geomMatchName = themeParameters->getValue(GEOM_MATCH).toString();
            std::string const attrMatchName = themeParameters->getValue(ATTR_MATCH).toString();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();
            std::string const separator = themeParameters->getValue(CD_IGNORED_FIELDS_SEPARATOR).toString();
            std::string const ignoredCommonFields = themeParameters->getValue(CD_IGNORED_COMMON_FIELDS).toString();
            std::string ignoredThemeFields = "";
            if ( themeName == "tn" )
                ignoredThemeFields = themeParameters->getValue(CD_IGNORED_TN_FIELDS).toString();
            if ( themeName == "hy" )
                ignoredThemeFields = themeParameters->getValue(CD_IGNORED_HY_FIELDS).toString();
            if ( themeName == "au" )
                ignoredThemeFields = themeParameters->getValue(CD_IGNORED_AU_FIELDS).toString();
            if ( themeName == "ib" )
                ignoredThemeFields = themeParameters->getValue(CD_IGNORED_IB_FIELDS).toString();
            
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
            std::vector<std::string> vParts;;
	        epg::tools::StringTools::Split(ignoredCommonFields, separator, vParts);
            epg::tools::StringTools::Split(ignoredThemeFields, separator, vParts);
            _sIgnoredFields.insert(vParts.begin(), vParts.end());

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
            _removeDuplicates(idRefName);
            _computeOrientedChangeDetection(_fsUp, idUpName, _fsRef, idRefName, false);
            _removeDuplicates(idUpName, true); //on se sert du champ attrMatch pour identifier les entrées de la 2ème passe (attrMAtch est NULL) et les filtrer (suppression doublons)
            _computeChangeDetection();
        }

        ///
        ///
        ///
        void ChangeDetectionOp::_removeDuplicates(std::string const& uniqueKey, bool whereAttMatchIsNull) const {
            //--
            epg::Context *context = epg::ContextS::getInstance();

            // epg parameters
            epg::params::EpgParameters const& epgParams = context->getEpgParameters();
            std::string const idName = epgParams.getValue(ID).toString();

            //app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            std::string cdTableName = _featureName + themeParameters->getValue(TABLE_CD_SUFFIX).toString();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();

            std::ostringstream ss;
            ss  << "DELETE FROM "
                << cdTableName << " a "
                << "USING " << cdTableName << " b "
                << "WHERE "
                << "a."<< idName << " < b."<< idName << " "
                << "AND "
                << "a."<< uniqueKey << " = b."<< uniqueKey << " ";

            if ( whereAttMatchIsNull ) {
                std::string const attrMatchName = themeParameters->getValue(ATTR_MATCH).toString();
                ss  << "AND a."<< attrMatchName << " IS NULL "
                    << "AND b."<< attrMatchName << " IS NULL ";
            }
            
            ss << ";";

            context->getDataBaseManager().getConnection()->update(ss.str());
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
            double const cdDistThreshold = themeParameters->getValue(CD_DIST_THRESHOLD).toDouble();
            double const egalDistThreshold = themeParameters->getValue(CD_EGAL_DIST_THRESHOLD).toDouble();
            std::string const geomMatchName = themeParameters->getValue(GEOM_MATCH).toString();
            std::string const attrMatchName = themeParameters->getValue(ATTR_MATCH).toString();

            std::vector<ign::geometry::Envelope> vBbox = _getBboxes();
            for (std::vector<ign::geometry::Envelope>::const_iterator vit = vBbox.begin() ; vit != vBbox.end() ; ++vit ) {
                std::cout << "processing BBOX [" << (vit-vBbox.begin())+1 << "/" << vBbox.size() << "]";
                ign::feature::FeatureFilter filter(*vit, countryCodeName+"='"+_countryCode+"'");
                //DEBUG
                // epg::tools::FilterTools::addAndConditions(filter, "ST_intersects(geom, ST_SetSRID(ST_Envelope('LINESTRING(4042018.800 2954022.411, 4042027.616 2954015.189)'::geometry), 3035))");

                //LOAD START
                ign::geometry::Envelope bBoxTarget = *vit;
                bBoxTarget.expandBy(cdDistThreshold);
                ign::feature::FeatureFilter filterTarget(bBoxTarget, countryCodeName+"='"+_countryCode+"'");
                ign::geometry::index::QuadTree<int> qTree;
                std::vector<ign::feature::Feature> vFeatures;
                qTree.ensureExtent( bBoxTarget );

                int numTargetFeatures = epg::sql::tools::numFeatures(*fsTarget, filterTarget);
                boost::progress_display displayLoad(numTargetFeatures, std::cout, "[ load target % complete ]\n");
                ign::feature::FeatureIteratorPtr itTarget = fsTarget->getFeatures(filterTarget);
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

                    ign::feature::Feature fCd = _fsCd->newFeature();
                    fCd.setAttribute(idSourceName, ign::data::String(idSource));
                    fCd.setAttribute(countryCodeName, ign::data::String(_countryCode));

                    /// \todo voir si c'est mieux de conserver tous les matchs ou juste le meilleur
                    double distMax = cdDistThreshold;
                    std::string idMax = "";
                    bool attEqualMax = false;

                    //NEW
                    epg::tools::MultiLineStringTool sourceMlsTool( geomSource );
                    std::set<int> sTarget;
                    qTree.query( geomSource.getEnvelope(), sTarget );
                    for( std::set<int>::const_iterator sit = sTarget.begin() ; sit != sTarget.end() ; ++sit ) {
                        ign::feature::Feature const& fTarget = vFeatures[*sit];
                        std::string idTarget = fTarget.getId();

                        double hausdorffDist = sourceMlsTool.orientedHausdorff(fTarget.getGeometry(), cdDistThreshold);
                        
                        if( hausdorffDist >= 0 && hausdorffDist < distMax) {
                            distMax = hausdorffDist;
                            idMax = idTarget;
                            if ( withAttrCompare ) attEqualMax = _attributsEqual(fSource, fTarget);
                            
                            if ( distMax == 0 ) break;
                        } 
                    }
                    if ( idMax != "" ) {
                        if ( withAttrCompare ) fCd.setAttribute(attrMatchName, ign::data::Boolean(attEqualMax));
                        fCd.setAttribute(geomMatchName, ign::data::Boolean(distMax <= egalDistThreshold));
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
                std::string const& fieldName = feat1.getAttributeName(i);
                if( _sIgnoredFields.find(fieldName) != _sIgnoredFields.end() ) continue;
                if( fieldName == feat1.getFeatureType().getDefaultGeometryName() ) continue;
                if( fieldName == feat1.getFeatureType().getIdName() ) continue;
                mAtt1.insert(std::make_pair(fieldName, feat1.getAttribute(i).toString()));
            }

            std::map<std::string, std::string> mAtt2;
            for( size_t i = 0 ; i < feat2.numAttributes() ; ++i )
            {
                std::string const& fieldName = feat2.getAttributeName(i);
                if( _sIgnoredFields.find(fieldName) != _sIgnoredFields.end() ) continue;
                if( fieldName == feat2.getFeatureType().getDefaultGeometryName() ) continue;
                if( fieldName == feat2.getFeatureType().getIdName() ) continue;
                mAtt2.insert(std::make_pair(fieldName, feat2.getAttribute(i).toString()));
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
                if (idRef.isNull())
                    sCreated.insert(idUp.toString());
                //Si idUp not defined
                else if (idUp.isNull())
                    sDeleted.insert(idRef.toString());
                else {
                    //si attrMatch not defined -> le mettre à true (null lors de la comparaison t2 -> t1 car déjà calculé lors de la comparaison t1 -> t2)
                    bool isAttMatch = attrMatch.isNull() ? true : attrMatch.toBoolean();
                    mmMatch.insert(std::make_pair(idRef.toString()+_separator+idUp.toString(), std::make_pair(geomMatch.toBoolean(), isAttMatch)));
                }
            }

            // 2eme passe :
            std::vector<boost::tuple<std::string, std::string, bool, bool>> vtModified;

            // on parcours mmMatch
            // Si on trouve 2 fois id1/id2 = match
            //   Si modif on ajoute dans vtModified
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
                bool attrStability = true;
                bool geomStability = true;
                auto range = mmMatch.equal_range(mmit->first);
                for (auto rit = range.first; rit != range.second; ++rit) {
                    ++count;
                    if (!rit->second.first) geomStability = false;
                    if (!rit->second.second) attrStability = false;
                }
                if (count == 2) {
                    if ( !geomStability || !attrStability ) {
                        std::vector<std::string> vIds;
                        epg::tools::StringTools::Split(mmit->first, _separator, vIds);
                        vtModified.push_back(boost::make_tuple(vIds.front(), vIds.back(), geomStability, attrStability));
                    }
                } else {
                    std::vector<std::string> vIds;
                    epg::tools::StringTools::Split(mmit->first, _separator, vIds);
                    sDeleted.insert(vIds.front());
                    sCreated.insert(vIds.back());
                }

                sTreated.insert(mmit->first);
            }

            //3eme passe : nettoyage:
            std::vector<boost::tuple<std::string, std::string, bool, bool>>::const_iterator vtit;
            for( vtit = vtModified.begin() ; vtit != vtModified.end() ; ++vtit ) {
                std::string idRef = boost::get<0>(*vtit);
                std::set<std::string>::const_iterator itDel = sDeleted.find(idRef);
                if( itDel != sDeleted.end() ) sDeleted.erase(itDel);
                
                std::string idUp = boost::get<1>(*vtit);
                std::set<std::string>::const_iterator itUp = sCreated.find(idUp);
                if( itUp != sCreated.end() ) sCreated.erase(itUp);
            }

            //--
            _updateCDTAble( vtModified, sDeleted, sCreated );
        }

        ///
        ///
        ///
        void ChangeDetectionOp::_explodeBbox(
            ign::geometry::Envelope const& bbox,
            std::vector<ign::geometry::Envelope> & vBbox
        ) const {
            // app parameters
            params::ThemeParameters *themeParameters = params::ThemeParametersS::getInstance();
            double const pitch = themeParameters->getValue(CD_BBOX_MAX_SIDE_LENGTH).toDouble();

            std::set<double> sXbounds = {bbox.xmin(), bbox.xmax()};
            double nextXbound = *sXbounds.begin() + pitch;
            while (nextXbound < *sXbounds.rbegin()) {
                sXbounds.insert(nextXbound);
                nextXbound += pitch;
            }

            std::set<double> sYbounds = {bbox.ymin(), bbox.ymax()};
            double nextYbound = *sYbounds.begin() + pitch;
            while (nextYbound < *sYbounds.rbegin()) {
                sYbounds.insert(nextYbound);
                nextYbound += pitch;
            }
            
            std::set<double>::const_iterator sitxmin = sXbounds.begin();
            std::set<double>::const_iterator sitxmax = sitxmin;
            for ( ++sitxmax ; sitxmax != sXbounds.end() ; ++sitxmax, ++sitxmin) {
                std::set<double>::const_iterator sitymin = sYbounds.begin();
                std::set<double>::const_iterator sitymax = sitymin;
                for ( ++sitymax ; sitymax != sYbounds.end() ; ++sitymax, ++sitymin ) {
                    vBbox.push_back(ign::geometry::Envelope(*sitxmin, *sitxmax, *sitymin, *sitymax));
                }
            }
        }

        ///
        ///
        ///
        std::vector<ign::geometry::Envelope> ChangeDetectionOp::_getBboxes() const {

            //DEBUG
            std::vector<ign::geometry::Envelope> vBboxDebug;
            _explodeBbox(ign::geometry::LineString(ign::geometry::Point(4010116,3017065), ign::geometry::Point(4075982,2929050)).getEnvelope(), vBboxDebug);
            // _explodeBbox(ign::geometry::LineString(ign::geometry::Point(4042018.800, 2954022.411), ign::geometry::Point(4042027.616, 2954015.189)).getEnvelope(), vBboxDebug);
            return vBboxDebug;

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

                _explodeBbox(geomLand.getEnvelope(), vBbox);
            }
            delete fsLand;

            return vBbox;
        }

        ///
        ///
        ///
        void ChangeDetectionOp::_updateCDTAble(
            std::vector<boost::tuple<std::string, std::string, bool, bool>> const& vtModified,
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
            std::string cdTableName = _featureName + themeParameters->getValue(TABLE_CD_SUFFIX).toString();
            std::string const idRefName = themeParameters->getValue(ID_REF).toString();
            std::string const idUpName = themeParameters->getValue(ID_UP).toString();
            std::string const geomMatchName = themeParameters->getValue(GEOM_MATCH).toString();
            std::string const attrMatchName = themeParameters->getValue(ATTR_MATCH).toString();

            {
                std::ostringstream ss;
                ss << "DELETE FROM " << cdTableName << " WHERE " << countryCodeName << " = '" << _countryCode << "';";
                context->getDataBaseManager().getConnection()->update(ss.str());
            }
            
            ign::feature::Feature fCd = _fsCd->newFeature();
            fCd.setAttribute(countryCodeName, ign::data::String(_countryCode));
            
            std::vector<boost::tuple<std::string, std::string, bool, bool>>::const_iterator vtit;
            for( vtit = vtModified.begin() ; vtit != vtModified.end() ; ++vtit ) {
                fCd.setAttribute(idRefName, ign::data::String(boost::get<0>(*vtit)));
                fCd.setAttribute(idUpName, ign::data::String(boost::get<1>(*vtit)));
                fCd.setAttribute(geomMatchName, ign::data::Boolean(boost::get<2>(*vtit)));
                fCd.setAttribute(attrMatchName, ign::data::Boolean(boost::get<3>(*vtit)));

                ign::feature::Feature fRef;
                _fsRef->getFeatureById(boost::get<0>(*vtit), fRef);
                fCd.setGeometry(fRef.getGeometry());

                _fsCd->createFeature(fCd); 
            }

            fCd.setAttribute(geomMatchName, ign::data::Null());
            fCd.setAttribute(attrMatchName, ign::data::Null());
            fCd.setAttribute(idUpName, ign::data::Null());
            for( std::set<std::string>::const_iterator sit = sDeleted.begin() ; sit != sDeleted.end() ; ++sit ) {
                fCd.setAttribute(idRefName, ign::data::String(*sit));

                ign::feature::Feature fRef;
                _fsRef->getFeatureById(*sit, fRef);
                fCd.setGeometry(fRef.getGeometry());

                _fsCd->createFeature(fCd);
            }

            fCd.setAttribute(idRefName, ign::data::Null());
            for( std::set<std::string>::const_iterator sit = sCreated.begin() ; sit != sCreated.end() ; ++sit ) {
                fCd.setAttribute(idUpName, ign::data::String(*sit));

                ign::feature::Feature fUp;
                _fsUp->getFeatureById(*sit, fUp);
                fCd.setGeometry(fUp.getGeometry());

                _fsCd->createFeature(fCd);
            }
        }
    }
}