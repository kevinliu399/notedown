#include <iostream>
#include <ctype.h>
#include <string>
#include <cassert>
#include <vector>
#include <utility>
#include <memory>
#include <sstream>
#include <vector>

// TODO: Add more token types as needed, > , ``, nested lists

/********************
*      Types        *
*********************/
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

/********************
*      Parser       *
*********************/

class Token {
private:
    TokenType type;
    std::string value;
    bool is_eof;
public:
    Token(TokenType type, std::string value) : type(type), value(value), is_eof(false) {}
    
    static Token createEOF() {
        Token token(TEXT, "");
        token.is_eof = true;
        return token;
    }

    TokenType getType() const {
        return type;
    }
    
    std::string getValue() const {
        return value;
    }

    bool isEOF() const {
        return is_eof;
    }
    
    // void repr() const {
    //     std::cout << "Token(" << type << ", \"" << value << "\")" << std::endl;
    // }
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
        int level = 1;
        advance();
        
        while (current_char == '#' && level < 6) {
            level++;
            advance();
        }

        if (!isspace(current_char)) {
            std::string rest = collect_until('\n');
            if (current_char == '\n') advance();
            return Token(TEXT, std::string(level, '#') + rest);
        }
        
        while (isspace(current_char) && current_char != '\n') {
            advance();
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
            default: return Token(TEXT, std::string(level, '#') + content);
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
            return Token::createEOF();
        }

        // Skip isolated newlines
        while (current_char == '\n') {
            advance();
            if (current_char == '\0') {
                return Token::createEOF();
            }
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

        std::string text_content;
        while (current_char != '\0' && !is_markdown_char(current_char)) {

            if (current_char == '\n' && peek() == '\n') {
                // If we see two newlines, stop collecting text
                break;
            }
            text_content += current_char;
            advance();
        }

        // Trim trailing newlines from text content
        while (!text_content.empty() && text_content.back() == '\n') {
            text_content.pop_back();
        }
        
        return Token(TEXT, text_content);
    }
};

class Parser {
private:
    std::unique_ptr<Lexer> lexer;
    std::vector<Token> tokens;
    std::string html;
    
public:
    Parser() = default;
    
    std::string parse(const std::string& markdown) {
        lexer = std::make_unique<Lexer>(markdown);
        tokens.clear();
        
        Token token = lexer->get_next_token();
        while (!token.isEOF()) {
            tokens.push_back(token);
            token = lexer->get_next_token();
        }
        
        return tokensToHtml();
    }
    
private:
    bool isInlineElement(TokenType type) {
        return type == BOLD || type == ITALIC || type == LINK || type == IMAGE;
    }
    
    bool isBlockElement(TokenType type) {
        return type == H1 || type == H2 || type == H3 || 
               type == H4 || type == H5 || type == H6 || 
               type == LIST;
    }
    
    std::string tokensToHtml() {
        std::stringstream output;
        bool inList = false;
        bool inParagraph = false;
        
        for (size_t i = 0; i < tokens.size(); i++) {
            const Token& token = tokens[i];
            TokenType type = token.getType();
            
            // Handle list wrapping
            if (type == LIST) {
                if (inParagraph) {
                    output << "</p>\n";
                    inParagraph = false;
                }
                if (!inList) {
                    output << "<ul>\n";
                    inList = true;
                }
            } else if (inList) {
                output << "</ul>\n";
                inList = false;
            }
            
            // Handle paragraph wrapping
            if ((type == TEXT || isInlineElement(type)) && !inParagraph) {
                output << "<p>";
                inParagraph = true;
            } else if (inParagraph && isBlockElement(type)) {
                output << "</p>\n";
                inParagraph = false;
            }
            
            // Convert token to HTML
            output << tokenToHtml(token);
            
            // Close paragraph if this is the last token or next token is a block element
            if (inParagraph && (i == tokens.size()-1 || isBlockElement(tokens[i+1].getType()))) {
                output << "</p>\n";
                inParagraph = false;
            }
        }
        
        if (inList) output << "</ul>\n";
        if (inParagraph) output << "</p>\n";
        
        return output.str();
    }
    
    std::string tokenToHtml(const Token& token) {
        std::string content = escapeHtml(token.getValue());
        
        switch (token.getType()) {
            case TEXT: return content;
            case H1: return "<h1>" + content + "</h1>\n";
            case H2: return "<h2>" + content + "</h2>\n";
            case H3: return "<h3>" + content + "</h3>\n";
            case H4: return "<h4>" + content + "</h4>\n";
            case H5: return "<h5>" + content + "</h5>\n";
            case H6: return "<h6>" + content + "</h6>\n";
            case BOLD: return "<strong>" + content + "</strong>";
            case ITALIC: return "<em>" + content + "</em>";
            case LIST: return "<li>" + content + "</li>\n";
            case LINK: {
                size_t sep = content.find('|');
                std::string text = content.substr(0, sep);
                std::string url = content.substr(sep + 1);
                return "<a href=\"" + url + "\">" + text + "</a>";
            }
            case IMAGE: {
                size_t sep = content.find('|');
                std::string alt = content.substr(0, sep);
                std::string src = content.substr(sep + 1);
                return "<img src=\"" + src + "\" alt=\"" + alt + "\">";
            }
            default: return content;
        }
    }
    
    std::string escapeHtml(const std::string& text) {
        std::stringstream output;
        for (char c : text) {
            switch (c) {
                case '<': output << "&lt;"; break;
                case '>': output << "&gt;"; break;
                case '&': output << "&amp;"; break;
                case '"': output << "&quot;"; break;
                default: output << c;
            }
        }
        return output.str();
    }
};


