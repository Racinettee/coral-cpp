// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// and Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_STDLIB_HPP_
#define CHAISCRIPT_STDLIB_HPP_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "chaiscript_defines.hpp"
#include "dispatchkit/dispatchkit.hpp"
#include "dispatchkit/boxed_value.hpp"

#ifndef CHAISCRIPT_NO_THREADS
#include <future>
#endif


/// @file
///
/// This file generates the standard library that normal ChaiScript usage requires.

namespace chaiscript
{
  class Std_Lib
  {
    public:

      static ModulePtr library();

  };
}

#endif

