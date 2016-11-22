#include "solution.h"


std::mutex solutionMutex;


Solution::Solution()
: hasError_(false)
{}


RouteVec& Solution::getRouteVec()
{
  return routeVec_;
}

const RouteVec& Solution::getRouteVec() const
{
  return routeVec_;
}


SolutionInfoVec& Solution::getSolutionInfoVec()
{
  return solutionInfoVec_;
}

const SolutionInfoVec& Solution::getSolutionInfoVec() const
{
  return solutionInfoVec_;
}


void Solution::setErrorBool(bool errorBool)
{
  hasError_ = errorBool;
}

bool Solution::getErrorBool() const
{
  return hasError_;
}
