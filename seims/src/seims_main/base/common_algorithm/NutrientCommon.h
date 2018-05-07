/*!
 * \brief Define some common used function in Nutrient cycling modules, e.g., NUTRMV, NUTRSED
 * \author Liang-Jun Zhu
 * \date 2016-9-28
 */
#ifndef SEIMS_NUTRIENT_COMMON_H
#define SEIMS_NUTRIENT_COMMON_H

/*!
 * \brief Calculate enrichment ratio for nutrient transport with runoff and sediment
 *           enrsb.f of SWAT
 * \param[in] sedyld sediment yield, kg
 * \param[in] surfq surface runoff, mm
 * \param[in] area area, ha
 */
float CalEnrichmentRatio(float sedyld, float surfq, float area);

#endif /* SEIMS_NUTRIENT_COMMON_H */
