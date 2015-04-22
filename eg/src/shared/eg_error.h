#pragma once

#ifndef EG_ERROR_H_INCLUDED
#define EG_ERROR_H_INCLUDED

extern char lastError[256];

void setError(const char *szError);

#endif /* EG_ERROR_H_INCLUDED */
