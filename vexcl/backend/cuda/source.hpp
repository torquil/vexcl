#ifndef VEXCL_BACKEND_CUDA_SOURCE_HPP
#define VEXCL_BACKEND_CUDA_SOURCE_HPP

/*
The MIT License

Copyright (c) 2012-2013 Denis Demidov <ddemidov@ksu.ru>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/**
 * \file   vexcl/backend/cuda/source.hpp
 * \author Denis Demidov <ddemidov@ksu.ru>
 * \brief  Helper class for CUDA source code generation.
 */

#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>

#include <vexcl/backend/common.hpp>
#include <vexcl/types.hpp>

namespace vex {

template <class T> struct global_ptr {};
template <class T> struct shared_ptr {};

template <class T>
struct type_name_impl <global_ptr<T> > {
    static std::string get() {
        std::ostringstream s;
        s << type_name<T>() << " *";
        return s.str();
    }
};

template <class T>
struct type_name_impl < global_ptr<const T> > {
    static std::string get() {
        std::ostringstream s;
        s << "const " << type_name<T>() << " *";
        return s.str();
    }
};

template <class T>
struct type_name_impl <shared_ptr<T> > {
    static std::string get() {
        std::ostringstream s;
        s << "__shared__ " << type_name<T>() << " *";
        return s.str();
    }
};

template <class T>
struct type_name_impl <shared_ptr<const T> > {
    static std::string get() {
        std::ostringstream s;
        s << "const __shared__" << type_name<T>() << " *";
        return s.str();
    }
};

template<typename T>
struct type_name_impl<T*>
{
    static std::string get() {
        return type_name_impl< global_ptr<T> >::get();
    }
};

namespace backend {

/// Returns standard CUDA program header.
/**
 * Defines pragmas necessary to work with double precision and anything
 * provided by the user with help of push_program_header().
 */
inline std::string standard_kernel_header(const command_queue &q) {
    return get_program_header(q);
}

class source_generator {
    private:
        unsigned           indent;
        bool               first_prm, cpu;
        std::ostringstream src;

    public:
        source_generator() : indent(0), first_prm(true), cpu(false) { }

        source_generator(const command_queue &queue)
            : indent(0), first_prm(true)
        {
            src << standard_kernel_header(queue);
        }

        source_generator& new_line() {
            src << "\n" << std::string(2 * indent, ' ');
            return *this;
        }

        source_generator& open(const char *bracket) {
            new_line() << bracket;
            ++indent;
            return *this;
        }

        source_generator& close(const char *bracket) {
            assert(indent > 0);
            --indent;
            new_line() << bracket;
            return *this;
        }

        template <class Return>
        source_generator& function(const std::string &name) {
            first_prm = true;
            new_line() << "__device__ " << type_name<Return>() << " " << name;
            return *this;
        }

        source_generator& kernel(const std::string &name) {
            first_prm = true;
            new_line() << "extern \"C\" __global__ void " << name;
            return *this;
        }

        template <class Prm>
        source_generator& parameter(const std::string &name) {
            if (first_prm)
                first_prm = false;
            else
                src << ",";

            new_line() << type_name<typename std::decay<Prm>::type>() << " " << name;
            return *this;
        }

        source_generator& grid_stride_loop(
                const std::string &idx = "idx", const std::string &bnd = "n"
                )
        {
            new_line() << "for";
            open("(");
            new_line() << "size_t " << idx << " = blockDim.x * blockIdx.x + threadIdx.x, "
                "grid_size = blockDim.x * gridDim.x;";
            new_line() << idx << " < " << bnd << ";";
            new_line() << idx << " += grid_size";
            close(")");

            return *this;
        }

        source_generator& barrier() {
            src << "__syncthreads();";
            return *this;
        }

        std::string str() const {
            return src.str();
        }
    private:
        template <class T>
        friend inline
        source_generator& operator<<(source_generator &src, T &&t) {
            src.src << t;
            return src;
        }
};

} // namespace backend
} // namespace vex

#endif
