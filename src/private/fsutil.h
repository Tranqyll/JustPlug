/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Fabien Caylus
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FSUTIL_H
#define FSUTIL_H

/*
 * This file is an internal header. It's not part of the public API,
 * and may change at any moment.
 */

#include <string> // for std::string
#include <vector> // for std::vector
#include <cerrno> // for errno
#include <cstdlib> // for malloc and free

#include "tinydir/tinydir.h"
#include "whereami/src/whereami.h"

#include "confinfo.h"

/*
 * Collection of some useful filesystem functions.
 */

namespace jp_private
{
namespace fsutil
{

// Returns the extension (without dot) of libraries
inline std::string libraryExtension()
{
#if defined(CONFINFO_PLATFORM_WIN32) || defined(CONFINFO_PLATFORM_CYGWIN)
    return "dll"
#elif defined(CONFINFO_PLATFORM_MACOS)
    return "dylib"
#else
    return "so";
#endif
}

// Retur the suffix for libraries (.extension)
inline std::string librarySuffix()
{
    return "." + libraryExtension();
}

typedef std::vector<std::string> PathList;

// List files in the specified directory, and append them to filesList
// The search can be recursive across directories
// extFilter can be used to search only for specified files
// NOTE: Return false on error (even if errors occurs, filesList
// could have been modified)
// NOTE: This function sets the errno variable in case of errors
inline bool listFilesInDir(const std::string& rootDir,
                           PathList* filesList,
                           const std::string& extFilter = std::string(),
                           bool recursive = false)
{
    bool success = true;

    if(!filesList)
    {
        errno = EINVAL;
        return false;
    }

    tinydir_dir dir;
    if(tinydir_open(&dir, rootDir.c_str()) == -1)
        return false;

    while(dir.has_next)
    {
        tinydir_file file;
        if(tinydir_readfile(&dir, &file) == -1)
        {
            success = false;
            continue;
        }

        // Regular file
        if(file.is_reg && (extFilter.empty() || extFilter == file.extension))
            filesList->push_back(file.path);
        // Directory
        else if(recursive
                && file.is_dir
                && strcmp(file.name, ".") != 0
                && strcmp(file.name, "..") != 0)
        {
            if(!listFilesInDir(file.path, filesList, extFilter, true))
                success = false;
        }

        if(tinydir_next(&dir) == -1)
        {
            success = false;
            break;
        }
    }
    tinydir_close(&dir);

    return success;
}

inline bool listLibrariesInDir(const std::string& rootDir,
                               PathList* filesList,
                               bool recursive = false)
{
    return listFilesInDir(rootDir, filesList, libraryExtension(), recursive);
}

// Returns the app directory
// Use whereami library
inline std::string appDir()
{
    const int length = wai_getExecutablePath(nullptr, 0, nullptr);
    if(length == -1)
        return std::string();
    char* path = (char*)malloc(length+1);
    int dirnameLength = length;
    if(wai_getExecutablePath(path, length, &dirnameLength) != length)
        return std::string();

    // Only get the dirname
    path[dirnameLength] = '\0';

    std::string str(path);
    free(path);
    return str;
}



} // namespace fsutil
} // namespace jp_private


#endif // FSUTIL_H