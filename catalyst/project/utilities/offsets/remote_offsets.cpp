#include <stdafx.hpp>
#include <winhttp.h>
#include <regex>
#include <sstream>
#include "remote_offsets.hpp"

#pragma comment(lib, "winhttp.lib")

namespace detail {
    
    static std::ptrdiff_t parse_hex( const std::string& str )
    {
        if ( str.empty( ) )
            return 0;
        
        std::string num = str;
        if ( num.size( ) > 2 && num[ 0 ] == '0' && ( num[ 1 ] == 'x' || num[ 1 ] == 'X' ) )
            num = num.substr( 2 );
        
        return static_cast< std::ptrdiff_t >( std::stoull( num, nullptr, 16 ) );
    }
    
} // namespace detail

bool remote_offsets::fetch_url( const std::string& url, std::string& output )
{
    // Parse URL
    URL_COMPONENTS url_comp{};
    url_comp.dwStructSize = sizeof( url_comp );
    
    wchar_t hostname[ 256 ]{};
    wchar_t url_path[ 1024 ]{};
    
    url_comp.lpszHostName = hostname;
    url_comp.dwHostNameLength = sizeof( hostname ) / sizeof( wchar_t );
    url_comp.lpszUrlPath = url_path;
    url_comp.dwUrlPathLength = sizeof( url_path ) / sizeof( wchar_t );
    
    std::wstring wurl( url.begin( ), url.end( ) );
    
    if ( !WinHttpCrackUrl( wurl.c_str( ), 0, 0, &url_comp ) )
        return false;
    
    bool is_https = ( url_comp.nScheme == INTERNET_SCHEME_HTTPS );
    
    HINTERNET session = WinHttpOpen( L"Catalyst/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0 );
    if ( !session )
        return false;
    
    DWORD flags = is_https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET connect = WinHttpConnect( session, hostname, url_comp.nPort, 0 );
    if ( !connect )
    {
        WinHttpCloseHandle( session );
        return false;
    }
    
    HINTERNET request = WinHttpOpenRequest( connect, L"GET", url_path, NULL, NULL, NULL, flags );
    if ( !request )
    {
        WinHttpCloseHandle( connect );
        WinHttpCloseHandle( session );
        return false;
    }
    
    // Send request
    if ( !WinHttpSendRequest( request, NULL, 0, NULL, 0, 0, 0 ) )
    {
        WinHttpCloseHandle( request );
        WinHttpCloseHandle( connect );
        WinHttpCloseHandle( session );
        return false;
    }
    
    if ( !WinHttpReceiveResponse( request, NULL ) )
    {
        WinHttpCloseHandle( request );
        WinHttpCloseHandle( connect );
        WinHttpCloseHandle( session );
        return false;
    }
    
    // Read response
    DWORD bytes_available = 0;
    std::string response;
    char buffer[ 4096 ];
    
    while ( WinHttpQueryDataAvailable( request, &bytes_available ) && bytes_available > 0 )
    {
        DWORD bytes_read = 0;
        DWORD to_read = ( bytes_available < sizeof( buffer ) ) ? bytes_available : sizeof( buffer );
        
        if ( WinHttpReadData( request, buffer, to_read, &bytes_read ) )
        {
            response.append( buffer, bytes_read );
        }
        else
        {
            break;
        }
    }
    
    WinHttpCloseHandle( request );
    WinHttpCloseHandle( connect );
    WinHttpCloseHandle( session );
    
    output = std::move( response );
    return !output.empty( );
}

bool remote_offsets::parse_offsets( const std::string& content )
{
    // Regex for module: namespace client_dll { ... }
    std::regex module_pattern( R"(namespace\s+(\w+_dll)\s*\{([^}]*)\})", std::regex::dotall );
    std::regex offset_pattern( R"(constexpr\s+std::ptrdiff_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+);)" );
    
    auto it = std::sregex_iterator( content.begin( ), content.end( ), module_pattern );
    auto end = std::sregex_iterator( );
    
    bool any = false;
    
    for ( ; it != end; ++it )
    {
        std::string module_name = ( *it )[ 1 ].str( );
        std::string module_body = ( *it )[ 2 ].str( );
        
        auto& offsets_map = m_offsets[ module_name ];
        
        auto off_it = std::sregex_iterator( module_body.begin( ), module_body.end( ), offset_pattern );
        auto off_end = std::sregex_iterator( );
        
        for ( ; off_it != off_end; ++off_it )
        {
            std::string name = ( *off_it )[ 1 ].str( );
            std::string value_str = ( *off_it )[ 2 ].str( );
            std::ptrdiff_t value = detail::parse_hex( value_str );
            
            offsets_map[ name ] = value;
            any = true;
        }
    }
    
    return any;
}

bool remote_offsets::fetch( )
{
    const std::string urls[ ] = {
        "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/offsets.hpp",
        "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/client_dll.hpp"
    };
    
    m_offsets.clear( );
    
    for ( const auto& url : urls )
    {
        std::string content;
        if ( !fetch_url( url, content ) )
        {
            g::console.error( "Failed to fetch offsets from: {}", url );
            return false;
        }
        
        if ( !parse_offsets( content ) )
        {
            g::console.warn( "No offsets parsed from: {}", url );
        }
    }
    
    if ( m_offsets.empty( ) )
    {
        g::console.error( "No offsets loaded!" );
        return false;
    }
    
    m_ready = true;
    g::console.success( "Remote offsets loaded ({} modules)", m_offsets.size( ) );
    
    return true;
}

std::uintptr_t remote_offsets::get( std::string_view module, std::string_view name ) const
{
    auto mod_it = m_offsets.find( std::string( module ) );
    if ( mod_it == m_offsets.end( ) )
        return 0;
    
    auto off_it = mod_it->second.find( std::string( name ) );
    if ( off_it == mod_it->second.end( ) )
        return 0;
    
    return static_cast< std::uintptr_t >( off_it->second );
}

std::uintptr_t remote_offsets::resolve( std::string_view module, std::string_view name, std::uintptr_t module_base ) const
{
    std::uintptr_t offset = get( module, name );
    if ( !offset )
        return 0;
    
    return module_base + offset;
}
