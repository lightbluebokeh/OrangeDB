#include "orange/orange.h"
#include "orange/table/table.h"
#include "exceptions.h"

#include <unordered_set>

static std::unordered_set<String> names;
static String cur;

namespace Orange {
    bool exists(const String& name) { return names.count(name); }

    bool create(const String& name) {
        if (using_db()) fs::current_path("..");
        orange_check(!exists(name), Exception::database_exists(name));
        names.insert(name);
        auto ret = fs::create_directory(name);
        if (using_db()) fs::current_path(cur);
        return ret;
    }

    bool drop(const String& name) {
        orange_check(exists(name), Exception::database_not_exist(name));
        names.erase(name);
        if (name == cur) {
            unuse();
            return fs::remove_all(name);
        } else if (using_db()) {
            return fs::remove_all("../" + name);
        } else {
            return fs::remove_all(name);
        }
    }

    bool use(const String& name) {
        if (name == cur) return 1;
        unuse();
        orange_check(exists(name), Exception::database_not_exist(name));
        fs::current_path(name);
        cur = name;
        return 1;
    }

    bool unuse() {
        if (using_db()) {
            SavedTable::close_all();
            cur = "";
            fs::current_path("..");
        }
        return 1;
    }

    bool using_db() { return !cur.empty(); }

    std::vector<String> all() { return { names.begin(), names.end() }; }

    std::vector<String> all_tables() {
        orange_check(using_db(), Exception::no_database_used());
        std::vector<String> data;
        for (auto it : fs::directory_iterator(".")) {
            if (it.is_directory()) data.push_back(it.path().filename().string());
        }
        return data;
    }

    String get_cur() { return cur; }

    void setup() {
        if (!fs::exists("db")) fs::create_directory("db");
        fs::current_path("db");
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_directory()) {
                names.insert(entry.path().filename().string());
            }
        }
    }

    void paolu() {
        unuse();
        fs::current_path("..");
        auto tmp = names;
        for (const auto& db_name : tmp) drop(db_name);
        fs::remove_all("db");
    }
}  // namespace Orange