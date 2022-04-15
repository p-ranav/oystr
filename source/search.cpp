#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#  include <posix_iterate.hpp>
#endif

#include <search.hpp>
namespace fs = std::filesystem;

namespace search
{
// bool filename_has_extension_from_list(
//     const char* fext,
//     const std::vector<std::string>& extensions)
// {
//   bool result = false;

//   bool all_extensions = extensions.size() == 0;
//   if (!all_extensions) {
//     for (const auto& ext : extensions) {
//       if (strcmp((const char*)fext, ext.data()) == 0) {
//         result = true;
//         break;
//       }
//     }
//   } else {
//     result = false;
//   }

//   return result;
// }

std::size_t read_file_and_search(const char* path,
                                 std::string_view needle,
                                 bool print_count,
                                 bool enforce_max_count,
                                 std::size_t max_count,
                                 bool print_only_file_matches,
                                 bool print_only_file_without_matches)
{
  try {
    auto mmap = mio::mmap_source(path);
    if (!mmap.is_open() || !mmap.is_mapped()) {
      return 0;
    }
    const std::string_view haystack(mmap.data(), mmap.size());

    return file_search(path,
                       haystack,
                       needle,
                       print_count,
                       enforce_max_count,
                       max_count,
                       print_only_file_matches,
                       print_only_file_without_matches);
  } catch (const std::exception& e) {
  }
  return 0;
}

bool has_one_of_suffixes(const std::string_view& str)
{
  static const std::unordered_set<std::string_view> suffixes {
      "~",
      ".dll",
      ".exe",
      ".o",
      ".so",
      ".dmg",
      ".7z",
      ".dmg",
      ".gz",
      ".iso",
      ".jar",
      ".rar",
      ".tar",
      ".zip",
      ".sql",
      ".sqlite",
      ".sys",
      ".tiff",
      ".tif",
      ".bmp",
      ".jpg",
      ".jpeg",
      ".gif",
      ".png",
      ".eps",
      ".raw",
      ".cr2",
      ".crw",
      ".pef",
      ".nef",
      ".orf",
      ".sr2",
      ".pdf",
      ".psd",
      ".ai",
      ".indd",
      ".arc",
      ".meta",
      ".pdb",
      ".pyc",
      ".Spotlight-V100",
      ".Trashes",
      "ehthumbs.db",
      "Thumbs.db",
      ".suo",
      ".user",
      ".lst",
      ".pt",
      ".pak",
      ".qml",
      ".ttf",
      ".html",
      "appveyor.yml",
      ".ply",
      ".FBX",
      ".fbx",
      ".uasset",
      ".umap",
      ".rc2.res",
      ".bin",
      ".d",
      ".gch",
      ".orig",
      ".project",
      ".workspace",
      ".idea",
      ".epf",
      "sdkconfig",
      "sdkconfig.old",
      "personal.mak",
      ".userosscache",
      ".sln.docstates",
      ".a",
      ".bin",
      ".bz2",
      ".dt.yaml",
      ".dtb",
      ".dtbo",
      ".dtb.S",
      ".dwo",
      ".elf",
      ".gcno",
      ".gz",
      ".i",
      ".ko",
      ".lex.c",
      ".ll",
      ".lst",
      ".lz4",
      ".lzma",
      ".lzo",
      ".mod",
      ".mod.c",
      ".o",
      ".patch",
      ".s",
      ".so",
      ".so.dbg",
      ".su",
      ".symtypes",
      ".symversions",
      ".tar",
      ".xz",
      ".zst",
      "extra_certificates",
      ".pem",
      ".priv",
      ".x509",
      ".genkey",
      ".kdev4",
      ".defaults",
      ".projbuild",
      ".mk"};

  return suffixes.find(str) != suffixes.end();
}

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

int print_entry(const char* filepath,
                const struct stat* info,
                const int typeflag,
                struct FTW* pathinfo)
{
  auto filepath_size = strlen(filepath);
  auto filepath_view = std::string_view(filepath, filepath_size);

  // Check if file extension is in `include_extension` list
  // Check if file extension is NOT in `exclude_extension` list
  if ((include_extension.empty()
       || has_one_of_suffixes(filepath_view, include_extension))
      && (exclude_extension.empty()
          || !has_one_of_suffixes(filepath_view, exclude_extension)))
  {
    /* const char *const filename = filepath + pathinfo->base; */
    const double bytes = (double)info->st_size; /* Not exact if large! */
    struct tm mtime;

    localtime_r(&(info->st_mtime), &mtime);

    printf("%04d-%02d-%02d %02d:%02d:%02d",
           mtime.tm_year + 1900,
           mtime.tm_mon + 1,
           mtime.tm_mday,
           mtime.tm_hour,
           mtime.tm_min,
           mtime.tm_sec);

    if (bytes >= 1099511627776.0)
      printf(" %9.3f TiB", bytes / 1099511627776.0);
    else if (bytes >= 1073741824.0)
      printf(" %9.3f GiB", bytes / 1073741824.0);
    else if (bytes >= 1048576.0)
      printf(" %9.3f MiB", bytes / 1048576.0);
    else if (bytes >= 1024.0)
      printf(" %9.3f KiB", bytes / 1024.0);
    else
      printf(" %9.0f B  ", bytes);

    if (typeflag == FTW_SL) {
      char* target;
      size_t maxlen = 1023;
      ssize_t len;

      while (1) {
        target = (char*)malloc(maxlen + 1);
        if (target == NULL)
          return ENOMEM;

        len = readlink(filepath, target, maxlen);
        if (len == (ssize_t)-1) {
          const int saved_errno = errno;
          free(target);
          return saved_errno;
        }
        if (len >= (ssize_t)maxlen) {
          free(target);
          maxlen += 1024;
          continue;
        }

        target[len] = '\0';
        break;
      }

      printf(" %s -> %s\n", filepath, target);
      free(target);

      read_file_and_search(filepath,
                           query,
                           print_count,
                           enforce_max_count,
                           max_count,
                           print_only_file_matches,
                           print_only_file_without_matches);
    } else if (typeflag == FTW_SLN)
      printf(" %s (dangling symlink)\n", filepath);
    else if (typeflag == FTW_F)
      printf(" %s\n", filepath);
    else if (typeflag == FTW_D || typeflag == FTW_DP)
      printf(" %s/\n", filepath);
    else if (typeflag == FTW_DNR)
      printf(" %s/ (unreadable)\n", filepath);
    else
      printf(" %s (unknown)\n", filepath);
  }

