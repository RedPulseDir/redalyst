#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>
#include <string_view>
#include <optional>

class remote_offsets
{
public:
    bool fetch( );
    
    std::uintptr_t get( std::string_view module, std::string_view name ) const;
    
    bool is_ready( ) const { return m_ready; }
    
private:
    bool parse_offsets( const std::string& content );
    std::uintptr_t resolve( std::string_view module, std::string_view name ) const;
    
    std::unordered_map<std::string, std::unordered_map<std::string, std::ptrdiff_t>> m_offsets;
    bool m_ready{ false };
};

namespace g {
    inline remote_offsets offsets_remote{};
}
