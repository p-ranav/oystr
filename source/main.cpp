#include <limits>

#include <argparse.hpp>
#include <help.hpp>
#include <search.hpp>
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(NULL);
  argparse::ArgumentParser program("search", "0.1.0\n");
  program.add_argument("query");
  program.add_argument("path");

  // Generic Program Information
  program.add_argument("-h", "--help")
      .help("Shows help message and exits")
      .default_value(false)
      .implicit_value(true);

  // Generic Output Control
  program.add_argument("-c", "--count")
      .help("Print a count of matching lines for each input file.")
      .default_value(false)
      .implicit_value(true);

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

  // Files and Directory Selection
  program.add_argument("--exclude")
      .help(
          "Skip any command-line file with a name suffix that matches the "
          "pattern")
      .default_value<std::vector<std::string>>({})
      .append();

  program.add_argument("--include")
      .help(
          "Only include files with extension. By default all files are "
          "searched")
      .default_value<std::vector<std::string>>({})
      .append();

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    if (program.get<bool>("-h")) {
      search::print_help();
    } else {
      std::cerr << err.what() << std::endl;
      search::print_help();
    }
    std::exit(1);
  }

  auto path = fs::path(program.get<std::string>("path"));
  auto query = program.get<std::string>("query");
  auto print_count = program.get<bool>("-c");
  auto enforce_max_count = program.is_used("-m");
  std::size_t max_count = 0;
  if (enforce_max_count) {
    max_count = program.get<std::size_t>("-m");
  }
  auto print_only_file_matches = program.get<bool>("-l");
  auto print_only_file_without_matches = program.get<bool>("-L");
  auto include_extension = program.get<std::vector<std::string>>("--include");
  auto exclude_extension = program.get<std::vector<std::string>>("--exclude");

  // File
  if (fs::is_regular_file(path)) {
    auto count = search::read_file_and_search(path.string(),
                                              query,
                                              print_count,
                                              enforce_max_count,
                                              max_count,
                                              print_only_file_matches,
                                              print_only_file_without_matches);
    fmt::print("\n{} results\n", count);
  } else {
    auto count = search::directory_search(
        std::move(fs::directory_iterator(
            path, fs::directory_options::skip_permission_denied)),
        query,
        include_extension,
        exclude_extension,
        print_count,
        enforce_max_count,
        max_count,
        print_only_file_matches,
        print_only_file_without_matches);
    if (!print_only_file_matches && !print_only_file_without_matches) {
      fmt::print("\n{} results\n", count);
    }
  }
}
