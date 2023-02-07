#ifndef UDHO_DB_PG_SCHEMA_CONSTRAINTS_H
#define UDHO_DB_PG_SCHEMA_CONSTRAINTS_H

#include <type_traits>

namespace udho{
namespace db{
namespace pg{

namespace constraints{

    namespace labels{
        struct not_null{};
        struct default_value{};
        struct unique{};
    }


    template <typename FieldT, typename ValueT>
    struct not_null: labels::not_null{};

    template <typename FieldT, typename ValueT>
    struct unique: labels::unique{};

    template <typename Constant>
    struct default_{
        template <typename FieldT, typename ValueT>
        struct value: labels::default_value{
            using default_value = Constant;
        };
    };

    namespace has{
        template<typename FieldT>
        using not_null = std::is_base_of<pg::constraints::labels::not_null, FieldT>;
        template<typename FieldT>
        using unique = std::is_base_of<pg::constraints::labels::unique, FieldT>;
        template<typename FieldT>
        using default_value = std::is_base_of<pg::constraints::labels::default_value, FieldT>;
    }
}


}
}
}

#endif // UDHO_DB_PG_SCHEMA_CONSTRAINTS_H
