#include "numstore.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    try {
        numstore::Database db("test.db");

        {
            auto tx = db.begin();
            db.execute(tx, "create foo u32");

            std::vector<uint32_t> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            db.write(tx, data, "insert foo 0 10");
            tx.commit();
        }

        {
            auto tx = db.begin();

            auto var = db.get_var(tx, "get foo");
            std::cout << "foo has " << var.length() << " elements\n";

            auto values = db.read_all<uint32_t>(tx, "read foo[0:10]");
            for (auto v : values) std::cout << v << ' ';
            std::cout << '\n';

            // Prepared plan; destroyed at end of scope, before db closes.
            auto plan = db.plan("read foo[0:5]");
            std::vector<uint32_t> first(5);
            plan.read(tx, first);

            tx.commit();
        } // an uncommitted transaction would roll back here

        db.close();

        // Smart file
        numstore::SmartFile smf("test.smf");
        auto tx = smf.begin();
        smf.append(tx, std::vector<uint32_t>{0, 1, 2, 3, 4, 5});
        auto odds = smf.read<uint32_t>(tx, 3, sizeof(uint32_t), 2); // 1 3 5
        for (auto v : odds) std::cout << v << ' ';
        std::cout << '\n';
        tx.commit();
    } catch (const numstore::Error &e) {
        std::cerr << "numstore error: " << e.what() << '\n';
        return 1;
    }
}
