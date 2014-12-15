/** \file

    wraps boost options_description as printable_opts - show the options values used as well as defaults in
   usage or log messages.  boost program options library lacks any concept of printing configured values; it
   only supports parsing them from strings

   now only using string and vector<string> program options types in order to avoid onerous Streamable
   lexical_cast requirement - meaning little of the boost po library is actually used

*/

#ifndef GRAEHL__SHARED__PROGRAM_OPTIONS_HPP
#define GRAEHL__SHARED__PROGRAM_OPTIONS_HPP

#ifndef DEBUG_GRAEHL_PROGRAM_OPTIONS
#define DEBUG_GRAEHL_PROGRAM_OPTIONS 1
#endif

#include <graehl/shared/ifdbg.hpp>
#if DEBUG_GRAEHL_PROGRAM_OPTIONS
#include <graehl/shared/show.hpp>
DECLARE_DBG_LEVEL(GPROGOPT)
#define GPROGOPT(x) x
#else
#define GPROGOPT(x)
#endif

#ifdef _WIN32
#include <iso646.h>
#endif

#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED 1
#endif
#include <boost/program_options.hpp>
#include <boost/function.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/range/value_type.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <fstream>
#include <boost/pool/object_pool.hpp>
#include <graehl/shared/verbose_exception.hpp>
#include <graehl/shared/prefix_option.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/container.hpp>
#include <graehl/shared/from_strings.hpp>
#include <graehl/shared/noreturn.hpp>
#include <graehl/shared/value_str.hpp>
#include <graehl/shared/type_string.hpp>
#include <deque>
#include <list>
#include <boost/scoped_array.hpp>

namespace graehl {


/// work around obnoxious pickiness of boost cmdline parser - if you pass from
/// shell a='' it complains that there's no value (because there isn't any char
/// following =). this provides a fixed arg vector suitable for the parser
/// (supplying '' as the missing value).
inline bool cmdline_parser_fix_empty_equals(int argc, char const* const* argv, std::vector<std::string>& fixed,
                                            std::string const& forMissingValue = "''") {
  if (!argc) return false;
  bool any = false;
  --argc;
  ++argv;
  fixed.resize(argc);
  for (int i = 0; i < argc; ++i) {
    std::string& fixarg = fixed[i];
    fixarg = argv[i];
    std::string::size_type len = fixarg.size();
    if (len && fixarg[len - 1] == '=') {
      any = true;
      fixarg += forMissingValue;
    }
  }
  return any;
}

SIMPLE_EXCEPTION_PREFIX(program_options_exception, "program options: ");

struct option_options_base {
  boost::optional<bool> composing, multitoken, zero_tokens,
      required;  // would be bool except then i'd have to init.
  bool hidden, defaulted;  // affect options desc, not po::typed_value
  option_options_base() : hidden(), defaulted() {}
  boost::optional<std::string> is;
  static inline bool on(boost::optional<bool> x) { return x && *x; }
  template <class typed_value>
  typed_value* apply(typed_value* tv) const {
#if BOOST_VERSION >= 105000
    if (is) tv->value_name("[" + *is + "]");
#endif
    if (on(composing)) tv->composing();
    if (on(multitoken)) tv->multitoken();
    if (on(zero_tokens)) tv->zero_tokens();
    if (on(required)) tv->required();
    return tv;
  }
};

template <class Val>
struct notify_base {
  boost::function<void()> notify0;
  boost::function<void(Val const&)> notify;
  std::string log_prefix;
  void call(Val const& final_value) const {
    SHOWIF1(GPROGOPT, 1, "notify_base::call", to_string(final_value));
    if (notify) notify(final_value);
    if (notify0) notify0();
  }
};

template <class V>
struct option_options : option_options_base, notify_base<V> {
  notify_base<V> const& notifier() const { return *this; }
  option_options() {}
  boost::optional<value_str> implicit_value_str, default_value_str;
  template <class typed_value>
  typed_value* apply(typed_value* tv) const {
    if (implicit_value_str) tv->implicit_value(implicit_value_str->get<V>(), implicit_value_str->str);

    if (default_value_str) tv->default_value(default_value_str->get<V>(), default_value_str->str);

    option_options_base::apply(tv);
    return tv;
  }

