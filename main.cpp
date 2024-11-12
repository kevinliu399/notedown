#include <iostream>
#include <ctype.h>
#include <string>

// TODO: Add more token types as needed, > , ``, nested lists
typedef enum Type {
    TEXT,           // Plain text
    H1,             // #
    H2,             // ##
    H3,             // ###
    H4,             // ####
    H5,             // #####
    H6,             // ######
    BOLD,           // **bold**
    ITALIC,         // *italic*
    LINK,           // [text](url)
    IMAGE,          // ![alt](url)
    LIST,           // - item
} TokenType;

class Token {
private:
    TokenType type;
    std::string value;
public:
    Token(TokenType type, std::string value) : type(type), value(value) {}
    
    TokenType getType() const {
        return type;
    }
    
    std::string getValue() const {
        return value;
    }
    
    void repr() const {
        std::cout << "Token(" << type << ", \"" << value << "\")" << std::endl;
    }
};

class Lexer {
private:
    std::string text;
    size_t pos;
    char current_char;

    void advance() {
        pos++;
        if (pos >= text.length()) {
            current_char = '\0';
        } else {
            current_char = text[pos];
        }
    }

    char peek() {
        size_t peek_pos = pos + 1;
        if (peek_pos >= text.length()) {
            return '\0';
        }
        return text[peek_pos];
    }

    std::string collect_until(char delimiter, bool include_delimiter = false) {
        std::string result;
        while (current_char != '\0' && current_char != delimiter && current_char != '\n') {
            result += current_char;
            advance();
        }
        if (include_delimiter && current_char == delimiter) {
            result += current_char;
            advance();
        }
        return result;
    }

    Token handle_heading() {
        std::string original = "#";
        int level = 1;
        advance();
        
        // Skip leading whitespace before counting #
        while (isspace(current_char) && current_char != '\n') {
            original += current_char;
            advance();
        }
        
        while (current_char == '#' && level < 6) {
            original += "#";
            level++;
            advance();
        }
        
        // Count spaces after #
        int spaces = 0;
        while (isspace(current_char) && current_char != '\n') {
            spaces++;
            advance();
        }
        
        if (spaces == 0) {
            std::string rest = collect_until('\n');
            if (current_char == '\n') advance();
            return Token(TEXT, original + rest);
        }
        
        std::string content = collect_until('\n');
        if (current_char == '\n') advance();
        
        switch (level) {
            case 1: return Token(H1, content);
            case 2: return Token(H2, content);
            case 3: return Token(H3, content);
            case 4: return Token(H4, content);
            case 5: return Token(H5, content);
            case 6: return Token(H6, content);
            default: 
                return Token(TEXT, original + content);
        }
    }

    Token handle_emphasis() {
        std::string original = "*";
        advance();
        
        if (current_char == '*') {
            // Bold case
            advance();
            std::string content = collect_until('*');
            if (current_char == '*' && peek() == '*') {
                advance();
                advance();
                return Token(BOLD, content);
            }
            return Token(TEXT, "**" + content + (current_char == '*' ? "*" : ""));
        } else {
            // Italic case
            std::string content;
            while (current_char != '\0' && current_char != '\n') {
                if (current_char == '*') {
                    advance();
                    return Token(ITALIC, content);
                }
                content += current_char;
                advance();
            }
            return Token(TEXT, "*" + content);
        }
    }

    Token handle_link() {
        advance();
        std::string text;
        int bracket_count = 1;
        
        while (current_char != '\0') {
            if (current_char == '\\' && peek() == '[') {
                text += '[';
                advance();
                advance();
                continue;
            }
            
            if (current_char == '[') {
                bracket_count++;
            } else if (current_char == ']') {
                bracket_count--;
                if (bracket_count == 0) {
                    advance();
                    break;
                }
            }
            text += current_char;
            advance();
        }
        
        if (bracket_count > 0) {
            return Token(TEXT, "[" + text);
        }
        
        if (current_char != '(') {
            return Token(TEXT, "[" + text + "]");
        }
        
        advance();
        std::string url = collect_until(')');
        if (current_char != ')') {
            return Token(TEXT, "[" + text + "](" + url);
        }
        
        advance();
        return Token(LINK, text + "|" + url);
    }

