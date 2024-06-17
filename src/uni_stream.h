/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - C Meynard
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef UNI_STREAM_H
#define UNI_STREAM_H

#include <fstream>

#if defined __MINGW32__
    #include <cstdio>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <ext/stdio_filebuf.h>
    #include <string>
    #include <type_traits>
    #include <stringapiset.h>
    #include <vector>

namespace uni_stream {

#if __cplusplus >= 201703L
  // Enable if _Path is a filesystem::path or experimental::filesystem::path
  template<typename _Path, typename _Result = _Path, typename _Path2
           = decltype(std::declval<_Path&>().make_preferred().filename())>
    using _If_fs_path = std::enable_if_t<std::is_same_v<_Path, _Path2>, _Result>;
#endif // C++17

    inline std::vector<wchar_t> _utf8ToUtf16(const char *__s)
    {
        std::vector<wchar_t> wstr;
        int size = ::MultiByteToWideChar(CP_UTF8, 0, __s, -1, NULL, 0);
        if (size > 0) {
            wstr.resize(size+1);
            size = ::MultiByteToWideChar(CP_UTF8, 0, __s, -1, wstr.data(), size);
            if (size <= 0)
                wstr.resize(0);
        }
        return wstr;
    }


    //////////////////////////////////////////////////////////////////////
    // ifstream
    //////////////////////////////////////////////////////////////////////
    template<class BASE, std::ios_base::openmode BASE_MODE>
    class uni_fstream : public BASE {
    public:
        uni_fstream( ) : file(nullptr)
        {
            this->set_rdbuf(&filebuf);
        }

        explicit uni_fstream(const char* __s, std::ios_base::openmode __mode = BASE_MODE) : file(nullptr)
        {
            this->set_rdbuf(&filebuf);
            open(__s,__mode);
        }

        explicit uni_fstream(const std::string& __s, std::ios_base::openmode __mode = BASE_MODE) : file(nullptr)
        {
            this->set_rdbuf(&filebuf);
            open(__s,__mode);
        }
        ~uni_fstream()
        {
            close();
        }

        uni_fstream (const uni_fstream&) = delete;
        uni_fstream(uni_fstream&& __rhs)
            : std::ifstream(std::move(__rhs)),filebuf(std::move(__rhs.filebuf)),file(std::move(__rhs.file))
        {
            this->set_rdbuf(&filebuf);
        }

        uni_fstream& operator= (const uni_fstream&) = delete;
        uni_fstream& operator=(uni_fstream&& __rhs)
        {
            std::ifstream::operator=(std::move(__rhs));
            filebuf = std::move(__rhs.filebuf);
            this->set_rdbuf(&filebuf);
            file = std::move(__rhs.file);
            return *this;
        }

        void swap(uni_fstream& __rhs)
        {
            std::ifstream::swap(__rhs);
            filebuf.swap(__rhs.filebuf);
            this->set_rdbuf(&filebuf);
            __rhs.set_rdbuf(&__rhs.filebuf);
            std::swap(file,__rhs.file);
        }

        void open(const char* __s, std::ios_base::openmode __mode = BASE_MODE)
        {
            if (is_open()) {
                this->setstate(std::ios_base::failbit);
                return;
            }
            if (BASE_MODE != (std::ios_base::in | std::ios_base::out))
                __mode |= BASE_MODE;
            const wchar_t *mode = fopen_mode(__mode);
            if (mode == nullptr) {
                this->setstate(std::ios_base::failbit);
                return;
            }
            std::vector<wchar_t> wstr = _utf8ToUtf16(__s);
            if (wstr.size() == 0) {
                this->setstate(std::ios_base::failbit);
                return;
            }
            file = _wfopen(wstr.data(), mode);
            if (! file) {
                this->setstate(std::ios_base::failbit);
                return;
            }
            filebuf =  __gnu_cxx::stdio_filebuf<char>(file, __mode);
            if (!filebuf.is_open()) {
                this->setstate(std::ios_base::failbit);
                fclose(file);
                return;
            }
            this->clear();
        }

