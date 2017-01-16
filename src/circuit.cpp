#include "circuit.h"

// Components

Component::Component()
{
}

Component::Component(const std::string& _packageName, const Via& _pin0AbsPos)
  : packageName(_packageName), pin0AbsPos(_pin0AbsPos)
{
}

// Connections

ConnectionPoint::ConnectionPoint(const std::string& _componentName, int _pinIdx)
  : componentName(_componentName), pinIdx(_pinIdx)
{
}

Connection::Connection(
    const ConnectionPoint& _start, const ConnectionPoint& _end)
  : start(_start), end(_end)
{
}

// Circuit

Circuit::Circuit()
{
}

bool Circuit::hasParserError() const
{
  return parserErrorVec.size() > 0UL;
}

ConnectionViaVec Circuit::genConnectionViaVec() const
{
  ConnectionViaVec v;
  for (auto c : connectionVec) {
    auto startComponent = componentNameToComponentMap.at(c.start.componentName);
    auto endComponent = componentNameToComponentMap.at(c.end.componentName);

    auto startRelPin =
        packageToPosMap.at(startComponent.packageName).at(c.start.pinIdx);
    auto endRelPin =
        packageToPosMap.at(endComponent.packageName).at(c.end.pinIdx);

    Via startAbsPin = startRelPin + startComponent.pin0AbsPos;
    Via endAbsPin = endRelPin + endComponent.pin0AbsPos;

    v.push_back(StartEndVia(startAbsPin, endAbsPin));
  }
  return v;
}

StartEndVia Circuit::calcComponentFootprint(std::string componentName) const
{
  StartEndVia v(Via(INT_MAX, INT_MAX), Via(0, 0));
  auto component = componentNameToComponentMap.at(componentName);
  for (auto c : (packageToPosMap.at(component.packageName))) {
    c += component.pin0AbsPos;
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

PinViaVec Circuit::calcComponentPins(std::string componentName) const
{
  PinViaVec v;
  const Component component = componentNameToComponentMap.at(componentName);
  for (auto c : packageToPosMap.at(component.packageName)) {
    c += component.pin0AbsPos;
    v.push_back(c);
  }
  return v;
}
