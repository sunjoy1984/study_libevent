// Copyright (c) 2014 Baidu.com, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// This file is modified from boost.
//
// Copyright Beman Dawes 2002, 2006
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See library home page at http://www.boost.org/libs/system

#ifndef _COMM_PTR_BAD_WEAK_PTR_
#define _COMM_PTR_BAD_WEAK_PTR_

#include <exception>

namespace comm {
namespace ptr {

class bad_weak_ptr: public std::exception
{
public:
    virtual char const * what() const throw()
    {
        return "base::ptr::bad_weak_ptr";
    }
};

} // namespace ptr
} // namespace base

#endif // _COMM_PTR_BAD_WEAK_PTR_

/* vim: set ts=4 sw=4 sts=4 tw=100 */
