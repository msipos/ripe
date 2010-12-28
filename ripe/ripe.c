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

// Prototypes
int compile_c(const char* in_filename, const char* out_filename);

// Global flags
const char* app_dir = NULL;
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
  const char* type_path;
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

void module_add(const char* module_name, const char* obj_filename,
                const char* type_path, int flag)
{
  logging("module '%s' is at %s", module_name, obj_filename);

  // Check that this module is not loaded twice
  for (int i = 0; i < modules.size; i++){
    Module* m = array_get(&modules, Module*, i);
    if (strcmp(m->module_name, module_name) == 0) return;
  }

  Module* module = mem_new(Module);
  module->module_name = module_name;
  module->obj_filename = obj_filename;
  module->type_path = type_path;
  module->flag = flag;

  // Attempt to load meta file
  char* meta_filename = mem_asprintf("%seta", obj_filename);
  meta_filename[strlen(meta_filename) - 4] = 'm';
  FILE* f = fopen(meta_filename, "r");
  if (f != NULL){
    fclose(f);
    Conf conf;
    conf_load(&conf, meta_filename);
    cflags = mem_asprintf("%s %s", cflags, conf_query(&conf, "cflags"));
    lflags = mem_asprintf("%s %s", lflags, conf_query(&conf, "lflags"));
  }

  array_append(&modules, module);
}

void module_add_by_name(const char* module_name)
{
  const char* obj_path = mem_asprintf("%s/modules/%s/%s.o",
                                      app_dir,
                                      module_name,
                                      module_name);
  const char* type_path = mem_asprintf("%s/modules/%s/%s.typ",
                                       app_dir,
                                       module_name,
                                       module_name);

  module_add(mem_strdup(module_name), obj_path, type_path, 0);
}

const char* module_gcc()
{
  const char* gcc = "";
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    if (not path_exists(module->obj_filename)){
      err("cannot find module '%s' (looked for %s)",
          module->module_name, module->obj_filename);
    }
    gcc = mem_asprintf("%s %s", gcc, module->obj_filename);
  }
  return gcc;
}

void module_build_loader(const char* loader_obj_filename)
{
  // First create C file
  const char* loader_c_filename = path_temp_name("ripe", ".c");

  logging("writing module loaders to '%s'", loader_c_filename);

  FILE* f = fopen(loader_c_filename, "w");
  if (f == NULL) {
    err("can't open '%s' for writing: %s", loader_c_filename, strerror(errno));
  }

  fprintf(f, "void stack_push_annotation(char* annotation);\n");
  fprintf(f, "void stack_pop();\n");

  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    fprintf(f, "void init1_%s();\n", module->module_name);
    fprintf(f, "void init2_%s();\n", module->module_name);
    fprintf(f, "void init3_%s();\n", module->module_name);
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

  fprintf(f, "void ripe_module3(){\n");
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    fprintf(f, "  stack_push_annotation(\"init3_%s\");\n", module->module_name);
    fprintf(f, "    init3_%s();\n", module->module_name);
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
    err("cannot open '%s' for reading", in_filename);
  }

  char* cmd_line = mem_asprintf("gcc %s -c %s -o %s",
                                cflags, in_filename, out_filename);
  logging("%s", cmd_line);
  if (system(cmd_line)){
    return 1;
  }
  return 0;
}

void type_info_from_modules(const char* ignore)
{
  for (int i = 0; i < modules.size; i++){
    Module* module = array_get(&modules, Module*, i);
    if (ignore != NULL and strcmp(ignore, module->module_name) == 0){
      continue;
    }
    if (module->type_path != NULL){
      FILE* f = fopen(module->type_path, "r");
      if (f == NULL) err("can't open type file '%s' for reading: %s",
                         module->type_path,
                         strerror(errno));
      typer_load(f);
      fclose(f);
    }
  }
}

void type_info(const char* in_filename)
{
  Node* ast = build_tree(in_filename);
  if (ast == NULL) err("%s", build_tree_error);
  typer_init();
  typer_ast(ast);
  typer_dump(stdout);
}

int compile_to_c(int num_files, const char** in_filenames,
                 const char* module_name, const char* out_filename)
{
  dump_init();

  // Now for each input file
  for (int i = 0; i < num_files; i++){
    const char* in_filename = in_filenames[i];
    logging("compiling '%s' to '%s'", in_filename, out_filename);

    Node* ast = build_tree(in_filename);
    if (ast == NULL) err("%s", build_tree_error);
    generate(ast, module_name, in_filename);
  }

  // Output handling.
  FILE* f = fopen(out_filename, "w");
  if (f == NULL){
    err("cannot open '%s' for writing: %s\n", out_filename, strerror(errno));
  }
  dump_output(f, module_name);
  fclose(f);

  return 0;
}