/********************
*    LEXER TESTS    *
*********************/

// Helper function to convert TokenType to string for debugging
std::string tokenTypeToString(TokenType type) {
    switch(type) {
        case TEXT: return "TEXT";
        case H1: return "H1";
        case H2: return "H2";
        case H3: return "H3";
        case H4: return "H4";
        case H5: return "H5";
        case H6: return "H6";
        case BOLD: return "BOLD";
        case ITALIC: return "ITALIC";
        case LINK: return "LINK";
        case IMAGE: return "IMAGE";
        case LIST: return "LIST";
        default: return "UNKNOWN";
    }
}

void runTests() {
    struct TestCase {
        std::string name;
        std::string input;
        std::vector<std::pair<TokenType, std::string>> expected;
    };

    std::vector<TestCase> tests = {
        {
            "Basic Header Test",
            "# Header 1\n",
            {{H1, "Header 1"}}
        },
        {
            "Multi-Level Header Test",
            "## Header 2\n### Header 3\n###### Header 6",
            {{H2, "Header 2"}, {H3, "Header 3"}, {H6, "Header 6"}}
        },
        {
            "Basic Text Test",
            "Plain text",
            {{TEXT, "Plain text"}}
        },
        {
            "Basic Bold Test",
            "**bold**",
            {{BOLD, "bold"}}
        },
        {
            "Basic Italic Test",
            "*italic*",
            {{ITALIC, "italic"}}
        },
        {
            "List Test",
            "- Item 1\n- Item 2",
            {{LIST, "Item 1"}, {LIST, "Item 2"}}
        },
        {
            "Link Test",
            "[OpenAI](https://openai.com)",
            {{LINK, "OpenAI|https://openai.com"}}
        },
        {
            "Image Test",
            "![Alt Text](image.png)",
            {{IMAGE, "Alt Text|image.png"}}
        },
    };

    for (const auto& test : tests) {
        std::cout << "\nRunning test: " << test.name << std::endl;
        
        Lexer lexer(test.input);
        std::vector<Token> tokens;
        Token token = lexer.get_next_token();
        
        while (!token.isEOF()) {
            tokens.push_back(token);
            token = lexer.get_next_token();
        }

        bool passed = tokens.size() == test.expected.size();
        for (size_t i = 0; i < tokens.size() && passed; ++i) {
            if (tokens[i].getType() != test.expected[i].first || tokens[i].getValue() != test.expected[i].second) {
                passed = false;
                std::cout << "Mismatch in token " << i 
                          << ": Expected (" << tokenTypeToString(test.expected[i].first) << ", \"" << test.expected[i].second << "\") "
                          << "but got (" << tokenTypeToString(tokens[i].getType()) << ", \"" << tokens[i].getValue() << "\")\n";
            }
        }

        if (passed) {
            std::cout << "Test passed!" << std::endl;
        } else {
            std::cout << "Test failed!" << std::endl;
            std::cout << "Expected tokens:\n";
            for (const auto& expected : test.expected) {
                std::cout << "  (" << tokenTypeToString(expected.first) << ", \"" << expected.second << "\")\n";
            }
            std::cout << "Actual tokens:\n";
            for (const auto& actual : tokens) {
                std::cout << "  (" << tokenTypeToString(actual.getType()) << ", \"" << actual.getValue() << "\")\n";
            }
        }
    }

    std::cout << "\nAll tests completed successfully!" << std::endl;
}

void runParserTests() {
    struct TestCase {
        std::string name;
        std::string input;
        std::string expected_html;
    };

    std::vector<TestCase> tests = {
        {
            "Basic Header Test",
            "# Header 1",
            "<h1>Header 1</h1>\n"
        },
        {
            "Multiple Headers Test",
            "## Header 2\n### Header 3",
            "<h2>Header 2</h2>\n<h3>Header 3</h3>\n"
        },
        {
            "Text and Bold Test",
            "This is **bold** text.",
            "<p>This is <strong>bold</strong> text.</p>\n"
        },
        {
            "Italic Text Test",
            "This is *italic* text.",
            "<p>This is <em>italic</em> text.</p>\n"
        },
        {
            "Link Test",
            "This is a [link](http://example.com).",
            "<p>This is a <a href=\"http://example.com\">link</a>.</p>\n"
        },
        {
            "Image Test",
            "This is an image ![Alt text](image.png).",
            "<p>This is an image <img src=\"image.png\" alt=\"Alt text\">.</p>\n"
        },
        {
            "List Test",
            "- Item 1\n- Item 2",
            "<ul>\n<li>Item 1</li>\n<li>Item 2</li>\n</ul>\n"
        },
        {
            "Complex Mixed Content",
            "# Title\nSome **bold** and *italic* text with a [link](http://example.com).\n- List item 1\n- List item 2",
            "<h1>Title</h1>\n<p>Some <strong>bold</strong> and <em>italic</em> text with a <a href=\"http://example.com\">link</a>.</p>\n<ul>\n<li>List item 1</li>\n<li>List item 2</li>\n</ul>\n"
        },
    };

    Parser parser;

    for (const auto& test : tests) {
        std::cout << "\nRunning test: " << test.name << std::endl;
        
        std::string actual_html = parser.parse(test.input);
        
        if (actual_html == test.expected_html) {
            std::cout << "Test passed!" << std::endl;
        } else {
            std::cout << "Test failed!" << std::endl;
            std::cout << "Expected:\n" << test.expected_html << std::endl;
            std::cout << "Got:\n" << actual_html << std::endl;
        }
    }

    std::cout << "\nAll parser tests completed!" << std::endl;
}

int main() {
    // runTests();
    // runParserTests();
    return 0;
}