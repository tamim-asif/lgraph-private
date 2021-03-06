#ifndef EPRP_SCANNER_H
#define EPRP_SCANNER_H

#include <stdint.h>

#include <cassert>
#include <string>
#include <vector>

#include "spdlog/spdlog.h"

#define MIN_CHUNK_SIZE 4

// No token
#define TOK_NOP        0x00
// # foo
#define TOK_COMMENT    0x01
// @asd @_asd
#define TOK_REGISTER   0x03
// |>
#define TOK_PIPE       0x04
// a..z..0..9.._
#define TOK_ALNUM      0x05
// {
#define TOK_OB         0x06
// }
#define TOK_CB         0x07
// :
#define TOK_COLON      0x08
// >
#define TOK_GT         0x09
// |
#define TOK_OR         0x0a
// @
#define TOK_AT         0x0b
// foo:
#define TOK_LABEL      0x0c
// %outasd
#define TOK_OUTPUT     0x0d
// $asd
#define TOK_INPUT      0x0e
// $
#define TOK_DOLLAR     0x0f
// %
#define TOK_PERCENT    0x10
// .
#define TOK_DOT        0x11
// /
#define TOK_DIV        0x12

#define TOK_TRYMERGE   0x80

class Eprp_scanner {
protected:
  typedef uint8_t Token_id;

  struct Token {
    Token_id tok; // Token (identifier, if, while...)
    uint32_t pos; // Position in buffer
    uint16_t len; // length in buffer
  };
  typedef std::vector<Token> Token_list;

private:
  std::vector<Token_id> translate;
  bool token_list_spaced = true;
  Token_list token_list;

  // Fields updated for each chunk processed
  size_t scanner_pos;
  std::string buffer_name;
  const char *buffer;
  uint32_t buffer_sz; // Token.pos is uint32_t to save space. 4GB per chunk...
  size_t buffer_start_pos;
  size_t buffer_start_line;

  void setup_translate();

  void add_token(Token t);

  void chunked(const char *_buffer, size_t _buffer_sz, size_t _buffer_start_pos, size_t _buffer_start_line);

  void scan_raw_msg(const std::string &cat, const std::string &text, bool third) const;
public:
  Eprp_scanner();

  void scan_error(const std::string &text) const { scan_raw_msg("error", text, true); };
  void scan_warn(const std::string &text) const { scan_raw_msg("warning", text, true); };

  void parser_error(const std::string &text) const { scan_raw_msg("error", text, false); };
  void parser_warn(const std::string &text) const { scan_raw_msg("warning", text, false); };

  bool scan_next();

  bool scan_is_end() const {
    return scanner_pos >= token_list.size();
  }

  bool scan_is_token(Token_id tok) const {
    if(scanner_pos< token_list.size())
      return token_list[scanner_pos].tok == tok;
    return false;
  }

  void scan_append(std::string &text) const;
  std::string scan_text() const;
  int scan_calc_lineno() const;

  bool scan_is_prev_token(Token_id tok) const {
    if (scanner_pos==0)
      return false;
    assert(scanner_pos< token_list.size());
    return token_list[scanner_pos-1].tok == tok;
  }
  bool scan_is_next_token(Token_id tok) const {
    if (scanner_pos>=token_list.size())
      return false;
    return token_list[scanner_pos+1].tok == tok;
  }

  void parse(std::string name, const char *memblock, size_t sz);
  void parse(std::string name, const std::string &str) {
    parse(name,str.c_str(),str.size());
  }

  virtual void elaborate() = 0;
};

#endif