  template <class V2>
  static inline void optional_to(boost::optional<value_str> const& from, boost::optional<value_str>& to) {
    if (from) to = value_str(to_string_or_strings(from->get<V2>()), from->str);
  }

  template <class V2>
  explicit option_options(option_options<V2> const& o)
      : option_options_base(o) {  // cannot copy notifier
    optional_to<V2>(o.implicit_value_str, implicit_value_str);
    optional_to<V2>(o.default_value_str, default_value_str);
  }
};

template <class Val>
struct notify_from_strings : notify_base<Val> {
  typedef notify_base<Val> notify;
  typedef select_from_strings<Val> convert;
  typedef typename convert::string_or_strings from_type;
  Val* pv;
  notify_from_strings(Val* pv, notify const& base) : notify(base), pv(pv) {}
  void operator()(std::vector<std::string> const& finals) const {
    try {
      convert::from_strings(finals, *pv);
    } catch (std::exception& e) {
      throw program_options_exception(this->log_prefix + " - error parsing " + type_name(*pv)
                                      + " from string(s) " + to_string(finals) + ": " + e.what());
    }
    SHOWIF2(GPROGOPT, 1, "notify_from_strings", to_string(finals), to_string(*pv));
    notify::call(*pv);
  }
  void operator()(std::string const& final) const { (*this)(std::vector<std::string>(1, final)); }
  from_type inverse() const { return convert::to_string_or_strings(*pv); }
  std::string inverse_string() const { return convert::to_string(*pv); }
};

template <class OD, class OptionsValues>
void caption_add_options(OD& optionsDesc, OptionsValues& vals) {
  OD nested(vals.caption());
  vals.add_options(nested);
  optionsDesc.add(nested);
}

template <class OD, class OptionsValues, class Prefix>
void caption_add_options(OD& optionsDesc, OptionsValues& vals, Prefix const& prefix) {
  OD nested(vals.caption());
  vals.add_options(nested, prefix);
  optionsDesc.add(nested);
}

inline bool contains(boost::program_options::variables_map const& vm, std::string const& key) {
  return (bool)vm.count(key);
}

template <class V>
inline bool maybe_get(boost::program_options::variables_map const& vm, std::string const& key, V& val) {
  if (vm.count(key)) {
    val = vm[key].as<V>();
    return true;
  }
  return false;
}

inline std::string get_string(boost::program_options::variables_map const& vm, std::string const& key) {
  return vm[key].as<std::string>();
}

// change --opt-name=x --opt_name=x for all strings x.  danger: probably the argv from int main isn't supposed
// to be modified?
inline int arg_minusto_underscore(char* s) {
  if (!*s || *s++ != '-') return 0;
  if (!*s || *s++ != '-') return 0;
  int chars_replaced = 0;
  for (; *s; ++s) {
    if (*s == '=') break;
    if (*s == '-') {
      *s = '_';
      ++chars_replaced;
    }
  }
  return chars_replaced;
}

inline int argv_minus_to_underscore(int argc, char** argv) {
  int chars_replaced = 0;
  for (int i = 1; i < argc; ++i) {
    chars_replaced += arg_minusto_underscore(argv[i]);
  }
  return chars_replaced;
}


template <class Container>
boost::program_options::typed_value<std::vector<std::string> >*
multiple_strings(Container* v, notify_base<Container> const& notify) {
  notify_from_strings<Container> notify_str(v, notify);
  return boost::program_options::value<std::vector<std::string> >()->notifier(notify_str)->composing()->multitoken();
}

template <class Container>
boost::program_options::typed_value<std::vector<std::string> >*
multiple_strings_defaulted(Container* v, notify_base<Container> const& notify) {
  notify_from_strings<Container> notify_str(v, notify);
  return boost::program_options::value<std::vector<std::string> >()
      ->composing()
      ->multitoken()
      ->notifier(notify_str)
      ->default_value(notify_str.inverse(), notify_str.inverse_string());
}

template <class Value>
boost::program_options::typed_value<std::vector<std::string> >*
multiple_strings(Value* v, bool defaulted, notify_base<Value> const& notify, bool allow_zero_tokens = true) {
  boost::program_options::typed_value<std::vector<std::string> >* r
      = defaulted ? multiple_strings_defaulted(v, notify) : multiple_strings(v, notify);
  if (allow_zero_tokens) r->zero_tokens();
  return r;
}

template <class Value>
boost::program_options::typed_value<std::string>* single_string(Value* val, notify_base<Value> const& notify) {
  return boost::program_options::value<std::string>()->notifier(notify_from_strings<Value>(val, notify));
}

template <class Value>
boost::program_options::typed_value<std::string>* single_string_defaulted(Value* val,
                                                                          notify_base<Value> const& notify) {
  notify_from_strings<Value> notify_str(val, notify);
  return boost::program_options::value<std::string>()
      ->notifier(notify_str)
      ->default_value(notify_str.inverse(), notify_str.inverse_string());
}

template <class Val>
boost::program_options::typed_value<std::string>* single_string(Val* cont, bool defaulted,
                                                                notify_base<Val> const& notify) {
  return defaulted ? single_string_defaulted(cont, notify) : single_string(cont, notify);
}

template <class Val, class Enable = void>
struct multiple_for_container {
  typedef std::string strings_value;
  typedef boost::program_options::typed_value<strings_value> typed_value;
  typedef option_options<strings_value> options;
  static inline typed_value* strings(Val* val, bool defaulted, notify_base<Val> const& notify) {
    return defaulted ? single_string_defaulted(val, notify) : single_string(val, notify);
  }
};

template <class Val>
struct multiple_for_container<Val, typename boost::enable_if<is_nonstring_container<Val> >::type> {
  typedef std::vector<std::string> strings_value;
  typedef boost::program_options::typed_value<strings_value> typed_value;
  typedef option_options<strings_value> options;
  static inline typed_value* strings(Val* val, bool defaulted, notify_base<Val> const& notify) {
    return multiple_strings(val, defaulted, notify, true);
  }
};

template <class T>
boost::program_options::typed_value<T>* optional_value(T* v) {
  return boost::program_options::value<T>(v);
}

// to be program options parsed as string(s) then converted via notify
template <class T>
typename multiple_for_container<T>::typed_value* via_strings_value(std::string const& name, T* v,
                                                                   option_options<T> opt = option_options<T>(),
                                                                   bool defaulted = false) {
  opt.log_prefix = "--" + name + " ";
  option_options<typename select_from_strings<T>::string_or_strings> po(opt);
  typename multiple_for_container<T>::typed_value* tv
      = multiple_for_container<T>::strings(v, defaulted, opt.notifier());
  po.apply(tv);
  return tv;
}

template <class T>
boost::program_options::typed_value<T>* defaulted_value(T* v) {
  // this newed ptr is deleted by the program_options lib.
  return boost::program_options::value<T>(v)->default_value(*v, to_string(*v));
}

template <class T>
boost::program_options::typed_value<T>* defaulted_value(T* v, std::string const& default_string) {
  return boost::program_options::value<T>(v)->default_value(*v, default_string);
}


inline void program_options_fatal(std::string const&) NORETURN;
inline void program_options_fatal(std::string const& msg) ANALYZER_NORETURN {
  throw std::runtime_error(msg);
}


inline std::string const& get_single_arg(boost::any& v, std::vector<std::string> const& values) {
  boost::program_options::validators::check_first_occurrence(v);
  return boost::program_options::validators::get_single_string(values);
}

template <class I>
void must_complete_read(I& in, std::string const& msg = "Couldn't parse") {
  char c;
  if (in.bad()) program_options_fatal(msg + " - failed input");
  if (in >> c) program_options_fatal(msg + " - got extra char: " + std::string(c, 1));
}

template <class Ostream>
struct any_printer : public boost::function<void(Ostream&, boost::any const&)> {
  typedef boost::function<void(Ostream&, boost::any const&)> F;

