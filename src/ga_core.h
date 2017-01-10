#pragma once


#include <mutex>
#include <thread>
#include <vector>
#include <random>


//
// Random
//

typedef int GeneIdx;

class RandomIntGenerator
{
public:
  RandomIntGenerator();
  RandomIntGenerator(int min, int max);
  void setRange(int min, int max);
  int getRandomInt();
private:
  std::default_random_engine randomEngine_;
  std::uniform_int_distribution<> uniformIntDistribution_;
};

//
// Gene
//

typedef int Gene;
typedef std::vector<Gene> GeneVec;

class GeneDependency
{
public:
  GeneDependency(Gene gene, Gene geneDependency);

  Gene gene;
  Gene geneDependency;
};

//
// Organism
//

class Organism
{
public:
  Organism(int nGenes, RandomIntGenerator& randomGeneSelector);
  void createRandom();
  GeneIdx getRandomCrossoverPoint();
  void mutate();
  GeneVec calcConnectionIdxVec();
  void dump();

  int nCompletedRoutes;
  long completedRouteCost;

  GeneVec geneVec;
private:
  GeneVec topoSort();

  int nGenes_;
  RandomIntGenerator& randomGeneSelector_;
};

//
// Population
//

class OrganismPair
{
public:
  OrganismPair(Organism& a, Organism& b);
  Organism& a;
  Organism& b;
};

typedef int OrganismIdx;

typedef std::vector<Organism> OrganismVec;

class Population
{
public:
  Population(int nOrganismsInPopulation, double crossoverRate,
             double mutationRate);
  void reset(int nGenesPerOrganism);
  void nextGeneration();

  OrganismVec organismVec;
private:
  void createRandomPopulation();
  void crossover(OrganismPair& pair);
  OrganismPair selectPairTournament(int nCandidates);
  OrganismIdx tournamentSelect(int nCandidates);
  double getNormalizedRandom();

  int nOrganismsInPopulation_;
  double crossoverRate_;
  double mutationRate_;
  int nGenesPerOrganism_;
  RandomIntGenerator randomGeneSelector_;
  RandomIntGenerator randomOrganismSelector_;
};

