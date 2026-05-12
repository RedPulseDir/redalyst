#include <stdafx.hpp>
#include "remote_offsets.hpp"

bool offsets::initialize( )
{
    // Загружаем оффсеты из GitHub через WinHTTP
    if ( !g::offsets_remote.fetch( ) )
    {
        g::console.error( "Failed to fetch remote offsets" );
        return false;
    }
    
    // Разрешаем адреса через базу модуля + смещение
    csgo_input = g::offsets_remote.resolve( "client_dll", "dwCSGOInput", g::modules.client );
    entity_list = g::offsets_remote.resolve( "client_dll", "dwEntityList", g::modules.client );
    local_player_controller = g::offsets_remote.resolve( "client_dll", "dwLocalPlayerController", g::modules.client );
    global_vars = g::offsets_remote.resolve( "client_dll", "dwGlobalVars", g::modules.client );
    view_matrix = g::offsets_remote.resolve( "client_dll", "dwViewMatrix", g::modules.client );
    
    // Проверяем что всё ок
    if ( !csgo_input || !entity_list || !local_player_controller || !global_vars || !view_matrix )
    {
        g::console.error( "Some offsets failed to resolve:" );
        if ( !csgo_input ) g::console.print( "  - csgo_input" );
        if ( !entity_list ) g::console.print( "  - entity_list" );
        if ( !local_player_controller ) g::console.print( "  - local_player_controller" );
        if ( !global_vars ) g::console.print( "  - global_vars" );
        if ( !view_matrix ) g::console.print( "  - view_matrix" );
        return false;
    }
    
    g::console.success( "Offsets initialized (remote via WinHTTP)" );
    return true;
}