  return 0;
}

std::size_t directory_search_posix(
    const char* path,
    std::string_view query,
    const std::vector<std::string>& include_extension,
    const std::vector<std::string>& exclude_extension,
    bool print_count,
    bool enforce_max_count,
    std::size_t max_count,
    bool print_only_file_matches,
    bool print_only_file_without_matches)
{
  /* Invalid directory path? */
  if (path == NULL || *path == '\0')
    return 0;

  nftw(path, handle_posix_directory_entry, USE_FDS, FTW_PHYS);
}

#endif

void directory_search(const char* path,
                      std::string_view query,
                      const std::vector<std::string>& include_extension,
                      const std::vector<std::string>& exclude_extension,
                      bool print_count,
                      bool enforce_max_count,
                      std::size_t max_count,
                      bool print_only_file_matches,
                      bool print_only_file_without_matches)
{
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
  directory_search_posix(path,
                         query,
                         include_extension,
                         exclude_extension,
                         print_count,
                         enforce_max_count,
                         max_count,
                         print_only_file_matches,
                         print_only_file_without_matches);
#endif

  /*
  const auto query_size = query.size();
  std::size_t count = 0;

  for (auto const& dir_entry : std::filesystem::directory_iterator(
                                                                   path,
  std::filesystem::directory_options::skip_permission_denied)) { try { if
  (std::filesystem::is_regular_file(dir_entry)) { const auto& dir_path =
  dir_entry.path(); const char* path_cstr = (const char*)dir_path.c_str();
        std::string_view path_string {path_cstr, strlen(path_cstr)};

        // Ignore dot files
        if (dir_path.filename().c_str()[0] == '.') {
          continue;
        }

        // Ignore files without an extension
        if (!dir_path.has_extension()) {
          continue;
        }

        // Ignore large files
        const auto file_size = std::filesystem::file_size(dir_path);
        if (file_size < query_size || file_size > 400 * 1024) {
          // Ignore files larger than 400 kB
          //
          // Only search text files in source code
          //   Unless they are large JSON/CSV files, single files in source code
          //   are unlikely to be this large (although it does happen often)
          //
          // TODO(pranav): Provide a commandline argument to change this amount
          // fmt::print(fg(fmt::color::yellow), "Skipping large file {}\n",
          // path_string);
          continue;
        }

        // Ignore files without extension, they are unlikely to be source code
        if ((include_extension.empty() && has_one_of_suffixes(path_string))) {
          // fmt::print(fg(fmt::color::yellow), "Skipping {}\n", path_string);
          continue;
        }

        // Check if file extension is in `include_extension` list
        // Check if file extension is NOT in `exclude_extension` list
        if ((include_extension.empty()
             || filename_has_extension_from_list(dir_path.extension(),
                                                 include_extension))
            && (exclude_extension.empty()
                || !filename_has_extension_from_list(dir_path.extension(),
                                                     exclude_extension)))
        {
          count += read_file_and_search(dir_path,
                                        query,
                                        print_count,
                                        enforce_max_count,
                                        max_count,
                                        print_only_file_matches,
                                        print_only_file_without_matches);
        }
      } else {
        const auto& dir_path = dir_entry.path();
        const char* path_cstr = (const char*)dir_path.c_str();
        std::string_view path_string {path_cstr, strlen(path_cstr)};
        const auto filename = dir_path.filename();
        const char* filename_cstr = (const char*)filename.c_str();

        if (strlen(filename_cstr) > 0 && filename_cstr[0] == '.') {
          // dot directory, ignore it
          continue;
        }

        static const std::unordered_set<const char*> ignored_dirs = {
            ".git",
            ".github",
            "build",
            "node_modules",
            ".vscode",
            ".DS_Store",
            "debugPublic",
            "DebugPublic",
            "debug",
            "Debug",
            "Release",
            "release",
            "Releases",
            "releases",
            "cmake-build-debug",
            "__pycache__",
            "Binaries",
            "Doc",
            "doc",
            "Documentation",
            "docs",
            "Docs",
            "bin",
            "Bin",
            "patches",
            "tar-install",
            "CMakeFiles",
            "install",
            "snap",
            "LICENSES",
            "img",
            "images",
            "imgs",
            // Unreal Engine
            "Plugins",
            "Content"};

        bool ignore = false;
        for (const auto& d : ignored_dirs) {
          if (strcmp(filename_cstr, d) == 0) {
            ignore = true;
            break;
          }
        }
        if (ignore) {
          continue;
        }

        // fmt::print("Checking {}\n", path_string);

        count += search::directory_search(
            dir_path,
            query,
            include_extension,
            exclude_extension,
            print_count,
            enforce_max_count,
            max_count,
            print_only_file_matches,
            print_only_file_without_matches);
      }
    } catch (std::exception& e) {
      continue;
    }
  }
  */
}
* /

}  // namespace search