int compile_rip(int num_files, const char** in_filenames,
                int num_o_files, const char** in_o_filenames,
                const char* module_name, const char* out_filename)
{
  const char* c_filename = path_temp_name("ripe", ".c");

  // First populate typer info
  for (int i = 0; i < num_files; i++){
    const char* in_path = in_filenames[i];
    Node* ast = build_tree(in_path);
    if (ast == NULL) err("%s", build_tree_error);
    typer_ast(ast);
  }

  compile_to_c(num_files, in_filenames, module_name, c_filename);
  if (num_o_files == 0){
    compile_c(c_filename, out_filename);
  } else {
    const char* o_filename = path_temp_name("ripe", ".o");
    compile_c(c_filename, o_filename);
    char* objs = mem_asprintf("%s", in_o_filenames[0]);
    for (int i = 1; i < num_o_files; i++){
      objs = mem_asprintf("%s %s", objs, in_o_filenames[i]);
    }
    char* cmd_line = mem_asprintf("ld -r %s %s -o %s",
                                  objs, o_filename, out_filename);
    logging(cmd_line);
    if (system(cmd_line)){
      err("while building module with objects");
    }
    remove(o_filename);
  }

  remove(c_filename);
  return 0;
}

int build(const char* out_filename)
{
  // Module loader object
  const char* loader_filename = path_temp_name("ripe", ".o");
  module_build_loader(loader_filename);

  char* cmd_line = mem_asprintf("gcc %s %s %s %s/vm.o -o %s",
                                lflags, module_gcc(), loader_filename,
                                app_dir, out_filename);
  logging("%s", cmd_line);
  int rv = system(cmd_line);
  remove(loader_filename);
  module_cleanup();
  return rv;
}

int dump_c(const char* in_filename, const char* module_name)
{
  Node* ast = build_tree(in_filename);
  if (ast == NULL) err("%s", build_tree_error);

  const char* tmp_filename = path_temp_name("ripe", ".c");
  if (compile_to_c(1, &in_filename, module_name, tmp_filename))
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
  const char* p_filename = path_temp_name("ripe", ".out");
  if (build(p_filename)){
    remove(p_filename);
    return 1;
  }
  int rv = system(p_filename);
  remove(p_filename);
  return rv;
}

void bootstrap(const char* out_filename, int optind, int argc, char* const* argv)
{
  logging("starting bootstrap (%d input files)...", argc - optind);
  Array asts, objs;
  array_init(&asts, Node*);
  array_init(&objs, const char*);
  typer_init();

  for (int i = optind; i < argc; i++){
    const char* arg = argv[i];
    const char* ext = path_get_extension(arg);
    if (strequal(ext, ".rip")){
      Node* ast = build_tree(arg);
      if (ast == NULL){
        err("failed to parse '%s': %s", arg, build_tree_error);
      }
      array_append(&asts, ast);
      typer_ast(ast);
    } else if (strequal(ext, ".o")) {
      array_append(&objs, arg);
    } else if (strequal(ext, ".meta")) {
      Conf conf;
      conf_load(&conf, arg);
      cflags = mem_asprintf("%s %s", cflags, conf_query(&conf, "cflags"));
      lflags = mem_asprintf("%s %s", lflags, conf_query(&conf, "lflags"));
    } else {
      err("invalid extension '%s'", ext);
    }
  }

  // Now generate ASTs into dump objects.
  dump_init();
  for (int i = 0; i < asts.size; i++){
    Node* ast = array_get(&asts, Node*, i);
    generate(ast, "User", "input");
  }

  // Now dump into C file.
  const char* tmp_c_path = path_temp_name("ripe_boot_", ".c");
  logging("dumping into '%s'...", tmp_c_path);
  FILE* f = fopen(tmp_c_path, "w");
  if (f == NULL){
    err("cannot open '%s' for writing: %s\n", tmp_c_path, strerror(errno));
  }
  dump_output(f, "User");
  fclose(f);

  // Compile into an object file.
  const char* tmp_o_path = path_temp_name("ripe_boot_", ".o");
  const char* cmd_line = mem_asprintf("gcc %s %s -c -o %s",
                                      cflags,
                                      tmp_c_path,
                                      tmp_o_path);
  if (system(cmd_line)){
    err("failed running '%s'", cmd_line);
  }

  // Finally compile into the output file
  char* objs_txt = "";
  for (int i = 0; i < objs.size; i++){
    const char* o_path = array_get(&objs, const char*, i);
    objs_txt = mem_asprintf("%s %s", objs_txt, o_path);
  }

  cmd_line = mem_asprintf("gcc %s %s %s -o %s",
                          lflags,
                          objs_txt,
                          tmp_o_path,
                          out_filename);
  if (system(cmd_line)){
    err("failed running '%s'", cmd_line);
  }
}


