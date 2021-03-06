/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include "unicode.h"

#include <cstring>
#include <locale>
#include <iomanip>
#include <sstream>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

/// Determine endianness of the architecture
/// \return True if the architecture is little_endian
bool is_little_endian_arch()
{
  uint32_t i=1;
  return reinterpret_cast<uint8_t &>(i);
}

#define BUFSIZE 100

std::string narrow(const wchar_t *s)
{
  #ifdef _WIN32

  int slength=static_cast<int>(wcslen(s));
  int rlength=
    WideCharToMultiByte(CP_UTF8, 0, s, slength, NULL, 0, NULL, NULL);
  std::string r(rlength, 0);
  WideCharToMultiByte(CP_UTF8, 0, s, slength, &r[0], rlength, NULL, NULL);
  return r;

  #else
  // dummy conversion
  std::string r;
  r.reserve(wcslen(s));
  while(*s!=0)
  {
    r+=static_cast<char>(*s);
    s++;
  }

  return r;
  #endif
}

std::wstring widen(const char *s)
{
  #ifdef _WIN32

  int slength=static_cast<int>(strlen(s));
  int rlength=
    MultiByteToWideChar(CP_UTF8, 0, s, slength, NULL, 0);
  std::wstring r(rlength, 0);
  MultiByteToWideChar(CP_UTF8, 0, s, slength, &r[0], rlength);
  return r;

  #else
  // dummy conversion
  std::wstring r;
  r.reserve(strlen(s));
  while(*s!=0)
  {
    r+=wchar_t(*s);
    s++;
  }

  return r;
  #endif
}

std::string narrow(const std::wstring &s)
{
  #ifdef _WIN32

  int slength=static_cast<int>(s.size());
  int rlength=
    WideCharToMultiByte(CP_UTF8, 0, &s[0], slength, NULL, 0, NULL, NULL);
  std::string r(rlength, 0);
  WideCharToMultiByte(CP_UTF8, 0, &s[0], slength, &r[0], rlength, NULL, NULL);
  return r;

  #else
  // dummy conversion
  return std::string(s.begin(), s.end());
  #endif
}

std::wstring widen(const std::string &s)
{
  #ifdef _WIN32

  int slength=static_cast<int>(s.size());
  int rlength=
    MultiByteToWideChar(CP_UTF8, 0, &s[0], slength, NULL, 0);
  std::wstring r(rlength, 0);
  MultiByteToWideChar(CP_UTF8, 0, &s[0], slength, &r[0], rlength);
  return r;

  #else
  // dummy conversion
  return std::wstring(s.begin(), s.end());
  #endif
}

/// Appends a unicode character to a utf8-encoded string
/// \par parameters: character to append, string to append to
static void utf8_append_code(unsigned int c, std::string &result)
{
  if(c<=0x7f)
    result+=static_cast<char>(c);
  else if(c<=0x7ff)
  {
    result+=static_cast<char>((c >> 6)   | 0xc0);
    result+=static_cast<char>((c &0x3f) | 0x80);
  }
  else if(c<=0xffff)
  {
    result+=static_cast<char>((c >> 12)         | 0xe0);
    result+=static_cast<char>(((c >> 6) &0x3f) | 0x80);
    result+=static_cast<char>((c &0x3f)        | 0x80);
  }
  else
  {
    result+=static_cast<char>((c >> 18)         | 0xf0);
    result+=static_cast<char>(((c >> 12) &0x3f)| 0x80);
    result+=static_cast<char>(((c >> 6) &0x3f) | 0x80);
    result+=static_cast<char>((c &0x3f)        | 0x80);
  }
}

/// \param utf32:encoded wide string
/// \return utf8-encoded string with the same unicode characters as the input.
std::string utf32_to_utf8(const std::basic_string<unsigned int> &s)
{
  std::string result;

  result.reserve(s.size()); // at least that long

  for(const auto c : s)
    utf8_append_code(c, result);

  return result;
}

const char **narrow_argv(int argc, const wchar_t **argv_wide)
{
  if(argv_wide==nullptr)
    return nullptr;

  // the following never gets deleted
  const char **argv_narrow=new const char *[argc+1];
  argv_narrow[argc]=nullptr;

  for(int i=0; i<argc; i++)
    argv_narrow[i]=strdup(narrow(argv_wide[i]).c_str());

  return argv_narrow;
}

