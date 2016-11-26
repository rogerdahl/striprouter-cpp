#include "circuit.h"

using namespace std;

mutex circuitMutex;


Component::Component()
{}


Component::Component(const string& _packageName, const Via& _pin0AbsCoord)
  : packageName(_packageName), pin0AbsCoord(_pin0AbsCoord)
{}


ConnectionPoint::ConnectionPoint(const string& _componentName, u32 _pinIdx)
  : componentName(_componentName), pinIdx(_pinIdx)
{}


Connection::Connection(const ConnectionPoint& _start, const ConnectionPoint& _end)
  : start(_start), end(_end)
{}


Circuit::Circuit()
  : hasError(false)
{}


ConnectionViaVec Circuit::genConnectionViaVec()
{
  ConnectionViaVec v;
  for (auto c : connectionVec) {
    auto startComponent = componentNameToInfoMap[c.start.componentName];
    auto endComponent = componentNameToInfoMap[c.end.componentName];

    auto startRelPin = packageToCoordMap[startComponent.packageName][c.start.pinIdx];
    auto endRelPin = packageToCoordMap[endComponent.packageName][c.end.pinIdx];

    Via startAbsPin = startRelPin + startComponent.pin0AbsCoord;
    Via endAbsPin = endRelPin + endComponent.pin0AbsCoord;

    startAbsPin = connectionFudge(c.start.componentName, startAbsPin);
    endAbsPin = connectionFudge(c.end.componentName, endAbsPin);
    
    v.push_back(ViaStartEnd(startAbsPin, endAbsPin));
  }
  return v;
}

// Move connections to the closest vertical point outside the package
// footprint.

// This is just a fudge for now, making things during routing since we
// block off the entire component footprint for traces.

Via Circuit::connectionFudge(const string& componentName, const Via& absPin)
{
  auto footprint = calcComponentFootprint(componentName);
  float halfY = footprint.start.y() + (footprint.end.y() - footprint.start.y()) / 2.0f;
  if (absPin.y() < halfY) {
    return Via(absPin - Via(0, 1));
  }
  else {
    return Via(absPin + Via(0, 1));
  }
}


PinViaVec Circuit::calcComponentPins(std::string componentName)
{
  PinViaVec v;
  auto component = componentNameToInfoMap[componentName];
  for (auto c : packageToCoordMap[component.packageName]) {
    c += component.pin0AbsCoord;
    v.push_back(c);
  }
  return v;
}


ViaStartEnd Circuit::calcComponentFootprint(string componentName)
{
  ViaStartEnd v(Via(INT_MAX, INT_MAX), Via(0,0));
  auto component = componentNameToInfoMap[componentName];
  for (auto c : packageToCoordMap[component.packageName]) {
    c += component.pin0AbsCoord;
    if (c.x() < v.start.x()) {
      v.start.x() = c.x();
    }
    if (c.x() > v.end.x()) {
      v.end.x() = c.x();
    }
    if (c.y() < v.start.y()) {
      v.start.y() = c.y();
    }
    if (c.y() > v.end.y()) {
      v.end.y() = c.y();
    }
  }
  return v;
}
