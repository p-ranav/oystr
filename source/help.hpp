#pragma once
#include <fmt/core.h>

#define __PRINT(msg) fmt::print("{}", msg);

namespace search
{
void print_help()
{
  __PRINT("NAME\n");
  __PRINT("       oystr - print lines that match substring\n\n");
  __PRINT("SYNOPSIS\n");
  __PRINT("       oy [OPTION...] ");
  __PRINT("PATTERNS ");
  __PRINT("[FILE|DIRECTORY]\n\n");
  __PRINT("DESCRIPTION\n");
  __PRINT(
      "       Search plain-text data sets for lines that match a "
      "substring.\n\n");
  __PRINT("OPTIONS\n");
  __PRINT("   Generic Program Information\n");
  __PRINT("      --help Output a usage message and exit.\n\n");
  __PRINT("      -v, --version\n");
  __PRINT("             Output the version number of oystr and exit.\n\n");
  __PRINT("   Generic Output Control\n");
  __PRINT("      -c, --count\n");
  __PRINT(
      "             Suppress normal output; instead print a count of matching "
      "\n"
      "             lines for each input file.\n\n");
  __PRINT("      -L, --files-without-match\n");
  __PRINT(
      "             Suppress normal output; instead print the name of each "
      "input\n"
      "             file from which no output would normally have been "
      "printed.\n"
      "             The scanning will stop on the first match.\n\n");
  __PRINT("      -l, --files-with-matches\n");
  __PRINT(
      "             Suppress normal output; instead print the name of each "
      "input\n"
      "             file from which output would normally have been printed.\n"
      "             The scanning will stop on the first match.\n\n");
  __PRINT(
      "      -m NUM, --max-count NUM\n"
      "             Stop reading a file after NUM matching lines. When the -c "
      "or \n"
      "             --count option is also used, oystr does not output a "
      "count \n"
      "             greater than NUM.\n\n");
  __PRINT("   File and Directory Selection\n");
  __PRINT("      --exclude EXT\n");
  __PRINT("             Skip any command-line file with extension EXT.\n\n");
  __PRINT(
      "      --include EXT\n"
      "             Search only files with extension EXT.\n\n");
}

}  // namespace search
