#include "PER_STR.h"

#include "text.h"

PER_STR::PER_STR() : m_nSoilLayers(-1), m_dt(-1), m_nCells(-1), m_frozenT(NODATA_VALUE),
                     m_ks(nullptr), m_sat(nullptr), m_fc(nullptr),
                     m_soilThick(nullptr), m_soilLayers(nullptr),
                     m_infil(nullptr), m_soilT(nullptr), m_soilStorage(nullptr), m_soilStorageProfile(nullptr),
                     m_potVol(nullptr), m_surfQmm(nullptr),
                     m_perc(nullptr) {
}

PER_STR::~PER_STR() {
    if (m_perc != nullptr) Release2DArray(m_nCells, m_perc);
}

void PER_STR:: InitialOutputs() {
    CHECK_POSITIVE(MID_PER_STR, m_nCells);
    if (nullptr == m_perc) { Initialize2DArray(m_nCells, m_nSoilLayers, m_perc, 0.f); }
}

int PER_STR::Execute() {
    CheckInputData();
     InitialOutputs();
#pragma omp parallel for
    for (int i = 0; i < m_nCells; i++) {
        // Note that, infiltration, pothole seepage, irrigation etc. have been added to
        // the first soil layer in other modules. By LJ
        float excessWater = 0.f, maxSoilWater = 0.f, fcSoilWater = 0.f;
        for (int j = 0; j < (int) m_soilLayers[i]; j++) {
            excessWater = 0.f;
            maxSoilWater = m_sat[i][j];
            fcSoilWater = m_fc[i][j];
            // determine gravity drained water in layer
            excessWater += m_soilStorage[i][j] - fcSoilWater;
            //if (i == 100)
            //	cout<<"lyr: "<<j<<", soil storage: "<<m_soilStorage[i][j]<<", fc: "<<fcSoilWater<<", excess: "<<excessWater<<endl;
            // for the upper two layers, soil may be frozen
            if (j == 0 && m_soilT[i] <= m_frozenT) {
                continue;
            }
            m_perc[i][j] = 0.f;
            // No movement if soil moisture is below field capacity
            if (excessWater > 1.e-5f) {
                float maxPerc = maxSoilWater - fcSoilWater;
                if (maxPerc < 0.f) maxPerc = 0.1f;
                float tt = 3600.f * maxPerc / m_ks[i][j]; // secs
                m_perc[i][j] = excessWater * (1.f - exp(-m_dt / tt)); // secs

                if (m_perc[i][j] > maxPerc) {
                    m_perc[i][j] = maxPerc;
                }
                //Adjust the moisture content in the current layer, and the layer immediately below it
                m_soilStorage[i][j] -= m_perc[i][j];
                excessWater -= m_perc[i][j];
                m_soilStorage[i][j] = max(UTIL_ZERO, m_soilStorage[i][j]);
                // redistribute soil water if above field capacity (high water table), rewrite from sat_excess.f of SWAT
                //float qlyr = m_soilStorage[i][j];
                if (j < (int) m_soilLayers[i] - 1) {
                    m_soilStorage[i][j + 1] += m_perc[i][j];
                    if (m_soilStorage[i][j] - m_sat[i][j] > 1.e-4f) {
                        m_soilStorage[i][j + 1] += m_soilStorage[i][j] - m_sat[i][j];
                        m_soilStorage[i][j] = m_sat[i][j];
                    }
                } else { /// for the last soil layer
                    if (m_soilStorage[i][j] - m_sat[i][j] > 1.e-4f) {
                        float ul_excess = m_soilStorage[i][j] - m_sat[i][j];
                        m_soilStorage[i][j] = m_sat[i][j];
                        for (int ly = (int) m_soilLayers[i] - 2; ly >= 0; ly--) {
                            m_soilStorage[i][ly] += ul_excess;
                            if (m_soilStorage[i][ly] > m_sat[i][ly]) {
                                ul_excess = m_soilStorage[i][ly] - m_sat[i][ly];
                                m_soilStorage[i][ly] = m_sat[i][ly];
                            } else {
                                ul_excess = 0.f;
                                break;
                            }
                            if (ly == 0 && ul_excess > 0.f) {
                                // add ul_excess to depressional storage and then to surfq
                                if (m_potVol != NULL) {
                                    m_potVol[i] += ul_excess;
                                } else {
                                    m_surfQmm[i] += ul_excess;
                                }
                            }
                        }
                    }
                }
            } else {
                m_perc[i][j] = 0.f;
            }
        }
        /// update soil profile water
        m_soilStorageProfile[i] = 0.f;
        for (int ly = 0; ly < (int) m_soilLayers[i]; ly++) {
            m_soilStorageProfile[i] += m_soilStorage[i][ly];
        }
    }
    // DEBUG
    //cout << "PER_STR, cell id 14377, m_soilStorage: ";
    //for (int i = 0; i < (int)m_soilLayers[14377]; i++)
    //    cout << m_soilStorage[14377][i] << ", ";
    //cout << endl;
    // END OF DEBUG
    return 0;
}

