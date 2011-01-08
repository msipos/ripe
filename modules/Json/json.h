#ifndef JSON_H
#define JSON_H

#define JSON_ERROR_INVALID_TOKEN 1
#define JSON_ERROR_SYNTAX        2
Value json_parse(char* input, int* error, char** out);

#endif
