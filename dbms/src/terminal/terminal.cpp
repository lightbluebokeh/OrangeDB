#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>

#include "defs.h"
#include "orange/orange.h"
#include "orange/parser/parser.h"
#include "orange/syntax/syntax.h"

enum class ErrorCode { Ok, ParseError };

static std::string generate_error_message(const char* sql, const ast::parse_error& e) {
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

auto& cout = std::cout;
using std::endl;

static ErrorCode manage(const std::string& sql) {
    using namespace ast;
    static sql_parser parser;

    try {
        sql_ast ast = parser.parse(sql);
        auto results = Orange::program(ast);
        for (auto& result : results) {
            if (!result.ok())
                cout << RED << "FAILED: " << RESET << result.what() << endl;
            else if (result.has())
                cout << result.get() << endl;
        }
    }
    catch (const parse_error& e) {
        std::cout << generate_error_message(sql.c_str(), e);
        return ErrorCode::ParseError;
    }
    return ErrorCode::Ok;
}


static auto from_file(const String& name) {
    assert(fs::exists(name) && !fs::is_directory(name));
    std::ifstream ifs(name);
    ifs.seekg(0, std::ios::end);
    auto size = ifs.tellg();
    String program(size, ' ');
    ifs.seekg(0).read(program.data(), size);
    return manage(program);
}

// 启动 terminal 时的路径
static String cwd;

int main(int argc, char* argv[]) {
    cwd = fs::current_path().string() + "/";
    for (int i = 0; i < argc; i++) {
        // 消 warning
        printf("%s\n", argv[i]);
    }
    Orange::setup();
    std::cout << CYAN << "Welcome to OrangeDB terminal!" << RESET << std::endl;

    int ret_code = 0;
    std::string sql;
    while (true) {
        std::cout << ">> ";
        std::getline(std::cin, sql);
        if (sql == "q" || sql == "Q") break;
        if (sql == "f" || sql == "F") {
            printf("file name: ");
            String name;
            std::getline(std::cin, name);
            if (name.empty()) continue;
            if (name.front() != '/') name = cwd + name;
            if (!fs::exists(name)) {
                printf("\nno such file or directory\n");
                continue;
            } else if (fs::is_directory(name)) {
                printf("\ndirectory is not allowed\n");
                continue;
            }
            ret_code = (int)from_file(name);
            continue;
        }
        if (sql == "load") {
            printf("table name: ");
            String table;
            std::getline(std::cin, table);
            if (table.empty()) continue;

            printf("path: ");
            String path;
            std::getline(std::cin, path);
            if (path.empty()) continue;

            std::ifstream fin(path);
            if (!fin) {
                std::cout << "failed to open '" << path << "'\n";
                continue;
            }
            String line;
            const std::regex is_number("((\\+|-)?[[:digit:]]+)(\\.(([[:digit:]]+)?))?");
            while (std::getline(fin, line)) {
                std::stringstream ss(line);
                String token, values;
                while (std::getline(ss, token, '|')) {
                    std::cout << token << '\n';
                    if (std::regex_match(token, is_number)) {
                        values += token + ',';
                    } else {
                        values += '\'' + token + '\'' + ',';
                    }
                }
                values.pop_back();
                ret_code = (int)manage("INSERT INTO " + table + " VALUES (" + values + ");");
                if (ret_code != 0) {
                    printf("insertion failed\n");
                    break;
                }
            }
        }

        ret_code = (int)manage(sql);
    }

    Orange::unuse();  // 可能需要捕获ctrl+c信号

    std::cout << "Goodbye." << std::endl;
    return ret_code;
}
