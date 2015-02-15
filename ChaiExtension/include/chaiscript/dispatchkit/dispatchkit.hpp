// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2015, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_DISPATCHKIT_HPP_
#define CHAISCRIPT_DISPATCHKIT_HPP_

#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "../chaiscript_defines.hpp"
#include "../chaiscript_threading.hpp"
#include "boxed_cast.hpp"
#include "boxed_cast_helper.hpp"
#include "boxed_value.hpp"
#include "type_conversions.hpp"
#include "dynamic_object.hpp"
#include "proxy_constructors.hpp"
#include "proxy_functions.hpp"
#include "type_info.hpp"

namespace chaiscript {
class Boxed_Number;
}  // namespace chaiscript

namespace chaiscript {
namespace dispatch {
class Dynamic_Proxy_Function;
class Proxy_Function_Base;
struct Placeholder_Object;
}  // namespace dispatch
}  // namespace chaiscript


/// \namespace chaiscript::dispatch
/// \brief Classes and functions specific to the runtime dispatch side of ChaiScript. Some items may be of use to the end user.

namespace chaiscript
{
  namespace exception
  {
    /// Exception thrown in the case that an object name is invalid because it is a reserved word
    class reserved_word_error : public std::runtime_error
    {
      public:
        reserved_word_error(const std::string &t_word) CHAISCRIPT_NOEXCEPT
          : std::runtime_error("Reserved word not allowed in object name: " + t_word), m_word(t_word)
        {
        }

        reserved_word_error(const reserved_word_error &) = default;

        virtual ~reserved_word_error() CHAISCRIPT_NOEXCEPT {}

        std::string word() const
        {
          return m_word;
        }

      private:
        std::string m_word;
    };

    /// Exception thrown in the case that an object name is invalid because it contains illegal characters
    class illegal_name_error : public std::runtime_error
    {
      public:
        illegal_name_error(const std::string &t_name) CHAISCRIPT_NOEXCEPT
          : std::runtime_error("Reserved name not allowed in object name: " + t_name), m_name(t_name)
        {
        }

        illegal_name_error(const illegal_name_error &) = default;

        virtual ~illegal_name_error() CHAISCRIPT_NOEXCEPT {}

        std::string name() const
        {
          return m_name;
        }

      private:
        std::string m_name;
    };


    /// Exception thrown in the case that an object name is invalid because it already exists in current context
    class name_conflict_error : public std::runtime_error
    {
      public:
        name_conflict_error(const std::string &t_name) CHAISCRIPT_NOEXCEPT
          : std::runtime_error("Name already exists in current context " + t_name), m_name(t_name)
        {
        }

        name_conflict_error(const name_conflict_error &) = default;

        virtual ~name_conflict_error() CHAISCRIPT_NOEXCEPT {}

        std::string name() const
        {
          return m_name;
        }

      private:
        std::string m_name;

    };


    /// Exception thrown in the case that a non-const object was added as a shared object
    class global_non_const : public std::runtime_error
    {
      public:
        global_non_const() CHAISCRIPT_NOEXCEPT
          : std::runtime_error("a global object must be const")
        {
        }

        global_non_const(const global_non_const &) = default;
        virtual ~global_non_const() CHAISCRIPT_NOEXCEPT {}
    };
  }


  /// \brief Holds a collection of ChaiScript settings which can be applied to the ChaiScript runtime.
  ///        Used to implement loadable module support.
  class Module
  {
    public:
      Module &add(Type_Info ti, std::string name);

      Module &add(Type_Conversion d);

      Module &add(Proxy_Function f, std::string name);

      Module &add_global_const(Boxed_Value t_bv, std::string t_name);


      //Add a bit of ChaiScript to eval during module implementation
      Module &eval(const std::string &str);

      Module &add(const std::shared_ptr<Module> &m);

      template<typename Eval, typename Engine>
        void apply(Eval &t_eval, Engine &t_engine) const
        {
          apply(m_typeinfos.begin(), m_typeinfos.end(), t_engine);
          apply(m_funcs.begin(), m_funcs.end(), t_engine);
          apply_eval(m_evals.begin(), m_evals.end(), t_eval);
          apply_single(m_conversions.begin(), m_conversions.end(), t_engine);
          apply_globals(m_globals.begin(), m_globals.end(), t_engine);
        }

      ~Module()
      {
      }

    private:
      std::vector<std::pair<Type_Info, std::string> > m_typeinfos;
      std::vector<std::pair<Proxy_Function, std::string> > m_funcs;
      std::vector<std::pair<Boxed_Value, std::string> > m_globals;
      std::vector<std::string> m_evals;
      std::vector<Type_Conversion> m_conversions;

