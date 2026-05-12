// utilities/modules/modules.hpp
#pragma once

class modules
{
public:
    bool initialize( );
    [[nodiscard]] std::uintptr_t get_base( std::string_view name ) const;

    std::uintptr_t client{ 0 };
    std::uintptr_t engine2{ 0 };
    std::uintptr_t tier0{ 0 };
    std::uintptr_t schemasystem{ 0 };
    std::uintptr_t vphysics2{ 0 };
    std::uintptr_t inputsystem{ 0 };
    std::uintptr_t matchmaking{ 0 };
    std::uintptr_t soundsystem{ 0 };
};