  template <class T>
  struct typed_print {
    void operator()(Ostream& o, boost::any const& t) const {
      o << select_from_strings<T>::to_string(*boost::any_cast<T const>(&t));
    }
  };

  template <class T>
  static void typed_print_template(Ostream& o, boost::any const& t) {
    o << *boost::any_cast<T const>(&t);
  }

  any_printer() {}

  any_printer(const any_printer& x) : F(static_cast<F const&>(x)) {}

  template <class T>
  explicit any_printer(T const* tag)
      : F(typed_print<T>()) {}

  template <class T>
  void set() {
    F f((typed_print<T>()));  // extra parens necessary
    swap(f);
  }
};


enum printable_options_show {
  SHOW_DEFAULTED = 0x1,
  SHOW_EMPTY = 0x2,
  SHOW_DESCRIPTION = 0x4,
  SHOW_HIERARCHY = 0x8,
  SHOW_EMPTY_GROUPS = 0x10,
  SHOW_ALL = 0x0FFF,
  SHOW_HELP = 0x1000
};

// have to wrap regular options_description and store our own tables because
// author didn't make enough stuff protected/public or add a virtual print
// method to value_semantic
template <class Ostream>
struct printable_options_description : boost::program_options::options_description {
  typedef printable_options_description<Ostream> self_type;
  typedef boost::program_options::options_description options_description;
  typedef boost::program_options::option_description option_description;
  typedef boost::shared_ptr<self_type> group_type;
  typedef std::vector<group_type> groups_type;
  struct printable_option {
    typedef boost::shared_ptr<option_description> OD;

