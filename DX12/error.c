#include "error.h"

#include <stdlib.h>


void show_error_if_failed(HRESULT hr)
{
        if (FAILED(hr)) {
                #define MSG_LENGTH 265
                char error_message[MSG_LENGTH];
                memset(error_message, 0, MSG_LENGTH);
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        error_message, MSG_LENGTH - 1, NULL);
                MessageBox(NULL, error_message, "Error", MB_OK);
                exit(EXIT_FAILURE);
        }
}