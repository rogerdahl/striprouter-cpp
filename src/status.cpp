#include <climits>

#include "status.h"

using namespace std;

mutex statusMutex;

Status::Status()
: bestCost(INT_MAX), nChecked(0)
{}