    any_printer<Ostream> print;
    OD od;
    bool in_group;
    boost::optional<value_str> implicit;

    std::string const& name() const { return od->long_name(); }

    std::string const& description() const { return od->description(); }

    std::string const& vmkey() const { return od->key(name()); }
    template <class T>
    printable_option(T* tag, OD const& od)
        : print(tag), od(od), in_group(false) {}
    printable_option() : in_group(false) {}
  };
  typedef std::vector<printable_option> options_type;
  BOOST_STATIC_CONSTANT(unsigned, default_linewrap = 80);  // options_description::default_line_length_
  printable_options_description(unsigned line_length = default_linewrap) : options_description(line_length) {
    init();
  }

  typedef boost::object_pool<std::string> string_pool;
  printable_options_description(const std::string& caption, unsigned line_length = default_linewrap)
      : options_description(caption, line_length), caption(caption) {
    init();
  }

  void init() {
    n_this_level = 0;
    n_nonempty_groups = 0;
    descs.reset(new string_pool());
  }

  self_type& add_options() { return *this; }

  typedef std::string option_name;

  template <class V>
  self_type& option(option_name name, V* val, std::string const& description, bool hidden = false,
                    bool defaulted = false, option_options<V> const& opt = option_options<V>()) {
    return (*this)(name, via_strings_value(name, val, opt, defaulted), description);
  }

  template <class V>
  self_type& option(option_name name, V* val, std::string const& description, option_options<V> const& opt) {
    return (*this)(name, via_strings_value(name, val, opt, opt.defaulted), description, opt.hidden);
  }


  template <class V>
  self_type& defaulted(option_name name, V* val, std::string const& description, bool hidden = false,
                       option_options<V> const& opt = option_options<V>()) {
    return option(name, val, description, hidden, true, opt);
  }

  template <class V>
  self_type& required(option_name name, V* val, std::string const& description, bool hidden = false,
                      bool defaulted = false) {
    return required_flag(true, name, val, description, hidden, defaulted);
  }

  template <class V>
  self_type& required_flag(bool required, option_name name, V* val, std::string const& description,
                           bool hidden = false, bool defaulted = false) {
    option_options<V> opt;
    opt.required = required;
    return option(name, val, description, hidden, defaulted, opt);
  }

  boost::shared_ptr<string_pool> descs;  // because opts lib only takes char *, hold them here.
  template <class T, class C>
  self_type& operator()(char const* name, boost::program_options::typed_value<T, C>* val,
                        std::string const& description, bool hidden = false) {
    return (*this)(name, val, cstr(description), hidden);
  }

