#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <iso646.h>
#include <ctype.h>

#define L_INTEGER 1
#define L_DOUBLE  2
#define L_STRING  3
#define L_TRUE    4
#define L_FALSE   5
#define L_NULL    6
#define L_EOF     7

typedef struct {
  int a;
  int b;
} json_string_info;

typedef union {
  json_string_info s;
  int i;
  double d;
} json_token_info;

static void json_error(int error) ATTR_NORETURN;

static void json_error(int error)
{
  fprintf(stderr, "json_error: %d\n", error);
  exit(1);
}

static int json_lex(char* input, int* cur, json_token_info* tok_info)
{
  for(;;){
    int n = *cur;
    char c = input[n];
    switch (c){
      case '\n':
      case ' ':
      case '\t':
        (*cur)++;
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-':
        {
          int start = n;
          int is_double = 0;
          for(;;){
            (*cur)++;
            n = *cur;
            c = input[n];
            if (isdigit(c) or c=='-' or c=='+') continue;
            if (c=='E' or c=='e' or c=='.'){
              is_double = 1;
              continue;
            }

            if (is_double) {
              tok_info->d = atof(input+start);
              return L_DOUBLE;
            }
            tok_info->i = atoi(input+start);
            return L_INTEGER;
          }
        }

      case '"':
        {
          (*cur)++;
          tok_info->s.a = *cur;
          for(;;){
            n = *cur;
            c = input[n];
            if (c == '"'){
              tok_info->s.b = n - 1;
              (*cur)++;
              return L_STRING;
            }
            (*cur)++;
          }
        }
      case '{':
      case '}':
      case '[':
      case ']':
      case ':':
      case ',':
        (*cur)++;
        return c;

      case 't':
        if (input[n+1]=='r' and input[n+2]=='u' and input[n+3]=='e'){
          (*cur) += 4;
          return L_TRUE;
        }
        json_error(JSON_ERROR_INVALID_TOKEN);

      case 'f':
        if (input[n+1]=='a' and input[n+2]=='l' and input[n+3]=='s'
            and input[n+4]=='e'){
          (*cur) += 5;
          return L_FALSE;
        }
        json_error(JSON_ERROR_INVALID_TOKEN);

      case 'n':
        if (input[n+1]=='u' and input[n+2]=='l' and input[n+3]=='l'){
          (*cur) += 4;
          return L_NULL;
        }

      case 0:
        return L_EOF;

      default:
        json_error(JSON_ERROR_INVALID_TOKEN);
    }
  }
}

void json_parse2(char* input)
{
  int c = 0;
  json_token_info tok_info;
  int tok;

  while((tok = json_lex(input, &c, &tok_info)) != L_EOF){
    switch(tok){
      case L_INTEGER:
        printf("INTEGER %d", tok_info.i);
        break;
      case L_DOUBLE:
        printf("DOUBLE %g", tok_info.d);
        break;
      case L_TRUE:
        printf("TRUE");
        break;
      case L_FALSE:
        printf("FALSE");
        break;
      case L_NULL:
        printf("NULL");
        break;
      default:
        printf("SYMBOL %c", tok);
    }
    printf("\n");
  }
}

int peek_type(char* input, int c)
{
  json_token_info tok_info;
  return json_lex(input, &c, &tok_info);
}

Value json_parse_r(char* input, int* c)
{
  json_token_info tok_info;
  int tok = json_lex(input, c, &tok_info);
  switch(tok){
    case L_INTEGER:
      return int64_to_val(tok_info.i);
    case L_DOUBLE:
      return double_to_val(tok_info.d);
    case L_TRUE:
      return VALUE_TRUE;
    case L_FALSE:
      return VALUE_FALSE;
    case L_NULL:
      return VALUE_NIL;
    case L_STRING:
      return stringn_to_val(input + tok_info.s.a,
                            tok_info.s.b - tok_info.s.a + 1);
    case '[':
      {
        Value __array = array1_new(0);
        tok = peek_type(input, *c);
        if (tok == ']') {
          json_lex(input, c, &tok_info);
          return __array;
        }
        Array1* array = val_to_array1(__array);
        for(;;){
          array1_push(array, json_parse_r(input, c));
          tok = peek_type(input, *c);
          if (tok == ']') {
            json_lex(input, c, &tok_info);
            return __array;
          } else if (tok == ',') {
            json_lex(input, c, &tok_info);
          } else {
            json_error(JSON_ERROR_SYNTAX);
          }
        }
      }
    case '{':
      {
        HashTable* ht;
        Value __map = obj_new(klass_Map, (void**) &ht);
        ht_init2(ht);
        tok = peek_type(input, *c);
        if (tok == '}'){
          json_lex(input, c, &tok_info);
          return __map;
        }
        for(;;){
          Value key = json_parse_r(input, c);
          if (json_lex(input, c, &tok_info) != ':'){
            json_error(JSON_ERROR_SYNTAX);
          }
          Value val = json_parse_r(input, c);
          ht_set2(ht, key, val);

          tok = peek_type(input, *c);
          if (tok == '}'){
            json_lex(input, c, &tok_info);
            return __map;
          } else if (tok == ','){
            json_lex(input, c, &tok_info);
          } else {
            json_error(JSON_ERROR_SYNTAX);
          }
        }
      }
    default:
      json_error(JSON_ERROR_SYNTAX);
  }
}

Value json_parse(char* input, int c)
{
  return json_parse_r(input, &c);
}
