#include <argparse.hpp>
#include <help.hpp>
#include <searcher.hpp>
#include <unistd.h>
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  const auto is_stdout = isatty(STDOUT_FILENO) == 1;
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(NULL);
  argparse::ArgumentParser program("search", "0.1.0\n");
  program.add_argument("query");
  program.add_argument("path").remaining();

  // Generic Program Information
  program.add_argument("-h", "--help")
      .help("Shows help message and exits")
      .default_value(false)
      .implicit_value(true);

  program.add_argument("-f", "--filter")
      .help("Only evaluate files that match filter pattern")
      .default_value(std::string {"*.*"});

  program.add_argument("-j")
      .help("Number of threads")
      .scan<'d', int>()
      .default_value(3);

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

  enum class file_option_t
  {
    none,
    single_file,
    single_directory,
    multiple
  };

  file_option_t file_option;

  std::vector<std::string> paths;
  try {
    paths = program.get<std::vector<std::string>>("path");
    auto size = paths.size();

    if (size == 1) {
      if (fs::is_regular_file(fs::path(paths[0]))) {
        file_option = file_option_t::single_file;
      } else {
        file_option = file_option_t::single_directory;
      }
    } else {
      file_option = file_option_t::multiple;
    }
  } catch (std::logic_error& e) {
    // No files provided
    file_option = file_option_t::none;
  }

  auto query = program.get<std::string>("query");
  auto filter = program.get<std::string>("-f");
  auto num_threads = program.get<int>("-j");
  auto print_count = program.get<bool>("-c");
  auto enforce_max_count = program.is_used("-m");
  std::size_t max_count = 0;
  if (enforce_max_count) {
    max_count = program.get<std::size_t>("-m");
  }
  auto print_only_file_matches = program.get<bool>("-l");
  auto print_only_file_without_matches = program.get<bool>("-L");

  // Configure a searcher
  search::searcher searcher;
  searcher.m_query = query;
  searcher.m_filter = filter;
  searcher.m_print_count = print_count;
  searcher.m_enforce_max_count = enforce_max_count;
  searcher.m_max_count = max_count;
  searcher.m_print_only_file_matches = print_only_file_matches;
  searcher.m_print_only_file_without_matches = print_only_file_without_matches;
  searcher.m_is_stdout = is_stdout;
  searcher.m_ts = std::make_unique<thread_pool>(num_threads);

  if (file_option == file_option_t::none) {
    searcher.directory_search(".");
  } else if (file_option == file_option_t::single_file) {
    searcher.read_file_and_search((const char*)paths[0].c_str());
  } else if (file_option == file_option_t::single_directory) {
    searcher.directory_search((const char*)paths[0].c_str());
  } else if (file_option == file_option_t::multiple) {
    for (const auto& path : paths) {
      if (fs::is_regular_file(fs::path(path))) {
        searcher.read_file_and_search((const char*)path.c_str());
      } else {
        searcher.directory_search((const char*)path.c_str());
      }
    }
  }
}