  char const* cstr(std::string const& s) { return descs->construct(s)->c_str(); }

  template <class T, class C>
  self_type& operator()(std::string const& name, boost::program_options::typed_value<T, C>* val,
                        std::string const& description, bool hidden = false) {
    return (*this)(cstr(name), val, cstr(description), hidden);
  }

  std::size_t n_this_level, n_nonempty_groups;
  template <class T, class C>
  self_type& operator()(char const* name, boost::program_options::typed_value<T, C>* val,
                        char const* description = NULL, bool hidden = false) {
    SHOWIF3(GPROGOPT, 1, "adding", this, name, hidden);
    ++n_this_level;
    printable_option opt((T*)0, simple_add(name, val, description));
    if (!hidden) pr_options.push_back(opt);
    return *this;
  }


  // options tree (affects printing only, not option names)
  self_type& add(self_type const& desc, bool hidden = false) {
    options_description::add(desc);
    if (hidden) return *this;
    groups.push_back(group_type(new self_type(desc)));
    if (desc.size()) {
      for (typename options_type::const_iterator i = desc.pr_options.begin(), e = desc.pr_options.end();
           i != e; ++i) {
        pr_options.push_back(*i);
        pr_options.back().in_group = true;
      }
      ++n_nonempty_groups;  // could just not add an empty group. but i choose to allow that.
    }

    return *this;
  }

  void print_option(Ostream& o, printable_option const& opt,
                    boost::program_options::variable_value const& var, bool only_value = false) const {
    using namespace boost;
    using namespace boost::program_options;
    using namespace std;
    string const& name = opt.name();
    if (!only_value) {
      boost::shared_ptr<value_semantic const> psemantic(opt.od->semantic());
      if (psemantic && psemantic->is_required()) o << "#REQUIRED# ";
      if (var.defaulted()) o << "#DEFAULTED# ";
      if (var.empty()) {
        o << "#EMPTY# " << name;
        return;
      }
      o << name << " = ";
    }
    opt.print(o, var.value());
  }


  typedef std::vector<printable_option> option_set;

  // yield a list of options that were specified as defaulted (as opposed to optional args that would have
  // stored to an object that has a default value
  void collect_defaulted(boost::program_options::variables_map& vm, option_set& defaults) {
    for (typename options_type::iterator i = pr_options.begin(), e = pr_options.end(); i != e; ++i) {
      printable_option& opt = *i;
      if (vm[opt.vmkey()].defaulted()) defaults.push_back(opt);
    }
    for (typename groups_type::iterator i = groups.begin(), e = groups.end(); i != e; ++i) {
      (*i)->collect_defaulted(vm, defaults);
    }
  }

  bool validate(boost::program_options::variables_map& vm) {
    // was going to check that required options were set here; it turns out that the built in ->defaulted()
    // does this for me
    return true;
  }

  void print(Ostream& o, boost::program_options::variables_map const& vm,
             int show_flags = SHOW_DESCRIPTION | SHOW_DEFAULTED | SHOW_HIERARCHY) const {
    const bool show_defaulted = bool(show_flags & SHOW_DEFAULTED);
    const bool show_description = bool(show_flags & SHOW_DESCRIPTION);
    const bool hierarchy = bool(show_flags & SHOW_HIERARCHY);
    const bool show_empty = bool(show_flags & SHOW_EMPTY);
    const bool show_help = bool(show_flags & SHOW_HELP);
    const bool show_empty_groups = bool(show_flags & SHOW_EMPTY_GROUPS);

    using namespace boost::program_options;
    using namespace std;
    if (show_empty_groups || n_this_level || n_nonempty_groups > 1) o << "### " << caption << endl;
    for (typename options_type::const_iterator i = pr_options.begin(), e = pr_options.end(); i != e; ++i) {
      printable_option const& opt = *i;
      if (!show_help && opt.name() == "help") continue;
      if (hierarchy and opt.in_group) continue;
      variable_value const& var = vm[opt.vmkey()];
      if (var.defaulted() && !show_defaulted) continue;
      if (var.empty() && !show_empty) continue;
      if (show_description) o << "# " << opt.description() << endl;
      print_option(o, opt, var);
      o << endl;
    }
    o << endl;
    if (hierarchy)
      for (typename groups_type::const_iterator i = groups.begin(), e = groups.end(); i != e; ++i)
        if (show_empty_groups || (*i)->size()) (*i)->print(o, vm, show_flags);
  }