      template<typename T, typename InItr>
        static void apply(InItr begin, const InItr end, T &t) 
        {
          for_each(begin, end, [&t](typename std::iterator_traits<InItr>::reference obj)
              {
                try {
                  t.add(obj.first, obj.second);
                } catch (const chaiscript::exception::name_conflict_error &) {
                  /// \todo Should we throw an error if there's a name conflict 
                  ///       while applying a module?
                }
              }
            );
        }

      template<typename T, typename InItr>
        static void apply_globals(InItr begin, InItr end, T &t)
        {
          while (begin != end)
          {
            t.add_global_const(begin->first, begin->second);
            ++begin;
          }
        }

      template<typename T, typename InItr>
        static void apply_single(InItr begin, InItr end, T &t)
        {
          while (begin != end)
          {
            t.add(*begin);
            ++begin;
          }
        }

      template<typename T, typename InItr>
        static void apply_eval(InItr begin, InItr end, T &t)
        {
          while (begin != end)
          {
            t.eval(*begin);
            ++begin;
          }
        }
  };

  /// Convenience typedef for Module objects to be added to the ChaiScript runtime
  typedef std::shared_ptr<Module> ModulePtr;

  namespace detail
  {
    /// A Proxy_Function implementation that is able to take
    /// a vector of Proxy_Functions and perform a dispatch on them. It is 
    /// used specifically in the case of dealing with Function object variables
    class Dispatch_Function : public dispatch::Proxy_Function_Base
    {
      public:
        Dispatch_Function(std::vector<Proxy_Function> t_funcs)
          : Proxy_Function_Base(build_type_infos(t_funcs), calculate_arity(t_funcs)),
            m_funcs(std::move(t_funcs))
        {
        }

        virtual bool operator==(const dispatch::Proxy_Function_Base &rhs) const CHAISCRIPT_OVERRIDE
        {
          try {
            const auto &dispatchfun = dynamic_cast<const Dispatch_Function &>(rhs);
            return m_funcs == dispatchfun.m_funcs;
          } catch (const std::bad_cast &) {
            return false;
          }
        }

        virtual ~Dispatch_Function() {}

        virtual std::vector<Const_Proxy_Function> get_contained_functions() const CHAISCRIPT_OVERRIDE
        {
          return std::vector<Const_Proxy_Function>(m_funcs.begin(), m_funcs.end());
        }

        static int calculate_arity(const std::vector<Proxy_Function> &t_funcs);

        virtual bool call_match(const std::vector<Boxed_Value> &vals, const Type_Conversions &t_conversions) const CHAISCRIPT_OVERRIDE
        {
          return std::any_of(m_funcs.cbegin(), m_funcs.cend(),
                             [&vals, &t_conversions](const Proxy_Function &f){ return f->call_match(vals, t_conversions); });
        }

        virtual std::string annotation() const CHAISCRIPT_OVERRIDE
        {
          return "Multiple method dispatch function wrapper.";
        }

      protected:
        virtual Boxed_Value do_call(const std::vector<Boxed_Value> &params, const Type_Conversions &t_conversions) const CHAISCRIPT_OVERRIDE
        {
          return dispatch::dispatch(m_funcs, params, t_conversions);
        }

      private:
        std::vector<Proxy_Function> m_funcs;

        static std::vector<Type_Info> build_type_infos(const std::vector<Proxy_Function> &t_funcs);
    };
  }


  namespace detail
  {
    /// Main class for the dispatchkit. Handles management
    /// of the object stack, functions and registered types.
    class Dispatch_Engine
    {
      public:
        typedef std::map<std::string, chaiscript::Type_Info> Type_Name_Map;
        typedef std::map<std::string, Boxed_Value> Scope;
        typedef std::vector<Scope> StackData;

        struct State
        {
          std::map<std::string, std::vector<Proxy_Function> > m_functions;
          std::map<std::string, Proxy_Function> m_function_objects;
          std::map<std::string, Boxed_Value> m_global_objects;
          Type_Name_Map m_types;
          std::set<std::string> m_reserved_words;

          State &operator=(const State &) = default;
          State() = default;
          State(const State &) = default;
        };

        Dispatch_Engine()
          : m_stack_holder(this),
            m_place_holder(std::make_shared<dispatch::Placeholder_Object>())
        {
        }

        ~Dispatch_Engine()
        {
        }

        /// \brief casts an object while applying any Dynamic_Conversion available
        template<typename Type>
          typename detail::Cast_Helper<Type>::Result_Type boxed_cast(const Boxed_Value &bv) const
          {
            return chaiscript::boxed_cast<Type>(bv, &m_conversions);
          }

        /// Add a new conversion for upcasting to a base class
        void add(const Type_Conversion &d);

        /// Add a new named Proxy_Function to the system
        void add(const Proxy_Function &f, const std::string &name);

        /// Set the value of an object, by name. If the object
        /// is not available in the current scope it is created
        void add(const Boxed_Value &obj, const std::string &name);


