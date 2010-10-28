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

// This module should be cleaned up after compilation
#define FLAG_CLEANUP 1

Array modules;
typedef struct {
  int flag;
  const char* module_name;
  const char* obj_filename;
} Module;

void module_init()
{
  array_init(&modules, Module*);
}

void module_cleanup()
{
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    if (module->flag & FLAG_CLEANUP){
      remove(module->obj_filename);
    }
  }
}

void module_add(const char* module_name, const char* obj_filename, int flag)
{
  if (verbose){
    fprintf(stderr, "module '%s' is at %s\n", module_name, obj_filename);
  }

  // Check that this module is not loaded twice
  for (int i = 0; i < modules.size; i++){
    Module* m = array_get(&modules, Module*, i);
    if (strcmp(m->module_name, module_name) == 0) return;
  }

  Module* module = mem_new(Module);
  module->module_name = module_name;
  module->obj_filename = obj_filename;
  module->flag = flag;

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

void module_add_by_name(const char* module_name)
{
  const char* obj_path = mem_asprintf("%s/modules/%s/%s.o",
                                      app_dir,
                                      module_name,
                                      module_name);
  module_add(mem_strdup(module_name),
             obj_path, 0);
}

const char* module_gcc()
{
  const char* gcc = "";
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    if (not path_exists(module->obj_filename)){
      fprintf(stderr, "error: cannot find module '%s' (looked for %s)\n",
              module->module_name, module->obj_filename);
      exit(1);
    }
    gcc = mem_asprintf("%s %s", gcc, module->obj_filename);
  }
  return gcc;
}

void module_build_loader(const char* loader_obj_filename)
{
  // First create C file
  const char* tmp = tempnam(NULL, "ripe");
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
  fprintf(f, "void stack_push_annotation(char* annotation);\n");
  fprintf(f, "void stack_pop();\n");

  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    fprintf(f, "void init1_%s();\n", module->module_name);
    fprintf(f, "void init2_%s();\n", module->module_name);
  }

  fprintf(f, "void ripe_module1(){\n");
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    fprintf(f, "  stack_push_annotation(\"init1_%s\");\n", module->module_name);
    fprintf(f, "    init1_%s();\n", module->module_name);
    fprintf(f, "  stack_pop();\n");
  }
  fprintf(f, "}\n");

  fprintf(f, "void ripe_module2(){\n");
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    fprintf(f, "  stack_push_annotation(\"init2_%s\");\n", module->module_name);
    fprintf(f, "    init2_%s();\n", module->module_name);
    fprintf(f, "  stack_pop();\n");
  }
  fprintf(f, "}\n");
  fclose(f);

  // Now compile C file to object
  compile_c(loader_c_filename, loader_obj_filename);
  remove(loader_c_filename);
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
  if (system(cmd_line)){
    return 1;
  }
  return 0;
}

int compile_to_c(const char* in_filename, const char* module_name,
                 const char* out_filename)
{
  if (verbose) {
    printf("compiling '%s' to '%s'\n", in_filename, out_filename);
  }
  if (not path_exists(in_filename)) {
    fprintf(stderr, "cannot open '%s' for reading\n", in_filename);
    return 1;
  }
  Node* ast = build_tree(in_filename);
  if (ast == NULL){
    fprintf(stderr, "error while parsing '%s'\n", in_filename);
    return 1;
  }

  dump_init();
  if (generate(ast, module_name, in_filename)){
    fprintf(stderr, "error while generating '%s'\n", in_filename);
    return 1;
  }

  // Output handling.
  FILE* f = fopen(out_filename, "w");
  if (f == NULL){
    fprintf(stderr, "cannot open '%s' for writing: %s\n",
            out_filename, strerror(errno));
    return 1;
  }
  dump_output(f, module_name);
  fclose(f);

  return 0;
}

int compile_rip(const char* in_filename, const char* module_name,
                const char* out_filename)
{
  const char* tmp = tempnam(NULL, "ripe");
  const char* c_filename = mem_asprintf("%s.c", tmp);

  if (compile_to_c(in_filename, module_name, c_filename)){
    remove(c_filename);
    return 1;
  }
  if (compile_c(c_filename, out_filename)){
    remove(c_filename);
    return 1;
  }
  remove(c_filename);
  return 0;
}

