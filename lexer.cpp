#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include "test_runner.h"

using namespace std;

namespace Parse {

bool operator == (const Token& lhs, const Token& rhs) {
  using namespace TokenType;

  if (lhs.index() != rhs.index()) {
    return false;
  }
  if (lhs.Is<Char>()) {
    return lhs.As<Char>().value == rhs.As<Char>().value;
  } else if (lhs.Is<Number>()) {
    return lhs.As<Number>().value == rhs.As<Number>().value;
  } else if (lhs.Is<String>()) {
    return lhs.As<String>().value == rhs.As<String>().value;
  } else if (lhs.Is<Id>()) {
    return lhs.As<Id>().value == rhs.As<Id>().value;
  } else {
    return true;
  }
}

std::ostream& operator << (std::ostream& os, const Token& rhs) {
  using namespace TokenType;

#define VALUED_OUTPUT(type) \
  if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

  VALUED_OUTPUT(Number);
  VALUED_OUTPUT(Id);
  VALUED_OUTPUT(String);
  VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

  UNVALUED_OUTPUT(Class);
  UNVALUED_OUTPUT(Return);
  UNVALUED_OUTPUT(If);
  UNVALUED_OUTPUT(Else);
  UNVALUED_OUTPUT(Def);
  UNVALUED_OUTPUT(Newline);
  UNVALUED_OUTPUT(Print);
  UNVALUED_OUTPUT(Indent);
  UNVALUED_OUTPUT(Dedent);
  UNVALUED_OUTPUT(And);
  UNVALUED_OUTPUT(Or);
  UNVALUED_OUTPUT(Not);
  UNVALUED_OUTPUT(Eq);
  UNVALUED_OUTPUT(NotEq);
  UNVALUED_OUTPUT(LessOrEq);
  UNVALUED_OUTPUT(GreaterOrEq);
  UNVALUED_OUTPUT(None);
  UNVALUED_OUTPUT(True);
  UNVALUED_OUTPUT(False);
  UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

  return os << "Unknown token :(";
}

Lexer::Lexer(std::istream& input): input_stream(input), current_token(TokenType::None{}),
        current_context(Context::Program) {
    NextToken();
}

const Token& Lexer::CurrentToken() const {
    return current_token;
}

Token Lexer::NextToken() {
    if(current_token.Is<TokenType::Eof>()){
        return TokenType::Eof();
    }

    if(line.empty()){
        std::string s;
        while(std::getline(input_stream, s)){
            if(auto it = std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }); it != s.end()){
                break;
            }
        }
        
        if(s.empty() && current_indent_count > 0){
            current_token = TokenType::Dedent{};
            tokens.push_back(current_token);
            current_indent_count--;
            return current_token;
        } else if(s.empty()){
            current_token = TokenType::Eof{};
            tokens.push_back(current_token);
            return current_token;
        }
        
        if(s[s.size()-1] != '\n'){
            s += '\n';
        }