  typedef std::vector<std::string> unparsed_args;

  boost::program_options::parsed_options parse_options(
      std::vector<std::string> const& args, boost::program_options::positional_options_description* po = NULL,
      unparsed_args* unparsed_out = NULL, bool allow_unrecognized_positional = false,
      bool allow_unrecognized_opts = false, std::string const& argv0 = "command-line-options") {
    using namespace std;
    int n = (int)args.size();
    int argc = n + 1;
    boost::scoped_array<char*> argv((new char* [argc]));
    argv[0] = (char*)argv0.c_str();
    SHOWIF1(GPROGOPT, 1, "parse_options", to_string(args));
    for (int i = 0; i < n; ++i) argv[i + 1] = (char*)(args[i].c_str());
    return parse_options(argc, argv.get(), po, unparsed_out, allow_unrecognized_opts);
  }

  static inline bool is_option(std::string const& arg) { return !arg.empty() && arg[0] == '-'; }

  // remember to call store(return, vm) and notify(vm)
  boost::program_options::parsed_options
  parse_options(int argc, char* argv[], boost::program_options::positional_options_description* po = NULL,
                unparsed_args* unparsed_out = NULL, bool allow_unrecognized_opts = false,
                bool allow_unrecognized_positional = false, unparsed_args* all_positional_out = NULL) {
    using namespace boost::program_options;
    command_line_parser cl(argc, const_cast<char**>(argv));
    cl.options(*this);
    if (po) cl.positional(*po);
    if (allow_unrecognized_opts) cl.allow_unregistered();
    parsed_options parsed = cl.run();
    std::vector<std::string> unparsed_no_positional = collect_unrecognized(parsed.options, exclude_positional);
    // this is broken in that it includes *registered* positional options. workaround is to check for first
    // char is '-'
    std::vector<std::string> unparsed_with_positional
        = collect_unrecognized(parsed.options, include_positional);
    std::vector<std::string> just_positional;
    for (std::size_t i = 0, n = unparsed_with_positional.size(); i < n; ++i) {
      std::string const& arg = unparsed_with_positional[i];
      if (arg.empty() || arg[0] != '-') just_positional.push_back(arg);
      if (!allow_unrecognized_positional) {
        if (!po || po->max_total_count() < just_positional.size())
          program_options_fatal("Excessive positional (non-option) arguments: " + to_string(just_positional));
      }
    }

    if (!allow_unrecognized_opts && !unparsed_no_positional.empty())
      program_options_fatal("Unrecognized option in: " + to_string(unparsed_no_positional));
    if (unparsed_out) unparsed_out->swap(unparsed_no_positional);
    if (all_positional_out) all_positional_out->swap(just_positional);
    return parsed;
  }

  /// parses arguments, then stores/notifies from opts->vm.  returns unparsed
  /// options and positional arguments, but if not empty, throws exception unless
  /// allow_unrecognized_positional is true
  std::vector<std::string>
  parse_options_and_notify(int argc, char* argv[], boost::program_options::variables_map& vm,
                           boost::program_options::positional_options_description* po = NULL,
                           bool allow_unrecognized_positional = false, bool allow_unrecognized_opts = false) {
    unparsed_args r;
    boost::program_options::store(
        parse_options(argc, argv, po, &r, allow_unrecognized_positional, allow_unrecognized_opts), vm);
    notify(vm);
    return r;
  }

  std::size_t ngroups() const { return groups.size(); }
  std::size_t size() const { return pr_options.size(); }

