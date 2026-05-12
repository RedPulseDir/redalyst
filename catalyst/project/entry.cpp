#include <stdafx.hpp>
#include <curl/curl.h>

int main( )
{
    timeBeginPeriod( 1 );
    
    // Инициализируем curl глобально (один раз)
    curl_global_init( CURL_GLOBAL_ALL );
    
    {
        if ( !g::console.initialize( " :> (id recommend you cap your ingame fps for better performance)" ) )
            return 1;
            
        if ( !g::input.initialize( ) )
            return 1;
            
        if ( !g::memory.initialize( L"cs2.exe" ) )
            return 1;
    }
    
    {
        if ( !g::modules.initialize( ) )
            return 1;
            
        // Offsets теперь грузятся из интернета
        if ( !g::offsets.initialize( ) )
            return 1;
    }
    
    {
        std::thread( threads::game ).detach( );
        std::thread( threads::combat ).detach( );
        
        if ( !g::render.initialize( ) )
            return 1;
    }
    
    curl_global_cleanup( );
    return 0;
}