        void open(const std::string& __s, std::ios_base::openmode __mode = BASE_MODE)
        {
            open(__s.c_str(),__mode);
        }

#if __cplusplus >= 201703L
        template<typename _Path, typename _Require = _If_fs_path<_Path>>
        uni_fstream(const _Path& __s,
                       std::ios_base::openmode __mode = BASE_MODE)
            : uni_fstream(__s.c_str(), __mode)
        { }

        template<typename _Path>
        _If_fs_path<_Path, void>
        open(const _Path& __s, std::ios_base::openmode __mode = BASE_MODE)
        { open(__s.c_str(), __mode); }
#endif // C++17

        void close()
        {
            if (! is_open())
                return;

            if (filebuf.close() && ::fclose(file) == 0)
                return;
            this->setstate(std::ios_base::failbit);
        }

        bool is_open() const
        {
            return filebuf.is_open();
        }

    private:
        __gnu_cxx::stdio_filebuf<char> filebuf;
        FILE* file;

        static const wchar_t*
        fopen_mode(std::ios_base::openmode mode)
        {
          enum
            {
          in     = std::ios_base::in,
          out    = std::ios_base::out,
          trunc  = std::ios_base::trunc,
          app    = std::ios_base::app,
          binary = std::ios_base::binary
            };

          // _GLIBCXX_RESOLVE_LIB_DEFECTS
          // 596. 27.8.1.3 Table 112 omits "a+" and "a+b" modes.
          switch (mode & (in|out|trunc|app|binary))
            {
            case (   out                 ): return L"w";
            case (   out      |app       ): return L"a";
            case (             app       ): return L"a";
            case (   out|trunc           ): return L"w";
            case (in                     ): return L"r";
            case (in|out                 ): return L"r+";
            case (in|out|trunc           ): return L"w+";
            case (in|out      |app       ): return L"a+";
            case (in          |app       ): return L"a+";

            case (   out          |binary): return L"wb";
            case (   out      |app|binary): return L"ab";
            case (             app|binary): return L"ab";
            case (   out|trunc    |binary): return L"wb";
            case (in              |binary): return L"rb";
            case (in|out          |binary): return L"r+b";
            case (in|out|trunc    |binary): return L"w+b";
            case (in|out      |app|binary): return L"a+b";
            case (in          |app|binary): return L"a+b";

            default: return nullptr; // invalid
            }
        }

    };

    inline int rename_file(const std::string& old_name, const std::string& new_name)
    {
        int err = 0;
        std::vector<wchar_t> __old = _utf8ToUtf16(old_name.c_str());
        if (__old.size() == 0)
            return EILSEQ;
        std::vector<wchar_t> __new = _utf8ToUtf16(new_name.c_str());
        if (__new.size() == 0)
            return EILSEQ;
        if (_wunlink(__new.data()) == 0 || errno == ENOENT) {
            if (_wrename(__old.data(),__new.data()) < 0)
                err = errno;
        } else {
            err = errno;
        }
        return err;
    }

    typedef uni_fstream<std::ifstream,std::ios_base::in> ifstream;
    typedef uni_fstream<std::ofstream,std::ios_base::out> ofstream;
    typedef uni_fstream<std::fstream,std::ios_base::in | std::ios_base::out> fstream;

} // namespace uni_stream

#else   // __MINGW32__

namespace uni_stream {

inline int rename_file(const std::string& old_name, const std::string& new_name)
{
    return ::rename(old_name.c_str(),new_name.c_str()) < 0 ? errno : 0;
}

typedef ::std::ifstream ifstream;
typedef ::std::ofstream ofstream;
typedef ::std::fstream fstream;

} // namespace uni_stream

#endif  // __MINGW32__

#endif // UNI_STREAM_H
