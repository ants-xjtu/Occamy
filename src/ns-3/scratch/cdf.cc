#include "cdf.h"

#include "ns3/core-module.h"

#include <algorithm>
#include <fstream>

namespace ns3
{

CdfValue::CdfValue(void)
{
    m_value = 0.0;
    m_probability = 0.0;
}

CdfValue::CdfValue(double value, double prob)
{
    NS_ASSERT(prob >= 0.0 && prob <= 1.0);
    m_value = value;
    m_probability = prob;
}

CdfValue::~CdfValue()
{
}

CdfDistribution::CdfDistribution()
{
}

CdfDistribution::~CdfDistribution()
{
}

double
CdfDistribution::GetMean(void) const
{
    double mean = 0.0;
    for (uint32_t i = 1; i < m_cdfs.size(); i++)
    {
        double prob = m_cdfs[i].m_probability - m_cdfs[i - 1].m_probability;
        double val = 0.5 * (m_cdfs[i].m_value + m_cdfs[i - 1].m_value);
        mean += prob * val;
    }
    return mean;
}

Ptr<EmpiricalRandomVariable>
CdfDistribution::CreateEmpiricalRv(void) const
{
    Ptr<EmpiricalRandomVariable> rv = CreateObject<EmpiricalRandomVariable>();
    for (uint32_t i = 0; i < m_cdfs.size(); i++)
    {
        rv->CDF(m_cdfs[i].m_value, m_cdfs[i].m_probability);
    }
    // rv->SetAttribute ("Interpolate", BooleanValue (true));
    return rv;
}

bool
CdfDistribution::Validate() const
{
    for (uint32_t i = 0; i < m_cdfs.size() - 1; i++)
    {
        NS_ASSERT(m_cdfs[i].m_value < m_cdfs[i + 1].m_value);
        NS_ASSERT(m_cdfs[i].m_probability < m_cdfs[i + 1].m_probability);
        if (m_cdfs[i].m_value >= m_cdfs[i + 1].m_value)
        {
            return false;
        }
        if (m_cdfs[i].m_probability >= m_cdfs[i + 1].m_probability)
        {
            return false;
        }
    }
    return true;
}

bool
CompareCdfValue(CdfValue a, CdfValue b)
{
    return a.m_probability < b.m_probability;
}

bool
CdfDistribution::LoadCdf(std::string cdfFname)
{
    double value = 0.0;
    double prob = 0.0;
    m_cdfs.clear();
    std::ifstream cdfFile(cdfFname);
    if (!cdfFile.is_open())
    {
        std::cerr << "Error when opening \"" << cdfFname << "\": " << strerror(errno) << std::endl;
        return false;
    }
    std::vector<CdfValue> flowSizeEmp;
    while (cdfFile >> value >> prob)
    {
        m_cdfs.push_back(CdfValue(value, prob));
    }
    std::sort(m_cdfs.begin(), m_cdfs.end(), CompareCdfValue);
    Validate();
    return true;
}

} // namespace ns3