    Token handle_image() {
        advance();
        if (current_char != '[') {
            return Token(TEXT, "!");
        }
        Token linkToken = handle_link();
        if (linkToken.getType() == LINK) {
            return Token(IMAGE, linkToken.getValue());
        }
        return Token(TEXT, "!" + linkToken.getValue());
    }

    Token handle_list() {
        advance();
        
        if (!isspace(current_char)) {
            std::string rest = collect_until('\n');
            if (current_char == '\n') {
                advance();
            }
            return Token(TEXT, "-" + rest);
        }
        
        advance(); 
        std::string content = collect_until('\n');
        if (current_char == '\n') advance();
        return Token(LIST, content);
    }

    // TODO: add more characters as needed
    bool is_markdown_char(char c) {
        return c == '#' || c == '*' || c == '[' || c == '!' || c == '-';
    }

public:
    Lexer(const std::string& text) : text(text), pos(0) {
        current_char = text.empty() ? '\0' : text[0];
    }
    
    Token get_next_token() {
        if (current_char == '\0') {
            return Token(TEXT, "");
        }

        if (current_char == '#') {
            return handle_heading();
        }
        
        if (current_char == '*') {
            return handle_emphasis();
        }
        
        if (current_char == '[') {
            return handle_link();
        }
        
        if (current_char == '!') {
            return handle_image();
        }
        
        if (current_char == '-') {
            return handle_list();
        }

        // Handle regular text, including whitespace
        std::string text_content;
        while (current_char != '\0' && !is_markdown_char(current_char)) {
            if (current_char == '\n') {
                advance();
            } else {
                text_content += current_char;
                advance();
            }
        }
        
        // If the text is empty or only contains whitespace and newlines, skip it
        if (text_content.empty() || (text_content.find_first_not_of(" \t\n\r") == std::string::npos)) {
            if (current_char != '\0') {
                return get_next_token();
            }
            return Token(TEXT, "");
        }
        
        return Token(TEXT, text_content);
    }
};

class Interpreter {
private:
    Lexer lexer;
    Token current_token;
public:
    Interpreter(Lexer l) : lexer(l), current_token(l.get_next_token()) {}

    std::string parse() {
        std::string res;
        TokenType type = current_token.getType();
        std::string val = current_token.getValue();

        switch (type) {
            case TEXT:
                res = val;
                break;
            case H1:
                res = "<h1>" + val + "</h1>";
                break;
            case H2:
                res = "<h2>" + val + "</h2>";
                break;
            case H3:
                res = "<h3>" + val + "</h3>";
                break;
            case H4:
                res = "<h4>" + val + "</h4>";
                break;
            case H5:
                res = "<h5>" + val + "</h5>";
                break;
            case H6:
                res = "<h6>" + val + "</h6>";
                break;
            case BOLD:
                res = "<b>" + val + "</b>";
                break;
            case ITALIC:
                res = "<i>" + val + "</i>";
                break;
            case LINK:
            {
                std::string link = val.substr(0, val.find("|"));
                std::string url = val.substr(val.find("|") + 1, val.length());
                res = "<a href=\"" + url + "\">" + link + "</a>";
                break;
            }
            case IMAGE:
            {
                std::string alt = val.substr(0, val.find("|"));
                std::string src = val.substr(val.find("|") + 1, val.length());
                res = "<img src=\"" + src + "\" alt=\"" + alt + "\">";
                break;
            }
            case LIST:
                // TODO: Handle nested lists
                res = "<li>" + val + "</li>";
                break;
            default:
                res = "";
        }

        return res;
    }
};

int main() {
    std::string markdown =
        "# Valid heading 1\n"
        "#Invalid heading\n"
        "## Valid heading 2\n"
        "This is **bold** and *italic* text.\n"
        "This is **unclosed bold and *nested italic*\n"
        "- Valid list item\n"
        "-Invalid list item\n"
        "Here's a [valid link](https://example.com)\n"
        "Here's an [invalid link](https://example.com\n"
        "And a ![valid image](image.jpg)\n"
        "And an ![invalid image(image.jpg\n"
        "A *Nested bolded **nested italic** bolded again*\n";
    
    Lexer lexer(markdown);
    Token token = lexer.get_next_token();
    
    while (token.getType() != TEXT || !token.getValue().empty()) {
        token.repr();
        token = lexer.get_next_token();
    }
    
    return 0;
}