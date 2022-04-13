#pragma once
#include <iostream>

#include <termcolor.hpp>

#define __PRINT(msg) std::cout << msg;
#define __PRINT_INLINE_CYAN(msg) \
  std::cout << termcolor::cyan << termcolor::bold << msg << termcolor::reset;
#define __PRINT_INLINE_RED(msg) \
  std::cout << termcolor::red << termcolor::bold << msg << termcolor::reset;

namespace search
{
void print_help()
{
  __PRINT("NAME\n");
  __PRINT("       search - print lines that match substring\n\n");
  __PRINT("SYNOPSIS\n");
  __PRINT("       search [OPTION...] ");
  __PRINT_INLINE_RED("PATTERNS ");
  __PRINT_INLINE_CYAN("[FILE...]\n\n");
  __PRINT("DESCRIPTION\n");
  __PRINT(
      "       Search plain-text data sets for lines that match a "
      "substring.\n\n");
  __PRINT("OPTIONS\n");
  __PRINT("   Generic Program Information\n");
  __PRINT("      --help Output a usage message and exit.\n\n");
  __PRINT("      -v, --version\n");
  __PRINT("             Output the version number of search and exit.\n\n");
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
      "             --count option is also used, search does not output a "
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
