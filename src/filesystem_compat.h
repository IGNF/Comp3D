#ifndef FILESYSTEM_COMPAT_H
#define FILESYSTEM_COMPAT_H


#if defined(__GNUC__) && (__GNUC__ < 9)
#include <boost/filesystem.hpp>
namespace fs=boost::filesystem;
#define FS_COPY_OVERWRITE_EXISTING fs::copy_option::overwrite_if_exists
#else
#include <filesystem>
namespace fs=std::filesystem;
#define FS_COPY_OVERWRITE_EXISTING fs::copy_options::overwrite_existing
#endif

#endif // FILESYSTEM_COMPAT_H
