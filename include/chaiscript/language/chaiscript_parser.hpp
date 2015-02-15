// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009-2012, Jonathan Turner (jonathan@emptycrate.com)
// Copyright 2009-2015, Jason Turner (jason@emptycrate.com)
// http://www.chaiscript.com

#ifndef CHAISCRIPT_PARSER_HPP_
#define CHAISCRIPT_PARSER_HPP_

#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../dispatchkit/boxed_value.hpp"
#include "chaiscript_common.hpp"

namespace chaiscript
{
  /// \brief Classes and functions used during the parsing process.
  namespace parser
  {
    /// \brief Classes and functions internal to the parsing process. Not supported for the end user.
    namespace detail 
    {
      enum Alphabet
      {   symbol_alphabet = 0
        ,   keyword_alphabet
          ,   int_alphabet
          ,   float_alphabet
          ,   x_alphabet
          ,   hex_alphabet
          ,   b_alphabet
          ,   bin_alphabet
          ,   id_alphabet
          ,   white_alphabet
          ,   int_suffix_alphabet
          ,   float_suffix_alphabet
          ,   max_alphabet
          ,   lengthof_alphabet = 256
      };
    }

    class ChaiScript_Parser {

      std::string::const_iterator m_input_pos, m_input_end;
      int m_line, m_col;
      std::string m_multiline_comment_begin;
      std::string m_multiline_comment_end;
      std::string m_singleline_comment;
      std::shared_ptr<std::string> m_filename;
      std::vector<AST_NodePtr> m_match_stack;
      bool m_alphabet[detail::max_alphabet][detail::lengthof_alphabet];

      std::vector<std::vector<std::string>> m_operator_matches;
      std::vector<AST_Node_Type::Type> m_operators;

      public:
      ChaiScript_Parser()
        : m_line(-1), m_col(-1),
          m_multiline_comment_begin("/*"),
          m_multiline_comment_end("*/"),
          m_singleline_comment("//")
      {
        setup_operators();
      }

      ChaiScript_Parser(const ChaiScript_Parser &) = delete;
      ChaiScript_Parser &operator=(const ChaiScript_Parser &) = delete;

      void setup_operators();

      /// test a char in an m_alphabet
      bool char_in_alphabet(char c, detail::Alphabet a) const { return m_alphabet[a][static_cast<int>(c)]; }

      /// Prints the parsed ast_nodes as a tree
      /*
         void debug_print(AST_NodePtr t, std::string prepend = "") {
         std::cout << prepend << "(" << ast_node_type_to_string(t->identifier) << ") " << t->text << " : " << t->start.line << ", " << t->start.column << '\n';
         for (unsigned int j = 0; j < t->children.size(); ++j) {
         debug_print(t->children[j], prepend + "  ");
         }
         }
         */
      /// Shows the current stack of matched ast_nodes
      void show_match_stack() const {
        for (auto & elem : m_match_stack) {
          //debug_print(match_stack[i]);
          std::cout << elem->to_string();
        }
      }

      /// Clears the stack of matched ast_nodes
      void clear_match_stack() {
        m_match_stack.clear();
      }

      /// Returns the front-most AST node
      AST_NodePtr ast() const {
        return m_match_stack.front();
      }

      /// Helper function that collects ast_nodes from a starting position to the top of the stack into a new AST node
      void build_match(AST_NodePtr t_t, size_t t_match_start);

      /// Check to see if there is more text parse
      inline bool has_more_input() const {
        return (m_input_pos != m_input_end);
      }
      /// Skips any multi-line or single-line comment
      bool SkipComment();
      /// Skips ChaiScript whitespace, which means space and tab, but not cr/lf
      /// jespada: Modified SkipWS to skip optionally CR ('\n') and/or LF+CR ("\r\n")
      bool SkipWS(bool skip_cr=false);

      /// Reads a floating point value from input, without skipping initial whitespace
      bool Float_();

      /// Reads a hex value from input, without skipping initial whitespace
      bool Hex_();

      /// Reads an integer suffix
      void IntSuffix_() {
        while (has_more_input() && char_in_alphabet(*m_input_pos, detail::int_suffix_alphabet))
        {
          ++m_input_pos;
          ++m_col;
        }
      }

      /// Reads a binary value from input, without skipping initial whitespace
      bool Binary_();

      /// Parses a floating point value and returns a Boxed_Value representation of it
      static Boxed_Value buildFloat(const std::string &t_val);

