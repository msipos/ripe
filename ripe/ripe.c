// Copyright (C) 2008  Maksim Sipos <msipos@mailc.net>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "ripe/ripe.h"
#include <unistd.h>

// Prototypes
int compile_c(const char* in_filename, const char* out_filename);

// Global flags
const char* app_dir = NULL;
int verbose = 0;
const char* cflags = "";  // cflags are used when compiling to build .o
const char* lflags = "";  // lflags are used when compiling to build binary

////////////////////////////////////////////////////////////////////////////
// Modules
////////////////////////////////////////////////////////////////////////////

Array modules;
typedef struct {
  const char* module_name;
  const char* obj_filename;
} Module;

void module_init()
{
  array_init(&modules, Module*);
}

void module_add(const char* module_name, const char* obj_filename)
{
  if (verbose){
    fprintf(stderr, "module '%s' is at %s\n", module_name, obj_filename);
  }
  Module* module = mem_new(Module);
  module->module_name = module_name;
  module->obj_filename = obj_filename;
  
  // Attempt to load meta file
  char* meta_filename = mem_asprintf("%seta", obj_filename);
  meta_filename[strlen(meta_filename) - 4] = 'm';
  FILE* f = fopen(meta_filename, "r");
  if (f != NULL){
    fclose(f);
    Conf conf;
    conf_load(&conf, meta_filename);
    cflags = mem_asprintf("%s %s", cflags, conf_query(&conf, "includes"));
    lflags = mem_asprintf("%s %s", lflags, conf_query(&conf, "links"));
  }

  array_append(&modules, module);
}

const char* module_gcc()
{
  const char* gcc = "";
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    gcc = mem_asprintf("%s %s", gcc, module->obj_filename);
  }
  return gcc;
}

void module_build_loader(const char* loader_obj_filename)
{
  // First create C file
  const char* tmp = tmpnam(NULL);
  const char* loader_c_filename = mem_asprintf("%s.c", tmp);

  if (verbose){
    fprintf(stderr, "writing module loaders to '%s'\n",
            loader_c_filename);
  }

  FILE* f = fopen(loader_c_filename, "w");
  if (f == NULL){
    fprintf(stderr, "error: can't build loader object\n");
    fprintf(stderr, "while opening '%s' for writing: %s\n",
            loader_c_filename, strerror(errno));
    exit(1);
  }
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    fprintf(f, "void init1_%s();\n", module->module_name);
    fprintf(f, "void init2_%s();\n", module->module_name);
  }

  fprintf(f, "void ripe_module1(){\n");
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    fprintf(f, "init1_%s();\n", module->module_name);
  }
  fprintf(f, "}\n");  

  fprintf(f, "void ripe_module2(){\n");
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    fprintf(f, "init2_%s();\n", module->module_name);
  }
  fprintf(f, "}\n");
  fclose(f);
  
  // Now compile C file to object
  compile_c(loader_c_filename, loader_obj_filename);
}

////////////////////////////////////////////////////////////////////////////
// Builders
////////////////////////////////////////////////////////////////////////////

int compile_c(const char* in_filename, const char* out_filename)
{
  if (not path_exists(in_filename)) {
    fprintf(stderr, "cannot open '%s' for reading\n", in_filename);
    return 1;
  }

  char* cmd_line = mem_asprintf("gcc %s -c %s -o %s",
                                cflags, in_filename, out_filename);
  if (verbose) printf("%s\n", cmd_line);
  return system(cmd_line);
}

int compile_to_c(const char* in_filename, const char* module_name,
                 const char* out_filename)
{
  if (verbose) {
    printf("compiling '%s' to '%s'\n", in_filename, out_filename);
  }
  if (not path_exists(in_filename)) {
    fprintf(stderr, "cannot open '%s' for reading: %s\n", in_filename,
            strerror(errno));
    return 1;
  }
  Node* ast = build_tree(in_filename);
  if (ast == NULL){
    return 1;
  }
  if (generate(ast, module_name, in_filename, out_filename)){
    return 1;
  }
  return 0;
}

int compile_rip(const char* in_filename, const char* module_name, 
                const char* out_filename)
{
  const char* tmp = tmpnam(NULL);
  const char* c_filename = mem_asprintf("%s.c", tmp);
  
  if (c_filename == NULL){
    fprintf(stderr, "cannot create temporary filename: %s\n",
            strerror(errno));
    return 1;
  }
  compile_to_c(in_filename, module_name, c_filename);
  return compile_c(c_filename, out_filename);
}

int build(const char* in_filename, const char* out_filename)
{
  const char* tmp = tmpnam(NULL);
  const char* o_filename = mem_asprintf("%s.o", tmp);
  compile_rip(in_filename, "User", o_filename);
  module_add("User", o_filename);

  if (not path_exists(in_filename)) {
    fprintf(stderr, "cannot open '%s' for reading\n", in_filename);
    return 1;
  }

  // Module loader object
  tmp = tmpnam(NULL);
  const char* loader_filename = mem_asprintf("%s.o", tmp);
  module_build_loader(loader_filename);
  
  char* cmd_line = mem_asprintf("gcc %s %s %s %s/vm.o -o %s",
                                lflags, module_gcc(), loader_filename,
                                app_dir, out_filename);
  if (verbose) printf("%s\n", cmd_line);
  int rv = system(cmd_line);
  remove(loader_filename);
  return rv;
}

