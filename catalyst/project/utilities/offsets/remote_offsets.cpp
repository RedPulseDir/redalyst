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
    
    static std::string fetch_url_winhttp( const std::string& url )
    {
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
            return {};
        
        bool is_https = ( url_comp.nScheme == INTERNET_SCHEME_HTTPS );
        
        HINTERNET session = WinHttpOpen( L"Catalyst/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0 );
        if ( !session )
            return {};
        
        DWORD flags = is_https ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET connect = WinHttpConnect( session, hostname, url_comp.nPort, 0 );
        if ( !connect )
        {
            WinHttpCloseHandle( session );
            return {};
        }
        
        HINTERNET request = WinHttpOpenRequest( connect, L"GET", url_path, NULL, NULL, NULL, flags );
        if ( !request )
        {
            WinHttpCloseHandle( connect );
            WinHttpCloseHandle( session );
            return {};
        }
        
        if ( !WinHttpSendRequest( request, NULL, 0, NULL, 0, 0, 0 ) )
        {
            WinHttpCloseHandle( request );
            WinHttpCloseHandle( connect );
            WinHttpCloseHandle( session );
            return {};
        }
        
        if ( !WinHttpReceiveResponse( request, NULL ) )
        {
            WinHttpCloseHandle( request );
            WinHttpCloseHandle( connect );
            WinHttpCloseHandle( session );
            return {};
        }
        
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
        
        return response;
    }
    
} // namespace detail

bool remote_offsets::fetch( )
{
    const std::string urls[ ] = {
        "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/offsets.hpp",
        "https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/client_dll.hpp"
    };
    
    m_offsets.clear( );
    
    for ( const auto& url : urls )
    {
        g::console.print( "Fetching: {}", url );
        std::string content = detail::fetch_url_winhttp( url );
        
        if ( content.empty( ) )
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

bool remote_offsets::parse_offsets( const std::string& content )
{
    // Pattern for module: namespace client_dll { ... }
    // Without dotall flag, using manual approach - find modules by scanning line by line
    
    std::regex module_start( R"(namespace\s+(\w+_dll)\s*\()" );
    std::regex offset_pattern( R"(constexpr\s+std::ptrdiff_t\s+(\w+)\s*=\s*(0x[0-9A-Fa-f]+);)" );
    
    std::istringstream stream( content );
    std::string line;
    std::string current_module;
    bool in_module = false;
    int brace_count = 0;
    
    while ( std::getline( stream, line ) )
    {
        // Check for module start
        std::smatch module_match;
        if ( std::regex_search( line, module_match, module_start ) && !in_module )
        {
            current_module = module_match[ 1 ].str( );
            in_module = true;
            brace_count = 0;
            continue;
        }
        
        if ( in_module )
        {
            // Count braces to know when module ends
            for ( char c : line )
            {
                if ( c == '{' ) brace_count++;
                if ( c == '}' ) brace_count--;
            }
            
            if ( brace_count < 0 )
            {
                in_module = false;
                current_module.clear( );
                continue;
            }
            
            // Parse offsets within this module
            std::smatch offset_match;
            if ( std::regex_search( line, offset_match, offset_pattern ) )
            {
                std::string name = offset_match[ 1 ].str( );
                std::string value_str = offset_match[ 2 ].str( );
                std::ptrdiff_t value = detail::parse_hex( value_str );
                
                m_offsets[ current_module ][ name ] = value;
            }
        }
    }
    
    return !m_offsets.empty( );
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