      template<typename IntType>
      static Boxed_Value buildInt(const IntType &t_type, const std::string &t_val)
      {
        bool unsigned_ = false;
        bool long_ = false;
        bool longlong_ = false;

        auto i = t_val.size();

        for (; i > 0; --i)
        {
          const char val = t_val[i-1];

          if (val == 'u' || val == 'U')
          {
            unsigned_ = true;
          } else if (val == 'l' || val == 'L') {
            if (long_)
            {
              longlong_ = true;
            }

            long_ = true;
          } else {
            break;
          }
        }

        std::stringstream ss(t_val.substr(0, i));
        ss >> t_type;

        std::stringstream testu(t_val.substr(0, i));
        uint64_t u;
        testu >> t_type >> u;

        bool unsignedrequired = false;

        if ((u >> (sizeof(int) * 8)) > 0)
        {
          //requires something bigger than int
          long_ = true;
        }

        static_assert(sizeof(long) == sizeof(uint64_t) || sizeof(long) * 2 == sizeof(uint64_t), "Unexpected sizing of integer types");

        if ((sizeof(long) < sizeof(uint64_t)) 
            && (u >> ((sizeof(uint64_t) - sizeof(long)) * 8)) > 0)
        {
          //requires something bigger than long
          longlong_ = true;
        }


        size_t size = sizeof(int) * 8;

        if (longlong_)
        {
          size = sizeof(int64_t) * 8;
        } else if (long_) {
          size = sizeof(long) * 8;
        } 

        if ( (u >> (size - 1)) > 0)
        {
          unsignedrequired = true;
        }

        if (unsignedrequired && !unsigned_)
        {
          if (t_type == &std::hex || t_type == &std::oct)
          {
            // with hex and octal we are happy to just make it unsigned
            unsigned_ = true;
          } else {
            // with decimal we must bump it up to the next size
            if (long_)
            {
              longlong_ = true;
            } else if (!long_ && !longlong_) {
              long_ = true;
            }
          }
        }

        if (unsigned_)
        {
          if (longlong_)
          {
            uint64_t val;
            ss >> val;
            return const_var(val);
          } else if (long_) {
            unsigned long val;
            ss >> val;
            return const_var(val);
          } else {
            unsigned int val;
            ss >> val;
            return const_var(val);
          }
        } else {
          if (longlong_)
          {
            int64_t val;
            ss >> val;
            return const_var(val);
          } else if (long_) {
            long val;
            ss >> val;
            return const_var(val);
          } else {
            int val;
            ss >> val;
            return const_var(val);
          }
        }
      }

      /// Reads a number from the input, detecting if it's an integer or floating point
      bool Num(const bool t_capture = false);

      /// Reads an identifier from input which conforms to C's identifier naming conventions, without skipping initial whitespace
      bool Id_();

      /// Reads (and potentially captures) an identifier from input
      bool Id(const bool t_capture = false);

      /// Reads an argument from input
      bool Arg();

      /// Checks for a node annotation of the form "#<annotation>"
      bool Annotation();

      /// Reads a quoted string from input, without skipping initial whitespace
      bool Quoted_String_();

      /// Reads (and potentially captures) a quoted string from input.  Translates escaped sequences.
      bool Quoted_String(const bool t_capture = false);

      /// Reads a character group from input, without skipping initial whitespace
      bool Single_Quoted_String_();

      /// Reads (and potentially captures) a char group from input.  Translates escaped sequences.
      bool Single_Quoted_String(const bool t_capture = false);

      /// Reads a char from input if it matches the parameter, without skipping initial whitespace
      bool Char_(const char c);

      /// Reads (and potentially captures) a char from input if it matches the parameter
      bool Char(const char t_c, bool t_capture = false);

      /// Reads a string from input if it matches the parameter, without skipping initial whitespace
      bool Keyword_(const char *t_s);

      /// Reads (and potentially captures) a string from input if it matches the parameter
      bool Keyword(const char *t_s, bool t_capture = false);

      /// Reads a symbol group from input if it matches the parameter, without skipping initial whitespace
      bool Symbol_(const char *t_s);

      /// Reads (and potentially captures) a symbol group from input if it matches the parameter
      bool Symbol(const char *t_s, const bool t_capture = false, const bool t_disallow_prevention=false);

      /// Reads an end-of-line group from input, without skipping initial whitespace
      bool Eol_();

      /// Reads (and potentially captures) an end-of-line group from input
      bool Eol(const bool t_capture = false);

      /// Reads a comma-separated list of values from input, for function declarations
      bool Decl_Arg_List();


      /// Reads a comma-separated list of values from input
      bool Arg_List();

      /// Reads possible special container values, including ranges and map_pairs
      bool Container_Arg_List();

      /// Reads a lambda (anonymous function) from input
      bool Lambda();

      /// Reads a function definition from input
      bool Def(const bool t_class_context = false);

      /// Reads a function definition from input
      bool Try();

      /// Reads an if/else if/else block from input
      bool If();

      /// Reads a class block from input
      bool Class();

      /// Reads a while block from input
      bool While();

      /// Reads the C-style for conditions from input
      bool For_Guards();
      /**
       * Reads a for block from input
       */
      bool For();

      /// Reads a case block from input
      bool Case();

      /// Reads a switch statement from input
      bool Switch();

      /// Reads a curly-brace C-style class block from input
      bool Class_Block();

      /// Reads a curly-brace C-style block from input
      bool Block();

      /// Reads a return statement from input
      bool Return();

      /// Reads a break statement from input
      bool Break();

      /// Reads a continue statement from input
      bool Continue();

      /// Reads a dot expression(member access), then proceeds to check if it's a function or array call
      bool Dot_Fun_Array();

      /// Reads a variable declaration from input
      bool Var_Decl(const bool t_class_context = false);
      
      /// Reads an expression surrounded by parentheses from input
      bool Paren_Expression();

      /// Reads, and identifies, a short-form container initialization from input
      bool Inline_Container();

      /// Parses a variable specified with a & aka reference
      bool Reference();

      /// Reads a unary prefixed expression from input
      bool Prefix();

      /// Parses any of a group of 'value' style ast_node groups from input
      bool Value();

      bool Operator_Helper(const size_t t_precedence);

      bool Operator(const size_t t_precedence = 0);

      /// Reads a pair of values used to create a map initialization from input
      bool Map_Pair();

      /// Reads a pair of values used to create a range initialization from input
      bool Value_Range();

      /// Parses a string of binary equation operators
      bool Equation();

      /// Parses statements allowed inside of a class block
      bool Class_Statements();

      /// Top level parser, starts parsing of all known parses
      bool Statements();

      /// Parses the given input string, tagging parsed ast_nodes with the given m_filename.
      bool parse(const std::string &t_input, std::string t_fname);
    };
  }
}

#endif /* CHAISCRIPT_PARSER_HPP_ */

