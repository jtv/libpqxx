#include "pqxx-source.hxx"
#include "pqxx/pqxx_json.hpp"
#include "pqxx/row.hxx"
#include "pqxx/types.hxx"
#include "pqxx/result.hxx"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void pqxx::to_json(json& result_json, pqxx::result &res) {
    // number of rows
    pqxx::result::size_type num_rows = std::size(res);
    // number of columns
    pqxx::result::size_type num_columns = res.columns();

    for (pqxx::result::size_type j = 0; j < num_columns; ++j) {
        auto jth_column_name = res.column_name(j);
        for (pqxx::result::size_type i = 0; i < num_rows; ++i) {
            // in the ith column, map the json's data property's ith property's jth property, 
            // which is the name of the jth Postgres column of the result set, to the value of the jth 
            // column in the row
            result_json["data"][i][jth_column_name] = res[i][j];
        }
    }
    result_json["status-code"] = 200;
}

void pqxx::to_json(json& result_json, pqxx::result&& res) {
    to_json(result_json, res);
}

void pqxx::to_json(json& result_json, pqxx::row& res_row) {
    pqxx::row::size_type num_columns = res_row.size();
    for (pqxx::row::size_type i = 0; i < num_columns; ++i) {
        result_json["data"][i] = res_row[i];
    }
    result_json["status-code"] = 200;
}
