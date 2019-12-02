#include <defs.h>
#include <iostream>
#include <sstream>

#include <orange/orange.h>
#include <orange/parser/parser.h>
#include <orange/syntax/syntax.h>

enum class ErrorCode { Ok, ParseError };

std::string generate_error_message(const char* sql, const Orange::parser::parse_error& e) {
    std::stringstream ss;
    ss << RED << "FAILED" << RESET << ": " << e.what() << " (at " << e.first << ")\n";
    ss << "  " << sql << '\n';
    ss << "  " << std::string(e.first, ' ') << '^'
       << std::string(std::max(0, e.last - e.first - 1), ' ') << '\n';

    ss << CYAN << "expected" << RESET << ": <" << e.expected << ">\n";
    ss << CYAN << "got" << RESET << ": '\033[4m" << std::string(sql + e.first, sql + e.last)
       << "\033[0m'\n";
    return ss.str();
}

ErrorCode manage(const std::string& sql) {
    using namespace Orange::parser;
    static sql_parser parser;

    try {
        sql_ast ast = parser.parse(sql);
        std::cout << GREEN << "parsed " << ast.stmt_list.size() << " statement"
                  << (ast.stmt_list.size() <= 1 ? "" : "s") << RESET << std::endl;
        Orange::program(ast);
    }
    catch (const parse_error& e) {
        std::cout << generate_error_message(sql.c_str(), e);
        return ErrorCode::ParseError;
    }
    return ErrorCode::Ok;
}

int main(int argc, char* argv[]) {
    for (int i = 0; i < argc; i++) {
        // 消 warning
        printf("%s\n", argv[i]);
    }
    Orange::setup();
    std::cout << CYAN << "Welcome to OrangeDB terminal!" << RESET << std::endl;
    int retcode = 0;
    std::string sql;
    while (true) {
        std::cout << ">> ";
        std::getline(std::cin, sql);
        if (sql == "q" || sql == "Q") break;
        retcode = (int)manage(sql);
    }

    std::cout << "Goodbye." << std::endl;
    return retcode;
}
