// implements to_json for varius pqxx return types
// by implementing to_json for a return type, we can initialize a json using a value or variable,
// of type having said return type, by passing said value or variable to the json constructor.
// No extra syntax is needed!
// For example: suppose we have a result res that is already defined and a json my_json.
// Then, since to_json is defined for result objects below,
// we can get the result set from res in my_json by doing my_json = res;

#ifndef PQXX_H_JSON
#define PQXX_H_JSON
#include "pqxx/internal/result_iter.hxx"
#include "pqxx/result.hxx"
#include "pqxx/transaction_base.hxx"
#include <tuple>

// include json.hpp to use the nlohmann:json data type as json type
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace pqxx::internal {
    template<typename... TYPE> class result_iteration;
}

int cur_row_index = -1;

template <typename... types>
void tuple_callback(const types&... col_args) {
    int column_index = 0;
    auto tr_base_json_callback = [&column_index](const auto& elem) {
            tr_base_json[cur_row_index][column_index++] = elem;
    };
    (tr_base_json_callback(col_args), ...);
}

namespace pqxx { 
    void to_json(json& result_json, const result& res);
    void to_json(json& result_json, const result&& res);

    void to_json(json& result_json, const row& res_row);
    void to_json(json& result_json, const row&& res_row);

    template <typename ...types>
    // to_json for the return value of a pqxx::work::query(); the return value is an iterator to
    // the underlying result set, which is composed of a number of std::tuples<types...>
    void to_json(json& tr_base_json, result_iteration<types...>& iter_result) {
        for (auto it = iter_result.begin(); it != iter_result.end(); ++it) {
            cur_row_index++;
            std::apply(tuple_callback, *it);
        }
        tr_base_json["status-code"] = 200;
    }

    template <typename ...types>
    void to_json(json& tr_base_json, result_iteration<types...>&& iter_result) {
        to_json(tr_base_json, iter_result);
    }
}
#endif