int dump_c(const char* in_filename, const char* module_name)
{
  const char* tmp = tmpnam(NULL);
  const char* tmp_filename = mem_asprintf("%s.c", tmp);
  if (compile_to_c(in_filename, module_name, tmp_filename))
    return 1;
  FILE* f = fopen(tmp_filename, "r");
  char line[1024];
  while ( fgets(line, 1024, f) ){
    printf("%s", line);
  }
  fclose(f);
  remove(tmp_filename);
  return 0;
}

int run(const char* in_filename)
{
  const char* p_filename = tmpnam(NULL);
  if (build(in_filename, p_filename)){
    return 1;  
  }
  return system(p_filename);  
}

int main(int argc, char* const* argv)
{
  const char* out_filename = NULL;
  const char* module_name = NULL;

  module_init();
  app_dir = path_get_app_dir();
  Conf conf;
  const char* conf_file = path_join(2, app_dir, "ripe.conf");
  conf_load(&conf, conf_file);
  cflags = conf_query(&conf, "cflags");
  lflags = conf_query(&conf, "lflags");
  if (strcmp(conf_query(&conf, "verbose"), "1") == 0 or
      strcmp(conf_query(&conf, "verbose"), "true") == 0 or
      strcmp(conf_query(&conf, "verbose"), "yes") == 0){
    verbose = 1;
  }
  
  // Extend cflags
  cflags = mem_asprintf("%s -I%s/include", cflags, app_dir);
  
  // Mode of operation
  #define MODE_MODULE 1
  #define MODE_BUILD  2
  #define MODE_RUN    3
  #define MODE_DUMP_C 4
  
  int mode = MODE_RUN;
  
  int c;
  while ((c = getopt(argc, argv, "bcdvo:n:m:f:")) != -1){
    switch(c){
      case 'v':
        verbose = 1;
        break;
      case 'b':
        mode = MODE_BUILD;
        break;
      case 'c':
        mode = MODE_MODULE;
        break;
      case 'd':
        mode = MODE_DUMP_C;
        break;
/*      case 'h':*/
/*        display_help();*/
/*        return 0;*/
      case 'o':
        out_filename = optarg;
        break;
      case 'n':
        module_name = optarg;
        break;
      case 'm':
        {
          const char* obj_path = mem_asprintf("%s/%s.o", app_dir, optarg);
          if (not path_exists(obj_path)){
            fprintf(stderr, "error: cannot find module '%s' (looked for %s)\n",
                    optarg, obj_path);
            exit(1);
          }
          module_add(mem_strdup(optarg),
                     obj_path);
        }
        break;
      case 'f':
        {
          cflags = mem_asprintf("%s %s", cflags, optarg);
        }
        break;
      default:
        return 1;
    }  
  }
  if (optind == argc){
    fprintf(stderr, "error: no source file given\n");
    return 1;
  }

  if (verbose){
    fprintf(stderr, "cflags = %s\n", cflags);
  }

  char* in_filename;
  switch(mode){
    case MODE_MODULE:
      if (out_filename == NULL){
        out_filename = "out.o";
      }
      if (argc - optind > 1){
        fprintf(stderr, "error: only 1 C or Ripe file supported");
        return 1;
      }
      if (module_name == NULL){
        module_name = "Module";
      }
      
      in_filename = argv[optind];
      const char* input_extension = path_get_extension(in_filename);
      if (strcmp(input_extension, ".rip") == 0){
        return compile_rip(in_filename, module_name, out_filename);
      } else if (strcmp(input_extension, ".c") == 0){
        return compile_c(in_filename, out_filename);
      } else {
        fprintf(stderr, "error: unknown extension: %s\n",
                input_extension);
        return 1;
      }
      break;
    case MODE_BUILD:
      if (out_filename == NULL){
        out_filename = "ripe.out";
      }
      if (argc - optind > 1){
        fprintf(stderr, "error: only 1 Ripe file supported (for now)");
        return 1;
      }
      in_filename = argv[optind];
      if (module_name != NULL){
        fprintf(stderr, "warning: module name ignored in build mode\n");
      }
      return build(in_filename, out_filename);

      break;
    case MODE_DUMP_C:
      if (out_filename != NULL){
        fprintf(stderr, "warning: output filename ignored in dump C mode\n");
      }
      if (argc - optind > 1){
        fprintf(stderr, "error: only 1 Ripe file supported (for now)");
        return 1;
      }
      if (module_name == NULL){
        module_name = "User";
      }
      in_filename = argv[optind];
      return dump_c(in_filename, module_name);
    case MODE_RUN:
      if (out_filename != NULL){
        fprintf(stderr, "warning: output filename ignored in run mode\n");
      }
      if (module_name != NULL){
        fprintf(stderr, "warning: module name ignored in run mode\n");
      }
      in_filename = argv[optind];
      return run(in_filename);
  }
}