        /// Adds a named object to the current scope
        /// \warning This version does not check the validity of the name
        /// it is meant for internal use only
        void add_object(const std::string &name, const Boxed_Value &obj);

        /// Adds a new global shared object, between all the threads
        void add_global_const(const Boxed_Value &obj, const std::string &name);


        /// Adds a new global (non-const) shared object, between all the threads
        void add_global(const Boxed_Value &obj, const std::string &name);


        /// Adds a new scope to the stack
        void new_scope();

        /// Pops the current scope from the stack
        void pop_scope()
        {
          m_stack_holder->call_params.pop_back();
          StackData &stack = get_stack_data();
          if (stack.size() > 1)
          {
            stack.pop_back();
          } else {
            throw std::range_error("Unable to pop global stack");
          }
        }


        /// Pushes a new stack on to the list of stacks
        void new_stack()
        {
          // add a new Stack with 1 element
          m_stack_holder->stacks.emplace_back(1);
        }

        void pop_stack()
        {
          m_stack_holder->stacks.pop_back();
        }

        /// Searches the current stack for an object of the given name
        /// includes a special overload for the _ place holder object to
        /// ensure that it is always in scope.
        Boxed_Value get_object(const std::string &name) const;

        /// Registers a new named type
        void add(const Type_Info &ti, const std::string &name);

        /// Returns the type info for a named type
        Type_Info get_type(const std::string &name) const;

        /// Returns the registered name of a known type_info object
        /// compares the "bare_type_info" for the broadest possible
        /// match
        std::string get_type_name(const Type_Info &ti) const;

        /// Return all registered types
        std::vector<std::pair<std::string, Type_Info> > get_types() const;

        /// Return a function by name
        std::vector< Proxy_Function > get_function(const std::string &t_name) const;

        /// \returns a function object (Boxed_Value wrapper) if it exists
        /// \throws std::range_error if it does not
        Boxed_Value get_function_object(const std::string &t_name) const;

        /// Return true if a function exists
        bool function_exists(const std::string &name) const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);

          const auto &functions = get_functions_int();
          return functions.find(name) != functions.end();
        }

        /// \returns All values in the local thread state in the parent scope, or if it doesn't exist,
        ///          the current scope.
        std::map<std::string, Boxed_Value> get_parent_locals() const
        {
          auto &stack = get_stack_data();
          if (stack.size() > 1)
          {
            return stack[1];
          } else {
            return stack[0];
          }
        }

        /// \returns All values in the local thread state, added through the add() function
        std::map<std::string, Boxed_Value> get_locals() const
        {
          auto &stack = get_stack_data();
          auto &scope = stack.front();
          return scope;
        }

        /// \brief Sets all of the locals for the current thread state.
        ///
        /// \param[in] t_locals The map<name, value> set of variables to replace the current state with
        ///
        /// Any existing locals are removed and the given set of variables is added
        void set_locals(const std::map<std::string, Boxed_Value> &t_locals)
        {
          auto &stack = get_stack_data();
          auto &scope = stack.front();
          scope = t_locals;
        }

        ///
        /// Get a map of all objects that can be seen from the current scope in a scripting context
        ///
        std::map<std::string, Boxed_Value> get_scripting_objects() const;

        ///
        /// Get a map of all functions that can be seen from a scripting context
        ///
        std::map<std::string, Boxed_Value> get_function_objects() const;

        /// Get a vector of all registered functions
        std::vector<std::pair<std::string, Proxy_Function > > get_functions() const;

        void add_reserved_word(const std::string &name);

        const Type_Conversions &conversions() const
        {
          return m_conversions;
        }

        Boxed_Value call_function(const std::string &t_name, const std::vector<Boxed_Value> &params) const
        {
          return dispatch::dispatch(get_function(t_name), params, m_conversions);
        }

        Boxed_Value call_function(const std::string &t_name) const
        {
          return call_function(t_name, std::vector<Boxed_Value>());
        }

        Boxed_Value call_function(const std::string &t_name, Boxed_Value p1) const
        {
          return call_function(t_name, std::vector<Boxed_Value>({std::move(p1)}));
        }

        Boxed_Value call_function(const std::string &t_name, Boxed_Value p1, Boxed_Value p2) const
        {
          return call_function(t_name, std::vector<Boxed_Value>({std::move(p1), std::move(p2)}));
        }

        /// Dump object info to stdout
        void dump_object(const Boxed_Value &o) const;

        /// Dump type info to stdout
        void dump_type(const Type_Info &type) const;

        /// Dump function to stdout
        void dump_function(const std::pair<const std::string, Proxy_Function > &f) const;

