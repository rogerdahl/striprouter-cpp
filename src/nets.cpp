#include "nets.h"

Nets::Nets(Layout& _layout)
  : layout_(_layout),
    setIdxVec_(_layout.setIdxVec),
    viaSetVec_(_layout.viaSetVec)
{
  setIdxVec_ = SetIdxVec(layout_.gridW * layout_.gridH, -1);
}

void Nets::connect(const Via& viaA, const Via& viaB)
{
  int setIdxA = getViaSetIdx(viaA);
  int setIdxB = getViaSetIdx(viaB);
  assert(setIdxA < static_cast<int>(viaSetVec_.size()));
  assert(setIdxB < static_cast<int>(viaSetVec_.size()));

  if (setIdxA == -1 && setIdxB == -1) {
    auto viaSetIdx = createViaSet();
    viaSetVec_[viaSetIdx].insert(viaA);
    viaSetVec_[viaSetIdx].insert(viaB);
    setIdxVec_[layout_.idx(viaA)] = viaSetIdx;
    setIdxVec_[layout_.idx(viaB)] = viaSetIdx;
  }
  else if (setIdxA != -1 && setIdxB == -1) {
    auto& viaSet = viaSetVec_[setIdxA];
    viaSet.insert(viaB);
    setIdxVec_[layout_.idx(viaB)] = setIdxA;
  }
  else if (setIdxA == -1 && setIdxB != -1) {
    auto& viaSet = viaSetVec_[setIdxB];
    viaSet.insert(viaA);
    setIdxVec_[layout_.idx(viaA)] = setIdxB;
  }
  else {
    auto& viaSetA = viaSetVec_[setIdxA];
    auto& viaSetB = viaSetVec_[setIdxB];
    viaSetA.insert(viaSetB.begin(), viaSetB.end());
    for (auto& v : setIdxVec_) {
      if (v == setIdxB) {
        v = setIdxA;
      }
    }
  }
}

void Nets::connectRoute(const RouteStepVec& routeStepVec)
{
  bool first = true;
  for (auto c : routeStepVec) {
    if (first) {
      first = false;
      continue;
    }
    if (!c.isWireLayer) {
      connect(routeStepVec[0].via, c.via);
    }
  }
  assert(isConnected(routeStepVec[0].via, routeStepVec[1].via));
}

// Register a single via as an equivalent to itself to simplify later checking
// for equivalents.
void Nets::registerPin(const Via& via)
{
  int setIdx = getViaSetIdx(via);
  if (setIdx != -1) {
    auto& viaSet = viaSetVec_[setIdx];
    viaSet.insert(via);
  }
  else {
    auto viaSetIdx = createViaSet();
    viaSetVec_[viaSetIdx].insert(via);
    setIdxVec_[layout_.idx(via)] = viaSetIdx;
  }
}

bool Nets::isConnected(const Via& currentVia, const Via& targetVia)
{
  auto viaSetIdx = setIdxVec_[layout_.idx(currentVia)];
  if (viaSetIdx == -1) {
    return false;
  }
  bool r = static_cast<bool>(viaSetVec_[viaSetIdx].count(targetVia));
  return r;
}

bool Nets::hasConnection(const Via& via)
{
  auto viaSetIdx = setIdxVec_[layout_.idx(via)];
  return viaSetIdx != -1;
}

ViaSet& Nets::getViaSet(const Via& via)
{
  int traceIdx = layout_.idx(via);
  int setIdx = setIdxVec_[traceIdx];
  assert(setIdx != -1);
  return viaSetVec_[setIdx];
}

//
// Private
//

int Nets::createViaSet()
{
  viaSetVec_.push_back(ViaSet([](const Via& a, const Via& b) -> bool {
    return std::lexicographical_compare(
        a.data(), a.data() + a.size(), b.data(), b.data() + b.size());
  }));
  return static_cast<int>(viaSetVec_.size()) - 1;
}

int Nets::getViaSetIdx(const Via& via)
{
  int traceIdx = layout_.idx(via);
  return setIdxVec_[traceIdx];
}
