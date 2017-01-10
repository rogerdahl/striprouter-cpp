//#include <algorithm>
#include <cassert>
//#include <chrono>
//#include <forward_list>
//#include <list>
//#include <set>

#include "ga_interface.h"


GeneticAlgorithm::GeneticAlgorithm(int nOrganismsInPopulation,
                                   double crossoverRate, double mutationRate)
  : nOrganismsInPopulation_(nOrganismsInPopulation),
    crossoverRate_(crossoverRate),
    mutationRate_(mutationRate),
    nConnectionsInCircuit_(0),
    population_(nOrganismsInPopulation, crossoverRate, mutationRate)
{
}

void GeneticAlgorithm::reset(int nConnectionsInCircuit)
{
  assert(!(nConnectionsInCircuit & 1)); // Must have even number of connections
  nConnectionsInCircuit_ = nConnectionsInCircuit;
  population_.reset(nConnectionsInCircuit);
  nextOrderingIdx_ = 0;
  nUnprocessedOrderings_ = nOrganismsInPopulation_;
}

OrderingIdx GeneticAlgorithm::reserveOrdering()
{
  if (!nConnectionsInCircuit_) {
    return -1;
  }
  auto isNewGenerationRequired = nextOrderingIdx_ == nOrganismsInPopulation_;
  auto isAllOrderingsReleased = !nUnprocessedOrderings_;
  if (isNewGenerationRequired) {
    if (isAllOrderingsReleased) {
      population_.nextGeneration();
      nUnprocessedOrderings_ = nOrganismsInPopulation_;
      nextOrderingIdx_ = 0;
    }
    else {
      return -1;
    }
  }
  return nextOrderingIdx_++;
}

ConnectionIdxVec GeneticAlgorithm::getOrdering(OrderingIdx orderingIdx)
{
  assert(orderingIdx != -1); // Must wait and try reserveOrdering() again
  assert(nConnectionsInCircuit_); // Must call reset() first
  return population_.organismVec[orderingIdx].calcConnectionIdxVec();
}

void GeneticAlgorithm::releaseOrdering(
  OrderingIdx orderingIdx, int nCompletedRoutes, long completedRouteCost
)
{
  population_.organismVec[orderingIdx].nCompletedRoutes = nCompletedRoutes;
  population_.organismVec[orderingIdx].completedRouteCost = completedRouteCost;
  --nUnprocessedOrderings_;
}

std::unique_lock<std::mutex> GeneticAlgorithm::scopeLock()
{
  return std::unique_lock<std::mutex>(mutex_);
}

