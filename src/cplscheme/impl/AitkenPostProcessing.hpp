#pragma once

#include "PostProcessing.hpp"
#include "logging/Logger.hpp"
#include <Eigen/Core>

#include <map>

namespace precice {
namespace cplscheme {
namespace impl {

class AitkenPostProcessing: public PostProcessing
{
public:

  AitkenPostProcessing(
      double initialRelaxationFactor,
      std::vector<int> dataIDs);

  virtual ~AitkenPostProcessing() {}

  virtual std::vector<int> getDataIDs() const
  {
    return _dataIDs;
  }

  virtual void setDesignSpecification(
      Eigen::VectorXd& q);

  virtual std::map<int, Eigen::VectorXd> getDesignSpecification(DataMap& cplData);

  virtual void initialize(
      DataMap& cpldata);

  virtual void performPostProcessing(
      DataMap& cpldata);

  virtual void iterationsConverged(
      DataMap& cpldata);

private:

  static logging::Logger _log;

  double _initialRelaxation;

  std::vector<int> _dataIDs;

  double _aitkenFactor;

  int _iterationCounter;

  Eigen::VectorXd _residuals;

  Eigen::VectorXd _designSpecification;
};

}
}
} // namespace precice, cplscheme, impl

