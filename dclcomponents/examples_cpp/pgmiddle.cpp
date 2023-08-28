#include <glog/logging.h>

#include <iostream>
#include <string>
#include <pqxx/pqxx>

#include <processor/dclprocessor.h>

int generate_parameter(int real_size, int max_diff) {
    srand(time(0));
    int x = rand() % 2 * max_diff - max_diff;
    return real_size - x;
}

int generate_obj_number(int min_n, int max_n) {
    srand(time(0));
    int x = rand() % (max_n - min_n);
    return min_n + x;
}

pqxx::result insert_object(char *obj_name) {
    std::string connection_string("host=79.173.96.138 port=5432 dbname=test_db user=tmk password=tmkdb");
    pqxx::connection conn(connection_string.c_str());
    pqxx::work transaction(conn);

    pqxx::result r {
        transaction.exec("INSERT INTO objects VALUES (DEFAULT, (SELECT object_types.id FROM object_types WHERE object_types.name = 'mandrel'), '----'," + transaction.quote(obj_name) + ");")
    };

    transaction.commit();
    return r;
}

pqxx::result select_objects() {
    std::string connection_string("host=79.173.96.138 port=5432 dbname=test_db user=tmk password=tmkdb");
    pqxx::connection conn(connection_string.c_str());
    pqxx::work transaction(conn);

    pqxx::result r {
        transaction.exec("SELECT * FROM objects LIMIT 10;")
    };
    if (r.size() < 1) {
        std::cout << "No data" << std::endl;
    }

    std::cout << "id\tobj_number\t\tobj_generated_id" << std::endl;
    for (auto row: r) {
        std::cout << row[0].c_str() << "\t" << row[2].c_str() << "\t\t" << row[3].c_str() << std::endl;
    }
    fflush(stdout);

    transaction.commit();
    return r;
}

pqxx::result select_count_objects() {
    std::string connection_string("host=79.173.96.138 port=5432 dbname=test_db user=tmk password=tmkdb");
    pqxx::connection conn(connection_string.c_str());
    pqxx::work transaction(conn);

    pqxx::result r {
        transaction.exec("SELECT count(*) FROM objects;")
    };
    if (r.size() < 1) {
        std::cout << "No data" << std::endl;
    }

    for (auto row: r) {
        std::cout << "Objects in database: " << row[0] << std::endl;
    }
    fflush(stdout);

    transaction.commit();
    return r;
}

pqxx::result insert_mandrel_ok(char *obj_name, char * obj_number, char *diameter_str) {
    std::string connection_string("host=79.173.96.138 port=5432 dbname=test_db user=tmk password=tmkdb");
    pqxx::connection conn(connection_string.c_str());
    pqxx::work transaction(conn);

    pqxx::result r {
        transaction.exec("CALL insert_mandrel_ok("
                         + transaction.quote(obj_name) + "," 
                          + transaction.quote(obj_number) + ", " 
                          + transaction.quote(diameter_str) + ");"
        )
    };

    transaction.commit();
    return r;
}

pqxx::result insert_billet(char *obj_name, char *obj_number, 
                           char *length_str, 
                           char *diameter1_str, char *diameter2_str, char *diameter3_str) {
    std::string connection_string("host=79.173.96.138 port=5432 dbname=test_db user=tmk password=tmkdb");
    pqxx::connection conn(connection_string.c_str());
    pqxx::work transaction(conn);

    pqxx::result r {
        transaction.exec("CALL insert_billet(" 
                              + transaction.quote(obj_name) + ", " 
                              + transaction.quote(obj_number) + ", " 
                              + transaction.quote(length_str) + ", " 
                              + transaction.quote(diameter1_str) + ", " 
                              + transaction.quote(diameter2_str) + ", " 
                              + transaction.quote(diameter3_str) + ");"
        )
    };

    transaction.commit();
    return r;
}

#include <stdlib.h> //strdup()

void work_with_object(void *user_data_ptr, char *boost_path, char *obj_name) {
    if (!user_data_ptr) {
        std::cout  << "[pg]:Hello, object " << obj_name << " from " << boost_path << " and no user data" << std::endl;
    }
    else {
        std::cout  << "[pg]:Hello, object " << obj_name << " from " << boost_path << " and some user data " << std::endl;
    }
    try {
#if 0
        pqxx::result ri{insert_object(obj_name)};
#else
        int usual_diameter = 600;
        int max_diff = usual_diameter * 0.05;
        char *obj_number = strdup("'0000'");
        char *diameter_str = strdup(std::to_string(generate_parameter(usual_diameter, max_diff)).c_str());
        pqxx::result ri{insert_mandrel_ok(obj_name, obj_number, diameter_str)};
        free(obj_number);
        free(diameter_str);
#endif
        pqxx::result rs{select_count_objects()};
#if 0
        for (auto row: rs) {
            std::cout << "[pg]: Row: ";
            for (auto field: row) std::cout << field.c_str() << " ";
            std::cout << std::endl;
            fflush(stdout);
        }
#endif
    }
    catch (pqxx::sql_error const &e) {
        std::cerr << "[pg]: SQL error: " << e.what() << std::endl;
        std::cerr << "[pg]: Query was: " << e.query() << std::endl;
        fflush(stderr);
        return;
    }
    catch (std::exception const &e) {
        std::cerr << "[pg]: Error: " << e.what() << std::endl;
        fflush(stderr);
        return;
    }
}

//user_data_ptr is something from main to work_with_object callback
int main(){
    char *user_data_ptr = strdup("user data");
    struct dclprocessor *proc = dclprocessor_get(2);
    attach_to_pipeline(proc, user_data_ptr, work_with_object);
    free(user_data_ptr);
    return 0;
}

