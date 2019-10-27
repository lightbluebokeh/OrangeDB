#include <orange/orange.h>

static std::unordered_set<String> names;
static String cur = "";

namespace Orange {
    bool exists(const String& name) { return names.count(name); }

    bool create(const String& name) {
        ensure(!exists(name), "database " + name + " already exists");
        names.insert(name);
        return fs::create_directory(name);
    }

    bool drop(const String& name) {
        ensure(exists(name), "database " + name + " does not exist");
        names.erase(name);
        if (name == cur) {
            fs::current_path("..");
            cur = "";
            return fs::remove(name);
        } else {
            return fs::remove("../" + name);
        }
    }

    bool use(const String& name) {
        if (name == cur) return 1;
        ensure(exists(name), "database " + name + " does not exist");
        if (cur.length()) fs::current_path("..");
        fs::current_path("name");
        return 1;
    }

    bool using_db() { return cur != ""; }

    std::vector<String> all() { return {names.begin(), names.end()}; }
    std::vector<String> all_tables() {
        if (!using_db()) return {};
        std::vector<String> ret;
        for (auto it: fs::directory_iterator(cur)) {
            if (it.is_directory()) ret.push_back(it.path().filename().string());
        }
        return ret;
    }

    String get_cur() { return cur; }
}