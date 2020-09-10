#ifndef ERROR_H
#define ERROR_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


void show_error_if_failed(HRESULT hr);

#endif