void PER_STR::Get2DData(const char *key, int *nRows, int *nCols, float ***data) {
     InitialOutputs();
    string sk(key);
    *nRows = m_nCells;
    *nCols = m_nSoilLayers;
    if (StringMatch(sk, VAR_PERCO)) { *data = m_perc; }
    else {
        throw ModelException(MID_PER_STR, "Get2DData", "Output " + sk + " does not exist.");
    }
}

void PER_STR::Set1DData(const char *key, int nRows, float *data) {
    CheckInputSize(key, nRows);
    string sk(key);
    if (StringMatch(sk, VAR_SOTE)) { m_soilT = data; }
    else if (StringMatch(sk, VAR_INFIL)) { m_infil = data; }
    else if (StringMatch(sk, VAR_SOILLAYERS)) { m_soilLayers = data; }
    else if (StringMatch(sk, VAR_SOL_SW)) { m_soilStorageProfile = data; }
    else if (StringMatch(sk, VAR_POT_VOL)) { m_potVol = data; }
    else if (StringMatch(sk, VAR_SURU)) { m_surfQmm = data; }
    else {
        throw ModelException(MID_PER_STR, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void PER_STR::Set2DData(const char *key, int nrows, int ncols, float **data) {
    CheckInputSize(key, nrows);
    string sk(key);
    m_nSoilLayers = ncols;
    if (StringMatch(sk, VAR_CONDUCT)) { m_ks = data; }
    else if (StringMatch(sk, VAR_SOILTHICK)) { m_soilThick = data; }
    else if (StringMatch(sk, VAR_SOL_UL)) { m_sat = data; }
    else if (StringMatch(sk, VAR_SOL_AWC)) { m_fc = data; }
    else if (StringMatch(sk, VAR_SOL_ST)) { m_soilStorage = data; }
    else {
        throw ModelException(MID_PER_STR, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

void PER_STR::SetValue(const char *key, float data) {
    string s(key);
    if (StringMatch(s, Tag_TimeStep)) { m_dt = int(data); }
    else if (StringMatch(s, VAR_T_SOIL)) { m_frozenT = data; }
    else {
        throw ModelException(MID_PER_STR, "SetValue",
                             "Parameter " + s + " does not exist in current module.");
    }
}

bool PER_STR::CheckInputData() {
    CHECK_POSITIVE(MID_PER_STR, m_date);
    CHECK_POSITIVE(MID_PER_STR, m_nCells);
    CHECK_POSITIVE(MID_PER_STR, m_dt);
    CHECK_POINTER(MID_PER_STR, m_ks);
    CHECK_POINTER(MID_PER_STR, m_sat);
    CHECK_DATA(MID_PER_STR, FloatEqual(m_frozenT, NODATA_VALUE), "The threshold soil freezing temperature has not been set.");
    CHECK_POINTER(MID_PER_STR, m_fc);
    CHECK_POINTER(MID_PER_STR, m_soilStorage);
    CHECK_POINTER(MID_PER_STR, m_soilStorageProfile);
    CHECK_POINTER(MID_PER_STR, m_soilThick);
    CHECK_POINTER(MID_PER_STR, m_soilT);
    CHECK_POINTER(MID_PER_STR, m_infil);
    return true;
}

bool PER_STR::CheckInputSize(const char *key, int n) {
    if (n <= 0) {
        throw ModelException(MID_PER_STR, "CheckInputSize",
                             "Input data for " + string(key) + " is invalid. The size could not be less than zero.");
    }
    if (m_nCells != n) {
        if (m_nCells <= 0) { m_nCells = n; }
        else {
            throw ModelException(MID_PER_STR, "CheckInputSize", "Input data for " + string(key) +
                " is invalid. All the input data should have same size.");
        }
    }
    return true;
}