 private:
  groups_type groups;
  options_type pr_options;
  std::string caption;
  boost::shared_ptr<option_description> simple_add(const char* name,
                                                   const boost::program_options::value_semantic* s,
                                                   const char* description = NULL) {
    typedef option_description OD;
    boost::shared_ptr<OD> od((description ? new OD(name, s, description) : new OD(name, s)));
    options_description::add(od);
    return od;
  }
};

typedef printable_options_description<std::ostream> printable_opts;

// since this can't be found by ADL for many containers, use the macro
// PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(container<X>) below to place it as boost::program_options::validate
template <class C, class charT>
void validate_collection(boost::any& v, const std::vector<std::basic_string<charT> >& s, C*, int) {
  if (v.empty()) v = boost::any(C());
  C* tv = boost::any_cast<C>(&v);
  assert(tv);
  for (std::size_t i = 0, e = s.size(); i < e; ++i) {
    try {
      boost::any a;
      std::vector<std::basic_string<charT> > cv;
      cv.push_back(s[i]);
      typedef typename boost::range_value<C>::type V;
      validate(a, cv, (V*)0, 0);
      add(*tv, boost::any_cast<V>(a));
    } catch (boost::bad_lexical_cast& /*e*/) {
      program_options_fatal("Couldn't validate option value: " + s[i]);
    }
  }
}

}  // graehl


/** Validates sequences. Allows multiple values per option occurrence
    and multiple occurrences. boost only provides std::vector. */

// use these macros in global namespace. note that the preprocessor does not understand commas inside <> so
// you may need to use extra parens or a comma macro
// e.g. PROGRAM_OPTIONS_FOR_CONTAINER((std::map<int, int>))
#define PROGRAM_OPTIONS_FOR_CONTAINER(ContainerFullyQualified)                                              \
  namespace boost {                                                                                         \
  namespace program_options {                                                                               \
  template <class charT>                                                                                    \
  void validate(boost::any& v, const std::vector<std::basic_string<charT> >& s, ContainerFullyQualified* f, \
                int i) {                                                                                    \
    graehl::validate_collection(v, s, f, i);                                                                \
  }                                                                                                         \
  }                                                                                                         \
  }

// e.g. PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(class T, std::vector<T>)
#define PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(TemplateArgs, ContainerTemplate)                                \
  namespace boost {                                                                                            \
  namespace program_options {                                                                                  \
  template <TemplateArgs, class charT>                                                                         \
  void validate(boost::any& v, const std::vector<std::basic_string<charT> >& s, ContainerTemplate* f, int i) { \
    graehl::validate_collection(v, s, f, i);                                                                   \
  }                                                                                                            \
  }                                                                                                            \
  }

#ifndef TEMPLATE_COMMA
#define TEMPLATE_COMMA ,
#endif

PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(class T TEMPLATE_COMMA class A, std::deque<T TEMPLATE_COMMA A>)

PROGRAM_OPTIONS_FOR_CONTAINER_TEMPLATE(class T TEMPLATE_COMMA class A, std::list<T TEMPLATE_COMMA A>)

// boost program options already provides vector

#ifndef GRAEHL_PROGRAM_OPTIONS_SAMPLE
#define GRAEHL_PROGRAM_OPTIONS_SAMPLE 0
#endif

#if GRAEHL_PROGRAM_OPTIONS_SAMPLE
#include <iostream>
using namespace std;
using namespace graehl;
using namespace boost;
using namespace boost::program_options;
int main(int argc, char* argv[]) {
  boost::program_options::variables_map vm;
  printable_options_description<ostream> all_options;
  int i = 3;
  std::string k = "4";
  std::vector<std::string> js;
  all_options.defaulted("in,i", &i, "i");
  all_options.defaulted("kstr,k", &k, "str");
  all_options.option("js,j", &js, "js (multiple occurences of str)");
  cout << "USAGE:\n" << all_options << "\n";
  try {
    parsed_options po = all_options.parse_options_and_notify(argc, argv);
    store(po, vm);
    cout << "i=" << i << " #js=" << js.size() << "\n";
    all_options.print(cout, vm);
  } catch (std::exception& e) {
    cerr << "ERROR: " << e.what() << "\n";
  }
}
#endif

#endif