int main(int argc, char* const* argv)
{
  const char* out_filename = NULL;
  const char* module_name = NULL;
  cflags = "";
  lflags = "";

  // Mode of operation
  #define MODE_MODULE 1
  #define MODE_BUILD  2
  #define MODE_RUN    3
  #define MODE_DUMP_C 4
  #define MODE_DUMP_S 5
  #define MODE_TYPE   6
  #define MODE_DUMP_T 7
  #define MODE_BOOTSTRAP 8

  int mode = MODE_RUN;
  {
    int c;
    while ((c = getopt(argc, argv, "Xutbcdvo:n:m:f:s")) != -1){
      switch(c){
        case 'v':
          log_verbosity = 1;
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
        case 't':
          mode = MODE_TYPE;
          break;
        case 'u':
          mode = MODE_DUMP_T;
          break;
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
        case 'X':
          mode = MODE_BOOTSTRAP;
          break;
        default:
          err("getopt failed");
          return 1;
      }
    }
  }

  if (mode == MODE_BOOTSTRAP){
    bootstrap(out_filename, optind, argc, argv);
    return 0;
  }

  module_init();
  app_dir = path_get_app_dir();

  Conf conf;
  const char* conf_file = path_join(2, app_dir, "ripe.meta");
  conf_load(&conf, conf_file);
  cflags = conf_query(&conf, "cflags");
  lflags = conf_query(&conf, "lflags");
  if (strcmp(conf_query(&conf, "verbose"), "1") == 0 or
      strcmp(conf_query(&conf, "verbose"), "true") == 0 or
      strcmp(conf_query(&conf, "verbose"), "yes") == 0){
    log_verbosity = 1;
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

  logging("cflags = %s", cflags);

  if (mode == MODE_DUMP_T){
    typer_init();
    type_info_from_modules(NULL);
    typer_dump(stdout);
    return 0;
  }

  if (optind == argc) err("no source file given");

  // Time to load typer info. Only for particular modes that require it.
  if (mode == MODE_MODULE or mode == MODE_BUILD or mode == MODE_RUN
      or mode == MODE_DUMP_C){
    typer_init();
    type_info_from_modules(module_name);
  }

  char* in_filename;
  switch(mode){
    case MODE_MODULE:
      if (out_filename == NULL) out_filename = "out.o";
      if (module_name == NULL) module_name = "Module";

      Array rip_files, o_files;
      array_init(&rip_files, const char*);
      array_init(&o_files, const char*);

      for (int arg = optind; arg < argc; arg++){
        in_filename = argv[arg];
        const char* input_extension = path_get_extension(in_filename);
        if (strequal(input_extension, ".rip")){
          array_append(&rip_files, in_filename);
        } else if (strequal(input_extension, ".o")){
          array_append(&o_files, in_filename);
        } else {
          err("unknown extension: '%s'", input_extension);
        }
      }
      compile_rip(rip_files.size, (const char**) (rip_files.data),
                  o_files.size, (const char**) (o_files.data),
                  module_name, out_filename);
      return 0;
    case MODE_BUILD:
      {
        if (module_name != NULL) warn("module name ignored in build mode");
        if (out_filename == NULL){
          out_filename = "ripe.out";
        }

        const char* o_filename = path_temp_name("ripe", ".o");
        const char* module_name = "_User";
        compile_rip(argc - optind, (const char **) argv + optind, 0, NULL,
                    module_name, o_filename);
        module_add(module_name, o_filename, NULL, FLAG_CLEANUP);

        return build(out_filename);
      }
    case MODE_DUMP_C:
      if (argc - optind > 1) err("only 1 Ripe file supported");
      if (out_filename != NULL) warn("output filename ignored in dump C mode");

      if (module_name == NULL){
        module_name = "User";
      }
      in_filename = argv[optind];
      return dump_c(in_filename, module_name);
    case MODE_DUMP_S:
      if (argc - optind > 1) err("only 1 Ripe file supported");
      if (out_filename != NULL) warn("output filename ignored in dump ast mode");
      if (module_name != NULL) warn("module name ignored in dump ast mode");

      Node* ast = build_tree(argv[optind]);
      if (ast == NULL) err("%s", build_tree_error);
      node_draw(ast);
      break;
    case MODE_RUN:
      {
        if (out_filename != NULL) warn("output filename ignored in run mode");
        if (module_name != NULL) warn("module name ignored in run mode");

        const char* o_filename = path_temp_name("ripe", ".o");
        const char* module_name = "_User";
        compile_rip(argc - optind, (const char **) argv + optind, 0, NULL, module_name, o_filename);
        module_add(module_name, o_filename, NULL, FLAG_CLEANUP);
      }
      return run();
    case MODE_TYPE:
      type_info(argv[optind]);
      break;
  }
}