void add_ripe_source(const char* in_filename)
{
  if (not path_exists(in_filename)) {
    fprintf(stderr, "error: cannot open '%s' for reading\n", in_filename);
    exit(1);
  }

  const char* tmp = tempnam(NULL, "ripe");
  const char* o_filename = mem_asprintf("%s.o", tmp);

  static int counter = 0;
  counter++;
  const char* module_name = mem_asprintf("_User%d", counter);

  compile_rip(in_filename, module_name, o_filename);
  module_add(module_name, o_filename, FLAG_CLEANUP);
}

int build(const char* out_filename)
{
  // Module loader object
  const char* tmp = tempnam(NULL, "ripe");
  const char* loader_filename = mem_asprintf("%s.o", tmp);
  module_build_loader(loader_filename);

  char* cmd_line = mem_asprintf("gcc %s %s %s %s/vm.o -o %s",
                                lflags, module_gcc(), loader_filename,
                                app_dir, out_filename);
  if (verbose) printf("%s\n", cmd_line);
  int rv = system(cmd_line);
  remove(loader_filename);
  module_cleanup();
  return rv;
}

int dump_c(const char* in_filename, const char* module_name)
{
  const char* tmp = tempnam(NULL, "ripe");
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

int run()
{
  const char* tmp = tempnam(NULL, "ripe");
  const char* p_filename = tmp;
  if (build(p_filename)){
    remove(p_filename);
    return 1;
  }
  int rv = system(p_filename);
  remove(p_filename);
  return rv;
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
  const char* modules = conf_query(&conf, "modules");

  // Load default modules: split modules string into words
  {
    char* ms = mem_strdup(modules);
    char* word = ms;
    while (*ms != 0)
    {
      if (*ms == ' ' or *ms == '\t'){
        *ms = 0;
        if (strlen(word) > 0) module_add_by_name(word);
        word = ms + 1;
      }
      ms++;
    }
    // Handle last word.
    if (strlen(word) > 0) module_add_by_name(word);
  }

  // Extend cflags
  cflags = mem_asprintf("%s -I%s/include", cflags, app_dir);

  // Mode of operation
  #define MODE_MODULE 1
  #define MODE_BUILD  2
  #define MODE_RUN    3
  #define MODE_DUMP_C 4
  #define MODE_DUMP_S 5

  int mode = MODE_RUN;

  int c;
  while ((c = getopt(argc, argv, "bcdvo:n:m:f:s")) != -1){
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
      case 's':
        mode = MODE_DUMP_S;
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
        module_add_by_name(optarg);
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
      if (argc - optind != 1){
        fprintf(stderr, "error: must provide one source file");
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
      if (argc - optind < 1){
        fprintf(stderr, "error: must provide at least one source file");
        return 1;
      }
      for (int i = optind; i < argc; i++){
        in_filename = argv[i];
        add_ripe_source(in_filename);
      }
      if (module_name != NULL){
        fprintf(stderr, "warning: module name ignored in build mode\n");
      }
      return build(out_filename);

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
    case MODE_DUMP_S:
      if (out_filename != NULL){
        fprintf(stderr, "warning: output filename ignored in dump ast mode\n");
      }
      if (argc - optind > 1){
        fprintf(stderr, "error: only 1 Ripe file supported (for now)");
        return 1;
      }
      if (module_name != NULL){
        fprintf(stderr, "warning: module name ignored in dump ast mode\n");
      }
      {
        Node* ast = build_tree(argv[optind]);
        node_draw(ast);
      }
      break;
    case MODE_RUN:
      if (out_filename != NULL){
        fprintf(stderr, "warning: output filename ignored in run mode\n");
      }
      if (module_name != NULL){
        fprintf(stderr, "warning: module name ignored in run mode\n");
      }
      if (argc - optind < 1){
        fprintf(stderr, "error: must provide at least one source file");
        return 1;
      }
      for (int i = optind; i < argc; i++){
        in_filename = argv[i];
        add_ripe_source(in_filename);
      }
      return run();
  }
}
