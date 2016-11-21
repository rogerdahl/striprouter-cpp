#include "solution.h"


std::mutex solutionMutex;


Solution::Solution()
: hasError_(false)
{}

RouteVec& Solution::getRouteVec()
{
  return routeVec_;
}

SolutionInfoVec& Solution::getSolutionInfoVec()
{
  return solutionInfoVec_;
}

void Solution::setErrorBool(bool errorBool)
{
  hasError_ = errorBool;
}

bool Solution::getErrorBool()
{
  return hasError_;
}
