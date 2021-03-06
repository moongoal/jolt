#include <cstdlib>
#include <jolt/memory/allocator.hpp>
#include <jolt/collections/linkedlist.hpp>
#include <jolt/io/stream.hpp>
#include "fs-driver.hpp"

using namespace jolt::text;
using namespace jolt::path;

namespace jolt {
    namespace vfs {
        Path FSDriver::virtual_to_actual(Path const &vpath) const {
            Path final_path{normalize(vpath)};

            final_path = is_absolute(final_path) ? final_path.replace(get_virtual_path(), get_os_path())
                                                 : String::join(SEPARATOR, get_os_path(), final_path);

            return final_path;
        }

        Path FSDriver::actual_to_virtual(Path const &apath) const {
            Path final_path = apath.replace(get_os_path(), get_virtual_path());

            return normalize(final_path);
        }

        io::Stream *FSDriver::open_impl(const Path &res_path, io::ModeFlags const mode) {
            Path actual_path = virtual_to_actual(res_path);

            return jltnew(io::FileStream, actual_path, mode);
        }

        FSDriver::file_name_vec FSDriver::list_impl() const { return list_impl(get_virtual_path(), true); }

        FSDriver::file_name_vec FSDriver::list_impl(path::Path const &path, bool const recurse) const {
            WIN32_FIND_DATAA find_data;
            file_name_vec result{256};
            collections::LinkedList<Path> folders{virtual_to_actual(path)};

            while(folders.get_first_node()) {
                decltype(folders)::Node *const cur_node = folders.get_first_node();
                Path const cur_dir = cur_node->get_value();
                Path const pattern = cur_dir + "/*";

                HANDLE h_find = FindFirstFileA(reinterpret_cast<const char *>(pattern.get_raw()), &find_data);
                jltassert2(h_find != INVALID_HANDLE_VALUE, "Invalid handle returned by FindFirstFile()");

                do {
                    if(find_data.cFileName[0] != '.') {
                        Path const &file_name = s(find_data.cFileName);
                        Path const file_path = String::join(SEPARATOR, cur_dir, file_name);
                        Path const vpath = actual_to_virtual(file_path);

                        result.push(vpath);

                        if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && recurse) {
                            folders.add(file_path);
                        }
                    }
                } while(FindNextFileA(h_find, &find_data));

                jltassert2(GetLastError() == ERROR_NO_MORE_FILES, "Error returned by FindNextFile()");

                FindClose(h_find);
                folders.remove(*cur_node);
            }

            return result;
        }
    } // namespace vfs
} // namespace jolt
