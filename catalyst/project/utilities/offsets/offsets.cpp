#include <stdafx.hpp>
#include "remote_offsets.hpp"

bool offsets::initialize( )
{
    // Загружаем оффсеты из интернета
    if ( !g::offsets_remote.fetch( ) )
    {
        g::console.error( "failed to fetch remote offsets" );
        return false;
    }
    
    // Разрешаем адреса через базу модуля + смещение
    csgo_input = g::offsets_remote.get( "client_dll", "dwCSGOInput" );
    csgo_input = csgo_input ? ( g::modules.client + csgo_input ) : 0;
    
    entity_list = g::offsets_remote.get( "client_dll", "dwEntityList" );
    entity_list = entity_list ? ( g::modules.client + entity_list ) : 0;
    
    local_player_controller = g::offsets_remote.get( "client_dll", "dwLocalPlayerController" );
    local_player_controller = local_player_controller ? ( g::modules.client + local_player_controller ) : 0;
    
    global_vars = g::offsets_remote.get( "client_dll", "dwGlobalVars" );
    global_vars = global_vars ? ( g::modules.client + global_vars ) : 0;
    
    view_matrix = g::offsets_remote.get( "client_dll", "dwViewMatrix" );
    view_matrix = view_matrix ? ( g::modules.client + view_matrix ) : 0;
    
    // Проверяем что всё ок
    if ( !csgo_input || !entity_list || !local_player_controller || !global_vars || !view_matrix )
    {
        g::console.error( "some offsets failed to resolve" );
        return false;
    }
    
    g::console.success( "offsets initialized (remote)" );
    return true;
}