        need_check_indent = true;
        line = s;
    }
    
    auto it = std::find_if(line.begin(), line.end(), [](unsigned char c){
        return !std::isspace(c);
    });
    
    if(need_check_indent){
        if(it == line.end()){
            throw LexerError("Invalid line. Expect not space symbol.");
        }
        
        size_t d = std::distance(line.begin(), it);
        if(d % 2 == 1){
            throw LexerError("Invalid indent - d % 2 == 1");
        }
        
        if(current_indent_count < d/2){
            current_token = TokenType::Indent{};
            tokens.push_back(current_token);
            current_indent_count++;
            return current_token;
        } else if(current_indent_count > d/2){
            current_token = TokenType::Dedent{};
            tokens.push_back(current_token);
            current_indent_count--;
            return current_token;
        }
        
        line = line.substr(d);
        need_check_indent = false;
    } else {
        if(it == line.end()){
            current_token = TokenType::Newline{};
            tokens.push_back(current_token);
            line = "";
            
            return current_token;
        } else {
            size_t x = std::distance(line.begin(), it);
            if(x > 0){
                line = line.substr(x);
            }
        }
    }
    
    if(line[0] == '"' || line[0] == '\''){
        char c = line[0];
        size_t pos = line.find(c, 1);
        if(pos == std::string::npos){
            throw LexerError("Invalid string");
        }
        
        current_token = TokenType::String{line.substr(1, pos-1)};
        tokens.push_back(current_token);
        line = line.substr(pos+1);
        
        return current_token;
    }
    
    if(line.size() > 1){
        if(line[0] == '!' && line[1] == '=') {
            current_token = TokenType::NotEq{};
            tokens.push_back(current_token);
            line = line.substr(2);
            
            return current_token;
        }
        
        if(line[0] == '=' && line[1] == '=') {
            current_token = TokenType::Eq{};
            tokens.push_back(current_token);
            line = line.substr(2);
            
            return current_token;
        }
        
        if(line[0] == '<' && line[1] == '=') {
            current_token = TokenType::LessOrEq{};
            tokens.push_back(current_token);
            line = line.substr(2);
            
            return current_token;
        }
        
        if(line[0] == '>' && line[1] == '=') {
            current_token = TokenType::GreaterOrEq{};
            tokens.push_back(current_token);
            line = line.substr(2);
            
            return current_token;
        }
    }
    
    char c = line[0];
    if(c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '>'
        || c == '<' || c == '(' || c == ')' || c == ',' || c == '.' || c == '?') {
        current_token = TokenType::Char{c};
        tokens.push_back(current_token);
        line = line.substr(1);
        
        return current_token;
    }
    
    unsigned char uc = static_cast<unsigned char>(c);
    if(std::isdigit(uc)){
        it = std::find_if(line.begin(), line.end(), [](unsigned char c){
            return !std::isdigit(c);
        });
        
        if(it == line.end()){
            throw LexerError("Invalid line. Not found the end of digital sequence.");
        }
        
        std::string sdigit = {line.begin(), it};
        line = {it, line.end()};
        int result{};
        auto [ptr, ec] { std::from_chars(sdigit.data(), sdigit.data() + sdigit.size(), result) };

        if (ec == std::errc()) {
            current_token = TokenType::Number{result};
            tokens.push_back(current_token);
            
            return current_token;
        } else {
            throw LexerError("Invalid number - " + sdigit);
        }
    }
    
    if(c == '_' || std::isalpha(uc)){
        it = std::find_if(line.begin(), line.end(), [](unsigned char c){
            return !std::isalnum(c) && c != '_';
        });
        
        if(it == line.end()){
            throw LexerError("Invalid line. Not found the end of Id sequence.");
        }
        
        std::string value = {line.begin(), it};
        line = {it, line.end()};
        
        bool lexema_found = true;
        if(value == "class"){
            current_token = TokenType::Class{};
        } else if(value == "return"){
            current_token = TokenType::Return{};
        } else if(value == "if"){
            current_token = TokenType::If{};
        } else if(value == "else"){
            current_token = TokenType::Else{};
        } else if(value == "def"){
            current_token = TokenType::Def{};
        } else if(value == "print"){
            current_token = TokenType::Print{};
        } else if(value == "and"){
            current_token = TokenType::And{};
        } else if(value == "or"){
            current_token = TokenType::Or{};
        } else if(value == "not"){
            current_token = TokenType::Not{};
        } else if(value == "None"){
            current_token = TokenType::None{};
        } else if(value == "True"){
            current_token = TokenType::True{};
        } else if(value == "False"){
            current_token = TokenType::False{};
        } else {
            lexema_found = false;
        }
        
        if(lexema_found){
            tokens.push_back(current_token);
            return current_token;
        }
        
        current_token = TokenType::Id{value};
        tokens.push_back(current_token);
        
        return current_token;
    }
    
    if(c == ':'){
        current_token = TokenType::Char{':'};
        tokens.push_back(current_token);
        line = '\n';

        return current_token;
    }
    
    if(c == '\n'){
        current_token = TokenType::Newline{};
        tokens.push_back(current_token);
        line = "";

        return current_token;
    }
    
    throw LexerError("Parse error. Unknown line - " + line);
}


    void TestSimpleAssignment() {
        istringstream input("x = 42\n");
        Lexer lexer(input);

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Id{"x"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{42}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
    }

    void TestKeywords() {
        istringstream input("class return if else def print or None and not True False");
        Lexer lexer(input);

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Class{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Return{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::If{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Else{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Def{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Print{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Or{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::None{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::And{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Not{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::True{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::False{}));
    }

    void TestNumbers() {
        istringstream input("42 15 -53");
        Lexer lexer(input);

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Number{42}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{15}));
        // Отрицательные числа формируются на этапе синтаксического анализа
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'-'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{53}));
    }

    void TestIds() {
        istringstream input("x    _42 big_number   Return Class  dEf");
        Lexer lexer(input);

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Id{"x"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"_42"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"big_number"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"Return"})); // keywords are case-sensitive
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"Class"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"dEf"}));
    }

    void TestPrint(){
        istringstream input("print 10, 24, -8");
        Lexer lexer(input);

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Print()));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{10}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{','}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{24}));
        // Отрицательные числа формируются на этапе синтаксического анализа
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{','}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'-'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{8}));
    }

    void TestStrings() {
        istringstream input(R"('word' "two words" 'long string with a double quote " inside' "another long string with single quote ' inside")");
        Lexer lexer(input);

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::String{"word"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::String{"two words"}));
        ASSERT_EQUAL(
                lexer.NextToken(), Token(TokenType::String{"long string with a double quote \" inside"})
        );
        ASSERT_EQUAL(
                lexer.NextToken(), Token(TokenType::String{"another long string with single quote ' inside"})
        );
    }

    void TestOperations() {
        istringstream input("+-*/= > < != == <> <= >=");
        Lexer lexer(input);

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Char{'+'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'-'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'*'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'/'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'>'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'<'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::NotEq{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eq{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'<'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'>'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::LessOrEq{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::GreaterOrEq{}));
    }

    void TestIndentsAndNewlines() {
        istringstream input(R"(
no_indent
  indent_one
    indent_two
      indent_three
      indent_three
      indent_three
    indent_two
  indent_one
    indent_two
no_indent
)");

        Lexer lexer(input);

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Id{"no_indent"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Indent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"indent_one"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Indent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"indent_two"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Indent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"indent_three"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"indent_three"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"indent_three"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Dedent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"indent_two"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Dedent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"indent_one"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Indent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"indent_two"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Dedent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Dedent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"no_indent"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
    }

    void TestEmptyLinesAreIgnored() {
        istringstream input(R"(
x = 1
  y = 2

  z = 3


)");
        Lexer lexer(input);

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Id{"x"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{1}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Indent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"y"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{2}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        // Пустая строка, состоящая только из пробельных символов не меняет текущий отступ,
        // поэтому следующая лексема — это Id, а не Dedent
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"z"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{3}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Dedent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
    }

    void TestMythonProgram() {
        istringstream input(R"(
x = 4
y = "hello"

class Point:
  def __init__(self, x, y):
    self.x = x
    self.y = y

  def __str__(self):
    return str(x) + ' ' + str(y)

p = Point(1, 2)
print str(p)
)");
        Lexer lexer(input);

        using namespace TokenType;

        ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Id{"x"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{4}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"y"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::String{"hello"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Class{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"Point"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{':'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Indent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Def{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"__init__"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'('}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"self"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{','}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"x"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{','}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"y"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{')'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{':'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Indent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"self"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'.'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"x"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"x"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"self"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'.'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"y"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"y"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Dedent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Def{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"__str__"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'('}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"self"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{')'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{':'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Indent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Return{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"str"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'('}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"x"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{')'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'+'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::String{" "}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'+'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"str"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'('}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"y"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{')'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Dedent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Dedent{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"p"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'='}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"Point"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'('}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{1}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{','}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Number{2}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{')'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Print{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"str"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{'('}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"p"}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Char{')'}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
        ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
    }

    void TestExpect() {
        istringstream is("bugaga");
        Lexer lex(is);

        ASSERT_DOESNT_THROW(lex.Expect<TokenType::Id>());
        ASSERT_EQUAL(lex.Expect<TokenType::Id>().value, "bugaga");
        ASSERT_DOESNT_THROW(lex.Expect<TokenType::Id>("bugaga"));
        ASSERT_THROWS(lex.Expect<TokenType::Id>("widget"), LexerError);
        ASSERT_THROWS(lex.Expect<TokenType::Return>(), LexerError);
    }

    void TestExpectNext() {
        istringstream is("+ bugaga + def 52");
        Lexer lex(is);

        ASSERT_EQUAL(lex.CurrentToken(), Token(TokenType::Char{'+'}));
        ASSERT_DOESNT_THROW(lex.ExpectNext<TokenType::Id>());
        ASSERT_DOESNT_THROW(lex.ExpectNext<TokenType::Char>('+'));
        ASSERT_THROWS(lex.ExpectNext<TokenType::Newline>(), LexerError);
        ASSERT_THROWS(lex.ExpectNext<TokenType::Number>(57), LexerError);
    }

    void TestAlwaysEmitsNewlineAtTheEndOfNonemptyLine() {
        {
            istringstream is("a b");
            Lexer lexer(is);

            ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Id{"a"}));
            ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Id{"b"}));
            ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
            ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
            ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
            ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
        }
        {
            istringstream is("+");
            Lexer lexer(is);

            ASSERT_EQUAL(lexer.CurrentToken(), Token(TokenType::Char{'+'}));
            ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Newline{}));
            ASSERT_EQUAL(lexer.NextToken(), Token(TokenType::Eof{}));
        }
    }

    void RunLexerTests(TestRunner& tr) {

        if(1 != 0){
            RUN_TEST(tr, Parse::TestSimpleAssignment);
            RUN_TEST(tr, Parse::TestKeywords);
            RUN_TEST(tr, Parse::TestNumbers);
            RUN_TEST(tr, Parse::TestIds);
            RUN_TEST(tr, Parse::TestStrings);
            RUN_TEST(tr, Parse::TestOperations);
            RUN_TEST(tr, Parse::TestIndentsAndNewlines);
            RUN_TEST(tr, Parse::TestEmptyLinesAreIgnored);
            RUN_TEST(tr, Parse::TestExpect);
            RUN_TEST(tr, Parse::TestExpectNext);
            RUN_TEST(tr, Parse::TestAlwaysEmitsNewlineAtTheEndOfNonemptyLine);
            RUN_TEST(tr, Parse::TestMythonProgram);
            RUN_TEST(tr, Parse::TestPrint);
        }
    }

} /* namespace Parse */
