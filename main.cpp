#include <search.hpp>
#include <argparse.hpp>

int main(int argc, char* argv[]) {

	argparse::ArgumentParser program("search");
	program.add_argument("path");
	program.add_argument("query");
	program.add_argument("-i")
		   .help("ignore case")
		   .default_value(false)
		   .implicit_value(true);
	program.add_argument("-n")
		   .help("show line numbers")
		   .default_value(false)
		   .implicit_value(true);
	program.add_argument("-r")
		   .help("recurse into subdirectories")
		   .default_value(false)
		   .implicit_value(true);

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err) {
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	auto root = fs::path(program.get<std::string>("path"));
	auto query = program.get<std::string>("query");
	auto ignore_case = program.get<bool>("-i");
	auto show_line_numbers = program.get<bool>("-n");
	auto recurse = program.get<bool>("-r");

	if (recurse)
	{
		search::recursive_directory_search(root, query, ignore_case, show_line_numbers);
	}
	else
	{
		search::directory_search(root, query, ignore_case, show_line_numbers);
	}

}