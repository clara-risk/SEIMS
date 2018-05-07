/*!
 * \brief Base namespace for implementation of BMP configuration
 * \author Liang-Jun Zhu
 * \description 1. lj 2018-4-12 Code reformat
 */
#ifndef SEIMS_BMP_FACTORY_H
#define SEIMS_BMP_FACTORY_H

#include "db_mongoc.h"
#include "data_raster.h"

#include "seims.h"

using namespace ccgl;
using namespace db_mongoc;
using namespace data_raster;

/*!
 * \brief Base class of all kind of BMPs Factory.
 *        Read from BMP_SCENARIOS collection of MongoDB
 * \ingroup bmps
 */
namespace bmps {
/*!
 * \class BMPFactory
 * \ingroup MainBMP
 *
 * \brief Base class to initiate a BMP data
 *
 */
class BMPFactory: Interface {
public:
    /// Constructor
    BMPFactory(int scenario_id, int bmp_id, int sub_scenario, int bmp_type,
               int bmp_priority, vector<string>& distribution, const string& collection,
               const string& location);

    /// Load BMP parameters from MongoDB
    virtual void loadBMP(MongoClient* conn, const string& bmpDBName) = 0;

    /*!
     * \brief Set raster data if needed
     * This function is not required for each BMP, so DO NOT define as pure virtual function.
     */
    virtual void setRasterData(map<string, FloatRaster *>& sceneRsMap) {
    };

    /*!
    * \brief Get raster data if needed
    * This function is not required for each BMP, so DO NOT define as pure virtual function.
    */
    virtual float* GetRasterData() { return nullptr; }

    /*!  Get BMP type
       1 - reach BMPs which are attached to specific reaches and will change the character of the reach.
       2 - areal structural BMPs which are corresponding to a specific structure in the watershed and will change the character of subbasins/cells.
       3 - areal non-structure BMPs which are NOT corresponding to a specific structure in the watershed and will change the character of subbasins/cells.
       4 - point structural BMPs
     */
    int bmpType() { return m_bmpType; }

    /// Get BMP priority
    int bmpPriority() { return m_bmpPriority; }

    /// Get subScenario ID
    int GetSubScenarioId() { return m_subScenarioId; }

    /// Output
    virtual void Dump(std::ostream* fs) = 0;

protected:
    const int m_scenarioId; ///< Scenario ID
    const int m_bmpId; ///< BMP ID
    const int m_subScenarioId; ///< SubScenario ID within one BMP iD
    const int m_bmpType; ///< BMP Type
    const int m_bmpPriority; ///< BMP Priority
    /*! Distribution vector of BMP
     *  Origin format is [distribution data type]|[distribution parameter name]|Collection name|...
     */
    vector<string> m_distribution;
    const string m_bmpCollection; ///< Collection name
    const string m_location; ///< Define where the BMP will be applied
};
}
#endif /* SEIMS_BMP_FACTORY_H */
