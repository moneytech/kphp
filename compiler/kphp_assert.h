#pragma once

#define compiler_assert_(x, y, level, unreachable)  ({          \
  int kphp_error_res__ = 0;                                     \
  if (!(x)) {                                                   \
    kphp_error_res__ = 1;                                       \
    on_compilation_error (#x, __FILE__, __LINE__, y, level);    \
    unreachable;                                                \
  }                                                             \
  kphp_error_res__;                                             \
})

#define compiler_assert(x, y, level) compiler_assert_(x, y, level,)
#define compiler_assert_noret(x, y, level) compiler_assert_(x, y, level, __builtin_unreachable())

#define kphp_warning(y)  compiler_assert (0, y, WRN_ASSERT_LEVEL)
#define kphp_typed_warning(x, y) do {                                      \
  FunctionPtr kphp_warning_fun__ = stage::get_function();                  \
  if (kphp_warning_fun__) {                                                \
    auto &disabled__ = kphp_warning_fun__->disabled_warnings;              \
    string s = x;                                                          \
    if (disabled__.find(s) == disabled__.end()) {                          \
      string message = y;                                                  \
      message += "\n   (Can be disabled by adding '" + s +                 \
                 "' to @kphp-disable-warnings attribute of function)";     \
      compiler_assert (0, message.c_str(), WRN_ASSERT_LEVEL);              \
    }                                                                      \
  }                                                                        \
} while (0)


#define kphp_error(x, y) compiler_assert (x, y, CE_ASSERT_LEVEL)
#define kphp_error_act(x, y, act) if (kphp_error (x, y)) act;
#define kphp_error_return(x, y) kphp_error_act (x, y, return)
#define kphp_assert(x) compiler_assert_noret (x, "Assertion "  #x " failed", FATAL_ASSERT_LEVEL)
#define kphp_assert_msg(x, y) compiler_assert_noret (x, y, FATAL_ASSERT_LEVEL)
#define kphp_fail_msg(y) kphp_assert_msg (0, y); _exit(1);
#define kphp_fail() kphp_assert (0); _exit(1);

enum AssertLevelT {
  WRN_ASSERT_LEVEL,
  CE_ASSERT_LEVEL,
  FATAL_ASSERT_LEVEL
};

void on_compilation_error(const char *description, const char *file_name, int line_number,
                          const char *full_description, AssertLevelT assert_level);

void on_compilation_error(const char *description, const char *file_name, int line_number, const std::string &full_description, AssertLevelT assert_level);
