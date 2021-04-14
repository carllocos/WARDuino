#ifndef PRINTING_H
#define PRINTING_H

void wa_printf(const char* format, ...);
void wa_evprintf(const char* format, ...);
void wa_dbgprintf(const char* format, ...);
void wa_proxy(const void* buff, int count);
void wa_write(const void* buff, int count);
void wa_flush();
#endif