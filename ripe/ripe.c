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

// Global flags
const char* cflags = "";
const char* lflags = "";

static void bootstrap(const char* out_filename, int arg1, int argc, char* const* argv)
{
  Array asts, objs, ast_filenames;
  array_init(&asts, Node*);
  array_init(&ast_filenames, const char*);
  array_init(&objs, const char*);

  for (int i = arg1; i < argc; i++){
    const char* arg = argv[i];
    const char* ext = path_get_extension(arg);
    if (strequal(ext, ".rip")){
      RipeInput input;
      fatal_push("while reading '%s'");
        input_from_file(&input, arg);
      fatal_pop();

      fatal_push("while building AST for '%s'", arg);
      Node* ast = build_tree(&input);
      fatal_pop();

      array_append(&asts, ast);
      array_append(&ast_filenames, arg);

      stran_absorb_ast(ast, arg);
    } else if (strequal(ext, ".o")) {
      array_append(&objs, arg);
    } else if (strequal(ext, ".meta")) {
      Conf conf;
      conf_load(&conf, arg);
      cflags = mem_asprintf("%s %s", cflags, conf_query(&conf, "cflags"));
      lflags = mem_asprintf("%s %s", lflags, conf_query(&conf, "lflags"));
    } else {
      fatal_throw("invalid extension '%s' of file '%s'", ext, arg);
    }
  }

  genist_run();

  for (uint i = 0; i < asts.size; i++){
    Node* ast = array_get(&asts, Node*, i);
    const char* arg = array_get(&ast_filenames, const char*, i);

    proc_process_ast(ast, arg);
  }

  // Now generate ASTs into dump objects.
  for (uint i = 0; i < asts.size; i++){
    Node* ast = array_get(&asts, Node*, i);
    const char* arg = array_get(&ast_filenames, const char*, i);

    fatal_push("while generating code in '%s'", arg);
    generate(ast, arg);
    fatal_pop();
  }

  // Now dump into C file.
  const char* tmp_c_path = path_temp_name("ripe_boot_", ".c");
  FILE* f = fopen(tmp_c_path, "w");
  if (f == NULL){
    fatal_throw("cannot open '%s' for writing: %s", tmp_c_path, strerror(errno));
  }
  fprintf(f, "%s", wr_dump("User"));
  fclose(f);

  // Compile into an object file.
  const char* tmp_o_path = path_temp_name("ripe_boot_", ".o");
  const char* cmd_line = mem_asprintf("gcc %s %s -c -o %s",
                                      cflags,
                                      tmp_c_path,
                                      tmp_o_path);
  if (system(cmd_line)){
    fatal_throw("failed running '%s'", cmd_line);
  }
  remove(tmp_c_path);

  // Finally compile into the output file
  const char* objs_txt = "";
  for (uint i = 0; i < objs.size; i++){
    const char* o_path = array_get(&objs, const char*, i);
    objs_txt = mem_asprintf("%s %s", objs_txt, o_path);
  }

  cmd_line = mem_asprintf("gcc %s %s %s -o %s",
                          lflags,
                          objs_txt,
                          tmp_o_path,
                          out_filename);
  if (system(cmd_line)){
    fatal_throw("failed running '%s'", cmd_line);
  }
  remove(tmp_o_path);
}


int main(int argc, char* const* argv)
{
  const char* out_filename = NULL;
  cflags = "";
  lflags = "";
  mem_init();
  lang_init();

  {
    int c;
    while ((c = getopt(argc, argv, "o:sv")) != -1){
      switch(c){
        case 'o':
          out_filename = optarg;
          break;
        case 's':
          /* ignore */
          break;
        default:
          fatal_throw("getopt failed");
          return 1;
      }
    }
  }

  bootstrap(out_filename, optind, argc, argv);
}
