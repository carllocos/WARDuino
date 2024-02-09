#include "vm_exception.h"

#include <stdio.h>

#include <cstdarg>

char exception[VM_Exception_Size];

void VM_Exception_write(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sprintf(exception, fmt);
    va_end(args);
}

char* VM_Exception_get_exception() { return exception; }

bool VM_Exception_has_exception() { return exception[0] == '\0'; }