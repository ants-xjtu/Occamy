#ifndef NS3_CDF_H
#define NS3_CDF_H

#include "ns3/random-variable-stream.h"

namespace ns3
{
class CdfValue
{
  public:
    CdfValue();
    CdfValue(double value, double prob);
    ~CdfValue();
    double m_value;
    double m_probability;
};

class CdfDistribution
{
  public:
    CdfDistribution();
    ~CdfDistribution();

    double GetMean(void) const;
    Ptr<EmpiricalRandomVariable> CreateEmpiricalRv(void) const;
    bool LoadCdf(std::string cdfFname);

  private:
    bool Validate() const;

    std::vector<CdfValue> m_cdfs;
};

} // namespace ns3

#endif