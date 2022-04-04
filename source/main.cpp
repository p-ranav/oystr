#include <limits>

#include <argparse.hpp>
#include <search.hpp>
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  argparse::ArgumentParser program("search", "0.1.0\n");
  program.add_argument("query");
  program.add_argument("path");
  program.add_argument("-c", "--count")
      .help("Print a count of matching lines for each input file.")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-i", "--ignore-case")
      .help(
          "Perform case insensitive matching.  By default, search is case "
          "sensitive.")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("--include")
      .help(
          "Only include files with extension. By default all files are "
          "searched")
      .default_value<std::vector<std::string>>({})
      .append();
  program.add_argument("--exclude")
      .help(
          "Skip any command-line file with a name suffix that matches the "
          "pattern")
      .default_value<std::vector<std::string>>({})
      .append();
  program.add_argument("-L", "--files-without-match")
      .help("Print only filenames of files that do not contain matches")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-l", "--files-with-matches")
      .help("Print only filenames of files that contain matches.")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("-m", "--max-count")
      .help("Stop reading a file after NUM matching lines.")
      .default_value(0)
      .required()
      .scan<'i', std::size_t>();

  program.add_argument("-n", "--line-number")
      .help(
          "Each output line is preceded by its relative line number in the "
          "file, starting at line 1.")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-o", "--only-matching")
      .help("Print only the matched (non-empty) parts of a matchiing line.")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-R", "-r", "--recursive")
      .help("Recursively search subdirectories listed.")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("-a", "--text")
      .help("Process a binary file as if it were text.")
      .default_value(false)
      .implicit_value(true);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  auto path = fs::path(program.get<std::string>("path"));
  auto query = program.get<std::string>("query");
  auto ignore_case = program.get<bool>("-i");
  auto print_count = program.get<bool>("-c");
  auto enforce_max_count = program.is_used("-m");
  std::size_t max_count = 0;
  if (enforce_max_count) {
    max_count = program.get<std::size_t>("-m");
  }
  auto print_only_file_matches = program.get<bool>("-l");
  auto print_only_file_without_matches = program.get<bool>("-L");
  auto print_line_numbers = program.get<bool>("-n");
  auto print_only_matching_parts = program.get<bool>("-o");
  auto recurse = program.get<bool>("-r");
  auto include_extension = program.get<std::vector<std::string>>("--include");
  auto exclude_extension = program.get<std::vector<std::string>>("--exclude");
  auto process_binary_file_as_text = program.get<bool>("-a");

  // File
  if (fs::is_regular_file(path)) {
    search::read_file_and_search(path.string(),
                                 query,
                                 {},
                                 {},
                                 ignore_case,
                                 print_count,
                                 enforce_max_count,
                                 max_count,
                                 print_line_numbers,
                                 print_only_file_matches,
                                 print_only_file_without_matches,
                                 print_only_matching_parts,
                                 process_binary_file_as_text);
  } else {
    // Directory
    if (recurse) {
      search::recursive_directory_search(path,
                                         query,
                                         include_extension,
                                         exclude_extension,
                                         ignore_case,
                                         print_count,
                                         enforce_max_count,
                                         max_count,
                                         print_line_numbers,
                                         print_only_file_matches,
                                         print_only_file_without_matches,
                                         print_only_matching_parts,
                                         process_binary_file_as_text);
    } else {
      search::directory_search(path,
                               query,
                               include_extension,
                               exclude_extension,
                               ignore_case,
                               print_count,
                               enforce_max_count,
                               max_count,
                               print_line_numbers,
                               print_only_file_matches,
                               print_only_file_without_matches,
                               print_only_matching_parts,
                               process_binary_file_as_text);
    }
  }
}
