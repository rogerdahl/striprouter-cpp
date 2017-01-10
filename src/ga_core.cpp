#include <algorithm>
#include <cassert>
#include <chrono>
#include <forward_list>
#include <list>
#include <random>
#include <set>
#include <climits>

#include <fmt/format.h>

#include "ga_core.h"



GeneDependency::GeneDependency(Gene gene, Gene geneDependency)
  : gene(gene), geneDependency(geneDependency)
{
}

//
// Random
//

RandomIntGenerator::RandomIntGenerator()
  : randomEngine_(static_cast<unsigned long>
                  (std::chrono::system_clock::now().time_since_epoch().count()))
{
}

RandomIntGenerator::RandomIntGenerator(int min, int max)
  : randomEngine_(static_cast<unsigned long>
                  (std::chrono::system_clock::now().time_since_epoch().count()))
{
  setRange(min, max);
}

void RandomIntGenerator::setRange(int min, int max)
{
  uniformIntDistribution_ = std::uniform_int_distribution<>(min, max);
}

GeneIdx RandomIntGenerator::getRandomInt()
{
  return uniformIntDistribution_(randomEngine_);
}

//
// Organism
//


Organism::Organism(int nGenes, RandomIntGenerator& randomGeneSelector)
  : nCompletedRoutes(0), completedRouteCost(0), nGenes_(nGenes),
    randomGeneSelector_(randomGeneSelector)
{
}

void Organism::createRandom()
{
  for (int i = 0; i < nGenes_; ++i) {
    geneVec.push_back(randomGeneSelector_.getRandomInt());
  }
}

GeneIdx Organism::getRandomCrossoverPoint()
{
  return randomGeneSelector_.getRandomInt();
}

void Organism::mutate()
{
  auto dependentIdx = randomGeneSelector_.getRandomInt();
  auto dependencyIdx = randomGeneSelector_.getRandomInt();
  geneVec[dependentIdx] = dependencyIdx;
}

GeneVec Organism::calcConnectionIdxVec()
{
  auto geneVec = topoSort();
  assert(static_cast<int>(geneVec.size()) == nGenes_);
  return geneVec;
}

void Organism::dump()
{
  fmt::print("nCompletedRoutes={} completedRouteCost={} nGenes={} genes=",
             nCompletedRoutes, completedRouteCost, nGenes_);
  for (auto& v : geneVec) {
    fmt::print(" {}", v);
  }
  fmt::print("\n");
}

//
// Private
//

GeneVec Organism::topoSort()
{
  std::list<GeneDependency> geneList;
  int i = 0;
//  fmt::print("{}\n", geneVec.size());
  for (auto geneIdx : geneVec) {
    geneList.push_back(GeneDependency(i, geneIdx));
    ++i;
  }

  geneList.sort([](const GeneDependency & a, const GeneDependency & b) -> bool {
    return a.geneDependency < b.geneDependency;
  });

  GeneVec geneVec;
  std::set<int> dependencySet;

  while (geneList.size()) {
    bool found = false;
    auto itr = geneList.begin();
    while (itr != geneList.end()) {
      if (dependencySet.count(itr->geneDependency)) {
        geneVec.push_back(itr->gene);
        dependencySet.insert(itr->gene);
        itr = geneList.erase(itr);
        found = true;
      }
      else {
        ++itr;
      }
    }
    if (!found) {
      geneVec.push_back(geneList.front().gene);
      dependencySet.insert(geneList.front().gene);
      geneList.pop_front();
    }
  }
  return geneVec;
}

//
// Population
//

OrganismPair::OrganismPair(Organism& a, Organism& b)
  : a(a), b(b)
{
}

Population::Population(int nOrganismsInPopulation, double crossoverRate,
                       double mutationRate)
  : nOrganismsInPopulation_(nOrganismsInPopulation),
    crossoverRate_(crossoverRate),
    mutationRate_(mutationRate),
    randomOrganismSelector_(0, nOrganismsInPopulation - 1)
{
  assert(!(nOrganismsInPopulation & 1)); // Must have even number of organisms
}

void Population::reset(int nGenesPerOrganism)
{
  nGenesPerOrganism_ = nGenesPerOrganism;
  randomGeneSelector_.setRange(0, nGenesPerOrganism - 1);
  createRandomPopulation();
}


void Population::nextGeneration()
{
  OrganismVec newGenerationVec;
  auto nMutations = 0;
  for (int i = 0; i < nOrganismsInPopulation_ / 2; ++i) {
    auto pair = selectPairTournament(2);
    if (getNormalizedRandom() < crossoverRate_) {
      crossover(pair);
    }
    if (getNormalizedRandom() < mutationRate_) {
      pair.a.mutate();
      ++nMutations;
    }
    if (getNormalizedRandom() < mutationRate_) {
      pair.b.mutate();
      ++nMutations;
    }
    newGenerationVec.push_back(pair.a);
    newGenerationVec.push_back(pair.b);
  }
  assert(static_cast<int>(newGenerationVec.size()) == nOrganismsInPopulation_);
  organismVec.swap(newGenerationVec);
}

//
// Private
//

void Population::createRandomPopulation()
{
  organismVec.clear();
  for (int i = 0; i < nOrganismsInPopulation_; ++i) {
    Organism organism(nGenesPerOrganism_, randomGeneSelector_);
    organism.createRandom();
    organismVec.push_back(organism);
  }
}

void Population::crossover(OrganismPair& pair)
{
  auto crossIdx = pair.a.getRandomCrossoverPoint();
  for (int i = crossIdx; i < static_cast<int>(pair.a.geneVec.size()); ++i) {
    std::swap(pair.a.geneVec[i], pair.b.geneVec[i]);
  }
}

OrganismPair Population::selectPairTournament(int nCandidates)
{
  auto organismAIdx = tournamentSelect(nCandidates);
  while (true) {
    auto organismBIdx = tournamentSelect(nCandidates);
    if (organismAIdx != organismBIdx) {
      return OrganismPair(organismVec[organismAIdx], organismVec[organismBIdx]);
    }
  }
}

OrganismIdx Population::tournamentSelect(int nCandidates)
{
  OrganismIdx bestOrganismIdx = -1;
  int lowestCompletedRouteCost = INT_MAX;
  int nHighestCompletedRoutes = 0;

  for (int i = 0; i < nCandidates; ++i) {
    OrganismIdx organismIdx = randomOrganismSelector_.getRandomInt();

    auto& organism = organismVec[organismIdx];
    auto hasMoreCompletedRoutes = organism.nCompletedRoutes >
                                  nHighestCompletedRoutes;
    auto hasEqualRoutesAndLowerCost =
      organism.nCompletedRoutes == nHighestCompletedRoutes
      && organism.completedRouteCost < lowestCompletedRouteCost;
    if (hasMoreCompletedRoutes || hasEqualRoutesAndLowerCost) {
      bestOrganismIdx = organismIdx;
      lowestCompletedRouteCost = organism.completedRouteCost;
      nHighestCompletedRoutes = organism.nCompletedRoutes;
    }
  }
  return bestOrganismIdx;
}

double Population::getNormalizedRandom()
{
  static std::default_random_engine e;
  static std::uniform_real_distribution<> d(0.0, 1.0);
  return d(e);
}
