#pragma once

#define VM_Exception_Size 512

void VM_Exception_write(const char* fmt, ...);

char* VM_Exception_get_exception();

bool VM_Exception_has_exception();