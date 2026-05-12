#include <stdafx.hpp>
#include <curl/curl.h>
#include <regex>
#include "remote_offsets.hpp"

namespace detail {
    
    static size_t write_callback( void* contents, size_t size, size_t nmemb, std::string* output )
    {
        const auto total = size * nmemb;
        output->append( static_cast< char* >( contents ), total );
        return total;
    }
    
    static std::string fetch_url( const std::string& url )
    {
        CURL* curl = curl_easy_init( );
        if ( !curl )
            return {};
            
        std::string response;
        
        curl_easy_setopt( curl, CURLOPT_URL, url.c_str( ) );
        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_callback );
        curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response );
        curl_easy_setopt( curl, CURLOPT_TIMEOUT, 10L );
        curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
        
        const auto res = curl_easy_perform( curl );
        curl_easy_cleanup( curl );
        
        if ( res != CURLE_OK )
            return {};
            
        return response;
    }
    
    static std::ptrdiff_t parse_hex( const std::string& str )
    {
        if ( str.empty( ) )
            return 0;
            
        // Remove 0x prefix if present
        std::string num = str;
        if ( num.size( ) > 2 && num[ 0 ] == '0' && ( num[ 1 ] == 'x' || num[ 1 ] == 'X' ) )
            num = num.substr( 2 );
            
        return static_cast< std::ptrdiff_t >( std::stoull( num, nullptr, 16 ) );
    }
    
} // namespace detail

bool remote_offsets::fetch( )
{
    const std::string urls[ ] = {
        "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/offsets.hpp",
        "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/client_dll.hpp"
    };
    
    for ( const auto& url : urls )
    {
        const auto content = detail::fetch_url( url );
        if ( content.empty( ) )
        {
            g::console.error( "failed to fetch offsets from: {}", url );
            return false;
        }
        
        if ( !parse_offsets( content ) )
        {
            g::console.error( "failed to parse offsets from: {}", url );
            return false;
        }
    }
    
    m_ready = true;
    g::console.success( "remote offsets loaded ({} modules)", m_offsets.size( ) );
    
    // Debug output
    for ( const auto& [mod, offs] : m_offsets )
    {
        g::console.print( "  module: {}", mod );
        for ( const auto& [name, val] : offs )
        {
            g::console.print( "    {} = 0x{:X}", name, val );
        }
    }
    
    return true;
}

bool remote_offsets::parse_offsets( const std::string& content )
{
    // Regex patterns
    std::regex module_pattern( R"(namespace\s+(\w+_dll)\s*\{([^}]*)\})", std::regex::dotall );
    std::regex offset_pattern( R"(constexpr\s+std::ptrdiff_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+);)" );
    
    auto it = std::sregex_iterator( content.begin( ), content.end( ), module_pattern );
    auto end = std::sregex_iterator( );
    
    bool any = false;
    
    for ( ; it != end; ++it )
    {
        const std::string module_name = ( *it )[ 1 ].str( );
        const std::string module_body = ( *it )[ 2 ].str( );
        
        auto& offsets_map = m_offsets[ module_name ];
        
        auto off_it = std::sregex_iterator( module_body.begin( ), module_body.end( ), offset_pattern );
        auto off_end = std::sregex_iterator( );
        
        for ( ; off_it != off_end; ++off_it )
        {
            const std::string name = ( *off_it )[ 1 ].str( );
            const std::string value_str = ( *off_it )[ 2 ].str( );
            const auto value = detail::parse_hex( value_str );
            
            offsets_map[ name ] = value;
            any = true;
        }
    }
    
    return any;
}

std::uintptr_t remote_offsets::get( std::string_view module, std::string_view name ) const
{
    const auto mod_it = m_offsets.find( std::string( module ) );
    if ( mod_it == m_offsets.end( ) )
        return 0;
        
    const auto off_it = mod_it->second.find( std::string( name ) );
    if ( off_it == mod_it->second.end( ) )
        return 0;
        
    return static_cast< std::uintptr_t >( off_it->second );
}

std::uintptr_t remote_offsets::resolve( std::string_view module, std::string_view name ) const
{
    const auto base = g::modules.get_base( module );
    if ( !base )
        return 0;
        
    const auto offset = this->get( module, name );
    if ( !offset )
        return 0;
        
    return base + offset;
}
