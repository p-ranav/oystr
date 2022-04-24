#include <argparse.hpp>
#include <searcher.hpp>
#include <unistd.h>
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  const auto is_path_from_terminal = isatty(STDIN_FILENO) == 1;
  const auto is_stdout = isatty(STDOUT_FILENO) == 1;
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(NULL);
  argparse::ArgumentParser program("search", "0.2.0\n");
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
      .default_value(5);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
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
  if (is_path_from_terminal) {
    // Input arguments ARE paths to files or directories
    // Parse the arguments
    try {
      paths = program.get<std::vector<std::string>>("path");
      auto size = paths.size();

      if (size == 1) {
        if (fs::is_regular_file(fs::path(paths[0]))) {
          file_option = file_option_t::single_file;
        } else if (fs::is_directory(fs::path(paths[0]))) {
          file_option = file_option_t::single_directory;
        }
      } else {
        file_option = file_option_t::multiple;
      }
    } catch (std::logic_error& e) {
      // No files provided
      file_option = file_option_t::none;
    }
  }

  auto query = program.get<std::string>("query");
  auto filter = program.get<std::string>("-f");
  auto num_threads = program.get<int>("-j");

  // Configure a searcher
  search::searcher searcher;
  searcher.m_query = query;
  searcher.m_filter = filter;
  searcher.m_is_stdout = is_stdout;
  searcher.m_is_path_from_terminal = is_path_from_terminal;

  if (is_path_from_terminal) {
    searcher.m_ts = std::make_unique<thread_pool>(num_threads);
    // Input arguments ARE paths to files or directories
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
        } else if (fs::is_directory(fs::path(path))) {
          searcher.directory_search((const char*)path.c_str());
        } else {
          fmt::print(fmt::fg(fmt::color::red) | fmt::emphasis::bold,
                     "\nError: '{}' is not a valid file or directory\n",
                     path);
          std::exit(1);
        }
      }
    }
  } else {
    // Input is from pipe
    for (std::string line; std::getline(std::cin, line);) {
      if (!line.empty() && line.size() >= query.size()) {
        searcher.file_search("", line);
      }
    }
  }
}
