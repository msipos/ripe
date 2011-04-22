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

void bootstrap(const char* out_filename, int arg1, int argc, char* const* argv)
{
  logging("starting bootstrap (%d input files)...", argc - optind);
  Array asts, objs;
  array_init(&asts, Node*);
  array_init(&objs, const char*);

  for (int i = arg1; i < argc; i++){
    const char* arg = argv[i];
    const char* ext = path_get_extension(arg);
    if (strequal(ext, ".rip")){
      RipeInput input;
      if (input_from_file(&input, arg)){
        err("failed to load '%s'", arg);
      }
      Node* ast = build_tree(&input);
      if (ast == NULL){
        err("failed to parse '%s': %s", arg, build_tree_error);
      }
      array_append(&asts, ast);
      if (stran_absorb_ast(ast)){
        err("during structure analysis of '%s': %s", arg, stran_error->text);
      }
    } else if (strequal(ext, ".o")) {
      array_append(&objs, arg);
    } else if (strequal(ext, ".meta")) {
      Conf conf;
      conf_load(&conf, arg);
      cflags = mem_asprintf("%s %s", cflags, conf_query(&conf, "cflags"));
      lflags = mem_asprintf("%s %s", lflags, conf_query(&conf, "lflags"));
    } else {
      err("invalid extension '%s' of file '%s'", ext, arg);
    }
  }

  // Now generate ASTs into dump objects.
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
  fprintf(f, "%s", wr_dump("User"));
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
  remove(tmp_c_path);

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
  remove(tmp_o_path);
}


int main(int argc, char* const* argv)
{
  const char* out_filename = NULL;
  cflags = "";
  lflags = "";
  mem_init();
  stran_init();
  wr_init();

  {
    int c;
    while ((c = getopt(argc, argv, "o:s")) != -1){
      switch(c){
        case 'o':
          out_filename = optarg;
          break;
        case 's':
          /* ignore */
          break;
        default:
          err("getopt failed");
          return 1;
      }
    }
  }

  bootstrap(out_filename, optind, argc, argv);
}
