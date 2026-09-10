#pragma once

#include <iosfwd>
#include <string>
#include <sstream>
#include <variant>
#include <stdexcept>
#include <optional>
#include <vector>
#include <iostream>
#include <stack>

class TestRunner;

namespace Parse {

    namespace TokenType {
      struct Number {
        int value;
      };

      struct Id {
        std::string value;
      };

      struct Char {
        char value;
      };

      struct String {
        std::string value;
      };

      struct Class{};
      struct Return{};
      struct If {};
      struct Else {};
      struct Def {};
      struct Newline {};
      struct Print {};
      struct Indent {};
      struct Dedent {};
      struct Eof {};
      struct And {};
      struct Or {};
      struct Not {};
      struct Eq {};
      struct NotEq {};
      struct LessOrEq {};
      struct GreaterOrEq {};
      struct None {};
      struct True {};
      struct False {};
    }

using TokenBase = std::variant<
  TokenType::Number,
  TokenType::Id,
  TokenType::Char,
  TokenType::String,
  TokenType::Class,
  TokenType::Return,
  TokenType::If,
  TokenType::Else,
  TokenType::Def,
  TokenType::Newline,
  TokenType::Print,
  TokenType::Indent,
  TokenType::Dedent,
  TokenType::And,
  TokenType::Or,
  TokenType::Not,
  TokenType::Eq,
  TokenType::NotEq,
  TokenType::LessOrEq,
  TokenType::GreaterOrEq,
  TokenType::None,
  TokenType::True,
  TokenType::False,
  TokenType::Eof
>;

enum class Context {
    Program = 0,
    Class = 1,
    If = 2,
    Else = 3,
    Def = 4,
};

//По сравнению с условием задачи мы добавили в тип Token несколько
//удобных методов, которые делают код короче. Например,
//
//token.Is<TokenType::Id>()
//
//гораздо короче, чем
//
//std::holds_alternative<TokenType::Id>(token).
struct Token : TokenBase {
  using TokenBase::TokenBase;

  template <typename T>
  bool Is() const {
    return std::holds_alternative<T>(*this);
  }

  template <typename T>
  const T& As() const {
    return std::get<T>(*this);
  }

  template <typename T>
  const T* TryAs() const {
    return std::get_if<T>(this);
  }
};

bool operator == (const Token& lhs, const Token& rhs);
std::ostream& operator << (std::ostream& os, const Token& rhs);

class LexerError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class Lexer {
public:
  explicit Lexer(std::istream& input);

  const Token& CurrentToken() const;
  Token NextToken();

  template <typename T>
  const T& Expect() const {
      if(current_token.Is<T>()){
          return current_token.As<T>();
      }
      throw LexerError("Expect()");
  }

  template <typename T, typename U>
  void Expect(const U& value) const {
      if(current_token.Is<T>()){
          const T& token = current_token.As<T>();
          if(token.value == value){
              return;
          }
      }
      throw LexerError("Expect(const U& value)");
  }

  template <typename T>
  const T& ExpectNext() {
      Token next_token = NextToken();
      return Expect<T>();
  }

  template <typename T, typename U>
  void ExpectNext(const U& value) {
      Token next_token = NextToken();
      if(current_token.Is<T>()){
          const T& token = current_token.As<T>();
          if(token.value == value){
              return;
          }
      }
      throw LexerError("ExpectNext(const U& value)");
  }

private:
    std::istream& input_stream;
    std::vector<Token> tokens;
    Token current_token;
    
    size_t current_indent_count{0};
    std::string line;
    bool need_check_indent = false;

    Context current_context;
    std::stack<Context> context_stack;
};

void RunLexerTests(TestRunner& test_runner);

} /* namespace Parse */
