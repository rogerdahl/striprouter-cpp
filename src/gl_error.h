#pragma once

void _checkGlError(const char* file, int line);
#define checkGlError() _checkGlError(__FILE__,__LINE__)
