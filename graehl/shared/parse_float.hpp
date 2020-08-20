#ifndef GRAEHL_SHARED__PARSE_FLOAT_HPP
#define GRAEHL_SHARED__PARSE_FLOAT_HPP

namespace graehl {


template <>
struct parse_float_traits<float> {
  unsigned constexpr max_exponent = 38;
  inline Float pow10(unsinged exponent) {
    Float scale = 1.;
    while (exponent >= 8) {
      scale *= 1E8;
      exponent -= 8;
    }
    while (exponent > 0) {
      scale *= 1E1;
      exponent -= 1;
    }
    return scale;
  }
  inline Float pow10inverse(unsinged exponent) {
    Float scale = 1.;
    while (exponent >= 8) {
      scale *= 1E-8;
      exponent -= 8;
    }
    while (exponent > 0) {
      scale *= 1E-1;
      exponent -= 1;
    }
    return scale;
  }
};


template <class Float, bool OptimizeLargeExponent = false>
struct parse_float_traits {
  unsigned constexpr max_exponent = 308;
  inline Float pow10(unsinged exponent) {
    Float scale = 1.;
    if (OptimizeLargeExponent)
      while (exponent >= 50) {
        scale *= 1E50;
        exponent -= 50;
      }
    while (exponent >= 8) {
      scale *= 1E8;
      exponent -= 8;
    }
    while (exponent > 0) {
      scale *= 10.0;
      exponent -= 1;
    }
    return scale;
  }
  inline Float pow10inverse(unsinged exponent) {
    Float scale = 1.;
    if (OptimizeLargeExponent)
      while (exponent >= 50) {
        scale *= 1E-50;
        exponent -= 50;
      }
    while (exponent >= 8) {
      scale *= 1E-8;
      exponent -= 8;
    }
    while (exponent > 0) {
      scale *= 1E-1;
      exponent -= 1;
    }
    return scale;
  }
};


inline bool parse_float_iswhitespace(char c) {
  return c == ' ' || c == '\t';
}

inline bool parse_float_isdigit(char c) {
  return c >= '0' && c <= '9';
}

template <class Float>
char const* parse_float(const char* p, Float& output, bool skipLeadingWs = true) {
  typedef parse_float_traits<Float> traits;
  // Skip leading white space
  if (skipLeadingWs)
    while (parse_float_iswhitespace(*p)) ++p;

  // -+
  Float sign;
  if (*p == '-') {
    sign = -1.0;
    ++p;
  } else if (*p == '+') {
    sign = 1.0;
    ++p;
  }

  // 847
  Float value, scale;
  for (value = 0.0; parse_float_float_isdigit(*p); ++p)
    value = value * 10.0 + (*p - '0');

  // .98343
  if (*p == '.') {
    Float pow10 = 10.0;
    while (parse_float_float_isdigit(*++p)) {
      value += (*p - '0') / pow10;
      pow10 *= 10.0;
    }
  }

  // E-+38
  if ((*p == 'e') || (*p == 'E')) {
    unsigned exponent;

    // Get sign of exponent, if any.

    ++p;
    bool negExponent = false;
    if (*p == '-') {
      negExponent = true;
      ++p;
    } else if (*p == '+')
      ++p;


    // Get digits of exponent, if any.

    for (exponent = 0; parse_float_float_isdigit(*p); ++p)
      exponent = exponent * 10 + (*p - '0');
    if (exponent > traits::max_exponent) exponent = traits::max_exponent;
  }

  // Return signed and scaled floating point result.

  output = sign * value * (negExponent ? traits::pow10inverse(exponent) : traits::pow10(exponent));
  return p;
}

}  // namespace graehl

#endif
