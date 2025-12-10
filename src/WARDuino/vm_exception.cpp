#include "vm_exception.h"

#include <stdio.h>

#include <cstdarg>

char exception[VM_Exception_Size] = {'\0'};

int VM_Exception_write(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(exception, VM_Exception_Size, fmt, args);
    va_end(args);
    return n;
}

char* VM_Exception_get_exception() { return exception; }

bool VM_Exception_has_exception() { return exception[0] != '\0'; }