        /// Returns true if a call can be made that consists of the first parameter
        /// (the function) with the remaining parameters as its arguments.
        Boxed_Value call_exists(const std::vector<Boxed_Value> &params)
        {
          if (params.empty())
          {
            throw chaiscript::exception::arity_error(static_cast<int>(params.size()), 1);
          }

          const Const_Proxy_Function &f = this->boxed_cast<Const_Proxy_Function>(params[0]);

          return Boxed_Value(f->call_match(std::vector<Boxed_Value>(params.begin() + 1, params.end()), m_conversions));
        }

        /// Dump all system info to stdout
        void dump_system() const;

        /// return true if the Boxed_Value matches the registered type by name
        bool is_type(const Boxed_Value &r, const std::string &user_typename) const
        {
          try {
            if (get_type(user_typename).bare_equal(r.get_type_info()))
            {
              return true;
            }
          } catch (const std::range_error &) {
          }

          try {
            const dispatch::Dynamic_Object &d = boxed_cast<const dispatch::Dynamic_Object &>(r);
            return d.get_type_name() == user_typename;
          } catch (const std::bad_cast &) {
          }

          return false;
        }

        std::string type_name(const Boxed_Value &obj) const
        {
          return get_type_name(obj.get_type_info());
        }

        State get_state() const
        {
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);
          chaiscript::detail::threading::shared_lock<chaiscript::detail::threading::shared_mutex> l2(m_global_object_mutex);

          return m_state;
        }

        void set_state(const State &t_state)
        {
          chaiscript::detail::threading::unique_lock<chaiscript::detail::threading::shared_mutex> l(m_mutex);
          chaiscript::detail::threading::unique_lock<chaiscript::detail::threading::shared_mutex> l2(m_global_object_mutex);

          m_state = t_state;
        }

        void save_function_params(std::initializer_list<Boxed_Value> t_params)
        {
          Stack_Holder &s = *m_stack_holder;
          s.call_params.back().insert(s.call_params.back().begin(), std::move(t_params));
        }

        void save_function_params(std::vector<Boxed_Value> &&t_params)
        {
          Stack_Holder &s = *m_stack_holder;

          for (auto &&param : t_params)
          {
            s.call_params.back().insert(s.call_params.back().begin(), std::move(param));
          }
        }

        void save_function_params(const std::vector<Boxed_Value> &t_params)
        {
          Stack_Holder &s = *m_stack_holder;
          s.call_params.back().insert(s.call_params.back().begin(), t_params.begin(), t_params.end());
        }

        void new_function_call()
        {
          Stack_Holder &s = *m_stack_holder;
          if (s.call_depth == 0)
          {
            m_conversions.enable_conversion_saves(true);
          }

          ++s.call_depth;

          save_function_params(m_conversions.take_saves());
        }

        void pop_function_call()
        {
          Stack_Holder &s = *m_stack_holder;
          --s.call_depth;

          assert(s.call_depth >= 0);

          if (s.call_depth == 0)
          {
            s.call_params.back().clear();
            m_conversions.enable_conversion_saves(false);
          }
        }

      private:
        /// Returns the current stack
        /// make const/non const versions
        const StackData &get_stack_data() const
        {
          return m_stack_holder->stacks.back();
        }

        StackData &get_stack_data()
        {
          return m_stack_holder->stacks.back();
        }

        const std::map<std::string, Proxy_Function> &get_function_objects_int() const
        {
          return m_state.m_function_objects;
        }

        std::map<std::string, Proxy_Function> &get_function_objects_int() 
        {
          return m_state.m_function_objects;
        }

        const std::map<std::string, std::vector<Proxy_Function> > &get_functions_int() const
        {
          return m_state.m_functions;
        }

        std::map<std::string, std::vector<Proxy_Function> > &get_functions_int() 
        {
          return m_state.m_functions;
        }

        static bool function_less_than(const Proxy_Function &lhs, const Proxy_Function &rhs);

        /// Throw a reserved_word exception if the name is not allowed
        void validate_object_name(const std::string &name) const;
        /// Implementation detail for adding a function. 
        /// \throws exception::name_conflict_error if there's a function matching the given one being added
        void add_function(const Proxy_Function &t_f, const std::string &t_name);

        mutable chaiscript::detail::threading::shared_mutex m_mutex;
        mutable chaiscript::detail::threading::shared_mutex m_global_object_mutex;

        struct Stack_Holder
        {
          Stack_Holder()
            : call_depth(0)
          {
            stacks.emplace_back(1);
            call_params.emplace_back();
          }

          std::deque<StackData> stacks;

          std::deque<std::list<Boxed_Value>> call_params;
          int call_depth;
        };

        Type_Conversions m_conversions;
        chaiscript::detail::threading::Thread_Storage<Stack_Holder> m_stack_holder;


        State m_state;

        Boxed_Value m_place_holder;
    };
  }
}

#endif

