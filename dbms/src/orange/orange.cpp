#include <orange/orange.h>
#include <orange/table/table.h>

static std::unordered_set<String> names;
static String cur = "";

namespace Orange {
    bool exists(const String& name) { return names.count(name); }

    bool create(const String& name) {
        if (using_db()) fs::current_path("..");
        orange_ensure(!exists(name), "database `" + name + "` already exists");
        names.insert(name);
        auto ret = fs::create_directory(name);
        if (using_db()) fs::current_path(cur);
        return ret;
    }

    bool drop(const String& name) {
        orange_ensure(exists(name), "database `" + name + "` does not exist");
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
        orange_ensure(exists(name), "database `" + name + "` does not exist");
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

    bool using_db() { return cur != ""; }

    TmpTable all() {
        return TmpTable::from_strings("databases", {names.begin(), names.end()});
    }

    TmpTable all_tables() {
        // if (!using_db()) return {};
        orange_ensure(using_db(), "using some database first");
        std::vector<String> data;
        for (auto it : fs::directory_iterator(".")) {
            if (it.is_directory()) data.push_back(it.path().filename().string());
        }
        return TmpTable::from_strings("tables", data);
    }

    String get_cur() { return cur; }

    void setup() {
        if (!fs::exists("db")) fs::create_directory("db");
        fs::current_path("db");
        for (auto entry : fs::directory_iterator("."))
            if (entry.is_directory()) {
                names.insert(entry.path().filename().string());
            }
    }

    void paolu() {
        unuse();
        fs::current_path("..");
        fs::remove_all("db");
        // std::cerr << e << std::endl;
    }
}  // namespace Orange