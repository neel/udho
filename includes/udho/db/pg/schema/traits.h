#ifndef UDHO_DB_PG_SCHEMA_TRAITS_H
#define UDHO_DB_PG_SCHEMA_TRAITS_H

namespace udho{
namespace db{
namespace pg{

namespace traits{

    namespace labels{
        struct not_null{};
        struct default_value{};
    }


    template <typename FieldT, typename ValueT>
    struct not_null: labels::not_null{};

    template <typename Constant>
    struct default_{
        template <typename FieldT, typename ValueT>
        struct value: labels::default_value{
            using default_value = Constant;
        };
    };

    namespace has{
        template<typename FieldT>
        using not_null = std::is_base_of<pg::traits::labels::not_null, FieldT>;
        template<typename FieldT>
        using default_value = std::is_base_of<pg::traits::labels::default_value, FieldT>;
    }
}


}
}
}

#endif // UDHO_DB_PG_SCHEMA_TRAITS_H