/// A helper function for dealing with different UTF16 endians
/// \par parameters: A 16-bit integer
/// \return A 16-bit integer with bytes swapped
uint16_t do_swap_bytes(uint16_t x)
{
  uint16_t b1=x & 0xFF;
  uint16_t b2=x & 0xFF00;
  return (b1 << 8) | (b2 >> 8);
}


void utf16_append_code(unsigned int code, bool swap_bytes, std::wstring &result)
{
  // we do not treat 0xD800 to 0xDFFF, although
  // they are not valid unicode symbols

  if(code<0xFFFF)
  { // code is encoded as one UTF16 character
    // we just take the code and possibly swap the bytes
    unsigned int a=(swap_bytes)?do_swap_bytes(code):code;
    result+=static_cast<wchar_t>(a);
  }
  else // code is encoded as two UTF16 characters
  {
    // if this is valid unicode, we have
    // code<0x10FFFF
    // but let's not check it programmatically

    // encode the code in UTF16, possibly swapping bytes.
    code=code-0x10000;
    unsigned int i1=((code>>10) & 0x3ff) | 0xD800;
    unsigned int a1=(swap_bytes)?do_swap_bytes(static_cast<uint16_t>(i1)):i1;
    result+=static_cast<wchar_t>(a1);
    unsigned int i2=(code & 0x3ff) | 0xDC00;
    unsigned int a2=(swap_bytes)?do_swap_bytes(static_cast<uint16_t>(i2)):i2;
    result+=static_cast<wchar_t>(a2);
  }
}


/// \par parameters: String in UTF-8 format, bool value indicating whether the
/// endianness should be different from the architecture one.
/// \return String in UTF-16 format. The encoding follows the endianness of the
///   architecture iff swap_bytes is true.
std::wstring utf8_to_utf16(const std::string& in, bool swap_bytes)
{
    std::wstring result;
    result.reserve(in.size());
    std::string::size_type i=0;
    while(i<in.size())
    {
      unsigned char c=in[i++];
      unsigned int code=0;
      // the ifs that follow find out how many UTF8 characters (1-4) store the
      // next unicode character. This is determined by the few most
      // significant bits.
      if(c<=0x7F)
      {
        // if it's one character, then code is exactly the value
        code=c;
      }
      else if(c<=0xDF && i<in.size())
      { // in other cases, we need to read the right number of chars and decode
        // note: if we wanted to make sure that we capture incorrect strings,
        // we should check that whatever follows first character starts with
        // bits 10.
        code=(c & 0x1F) << 6;
        c=in[i++];
        code+=c  & 0x3F;
      }
      else if(c<=0xEF && i+1<in.size())
      {
        code=(c & 0xF) << 12;
        c=in[i++];
        code+=(c & 0x3F) << 6;
        c=in[i++];
        code+=c & 0x3F;
      }
      else if(c<=0xF7 && i+2<in.size())
      {
        code=(c & 0x7) << 18;
        c=in[i++];
        code+=(c & 0x3F) << 12;
        c=in[i++];
        code+=(c & 0x3F) << 6;
        c=in[i++];
        code+=c & 0x3F;
      }
      else
      {
        // The string is not a valid UTF8 string! Either it has some characters
        // missing from a multi-character unicode symbol, or it has a char with
        // too high value.
        // For now, let's replace the character with a space
        code=32;
      }

      utf16_append_code(code, swap_bytes, result);
    }

    return result;
}

/// \par parameters: String in UTF-8 format
/// \return String in UTF-16BE format
std::wstring utf8_to_utf16_big_endian(const std::string& in)
{
  bool swap_bytes=is_little_endian_arch();
  return utf8_to_utf16(in, swap_bytes);
}

/// \par parameters: String in UTF-8 format
/// \return String in UTF-16LE format
std::wstring utf8_to_utf16_little_endian(const std::string& in)
{
  bool swap_bytes=!is_little_endian_arch();
  return utf8_to_utf16(in, swap_bytes);
}

/// \par parameters: String in UTF-16LE format
/// \return String in US-ASCII format, with \uxxxx escapes for other characters
std::string utf16_little_endian_to_ascii(const std::wstring& in)
{
  std::ostringstream result;
  std::locale loc;
  for(const auto c : in)
  {
    if(c<=255 && isprint(c, loc))
      result << (unsigned char)c;
    else
    {
      result << "\\u"
             << std::hex
             << std::setw(4)
             << std::setfill('0')
             << (unsigned int)c;
    }
  }
  return result.str();
}
