#include "nets.h"

Nets::Nets(Solution &_solution)
  : solution_(_solution), setIdxVec_(_solution.setIdxVec), viaSetVec_(_solution.viaSetVec)
{
  setIdxVec_ = SetIdxVec(solution_.gridW * solution_.gridH, -1);
}

void Nets::joinEquivalent(const Via &viaA, const Via &viaB)
{
  int setIdxA = getViaSetIdx(viaA);
  int setIdxB = getViaSetIdx(viaB);
  assert(setIdxA < static_cast<int>(viaSetVec_.size()));
  assert(setIdxB < static_cast<int>(viaSetVec_.size()));

  if (setIdxA == -1 && setIdxB == -1) {
    auto viaSetIdx = createViaSet();
    viaSetVec_[viaSetIdx].insert(viaA);
    viaSetVec_[viaSetIdx].insert(viaB);
    setIdxVec_[solution_.idx(viaA)] = viaSetIdx;
    setIdxVec_[solution_.idx(viaB)] = viaSetIdx;
  }
  else if (setIdxA != -1 && setIdxB == -1) {
    auto &viaSet = viaSetVec_[setIdxA];
    viaSet.insert(viaB);
    setIdxVec_[solution_.idx(viaB)] = setIdxA;
  }
  else if (setIdxA == -1 && setIdxB != -1) {
    auto &viaSet = viaSetVec_[setIdxB];
    viaSet.insert(viaA);
    setIdxVec_[solution_.idx(viaA)] = setIdxB;
  }
  else {
    auto &viaSetA = viaSetVec_[setIdxA];
    auto &viaSetB = viaSetVec_[setIdxB];
    viaSetA.insert(viaSetB.begin(), viaSetB.end());
    for (auto &v : setIdxVec_) {
      if (v == setIdxB) {
        v = setIdxA;
      }
    }
  }
}

// Register a single via as an equivalent to itself to simplify later checking for equivalents.
void Nets::registerPin(const Via &via)
{
  int setIdx = getViaSetIdx(via);
  if (setIdx != -1) {
    auto &viaSet = viaSetVec_[setIdx];
    viaSet.insert(via);
  }
  else {
    auto viaSetIdx = createViaSet();
    viaSetVec_[viaSetIdx].insert(via);
    setIdxVec_[solution_.idx(via)] = viaSetIdx;
  }
}

void Nets::joinEquivalentRoute(const RouteStepVec &routeStepVec)
{
  for (auto c : routeStepVec) {
    // TODO: Equivalents don't keep track of layers, so skip steps where we only step through to the other layer.
    if (!c.isWireLayer) {
      joinEquivalent(routeStepVec[0].via, c.via);
    }
  }
}

int Nets::getViaSetIdx(const Via &via)
{
  int traceIdx = solution_.idx(via);
  return setIdxVec_[traceIdx];
}

int Nets::createViaSet()
{
  viaSetVec_.push_back(ViaSet([](const Via &a, const Via &b) -> bool
                              {
                                return std::lexicographical_compare(a.data(),
                                                                    a.data() + a.size(),
                                                                    b.data(),
                                                                    b.data() + b.size());
                              }));
  return static_cast<int>(viaSetVec_.size()) - 1;
}

ViaSet &Nets::getViaSet(const Via &via)
{
  int traceIdx = solution_.idx(via);
  int setIdx = setIdxVec_[traceIdx];
  assert(setIdx != -1);
  return viaSetVec_[setIdx];
}

bool Nets::isEquivalentVia(const Via &currentVia, const Via &targetVia)
{
  auto viaSetIdx = setIdxVec_[solution_.idx(currentVia)];
  if (viaSetIdx == -1) {
    return false;
  }
  bool r = static_cast<bool>(viaSetVec_[viaSetIdx].count(targetVia));
  return r;
}

bool Nets::hasEquivalent(const Via &via)
{
  auto viaSetIdx = setIdxVec_[solution_.idx(via)];
  return viaSetIdx != -1;
}
