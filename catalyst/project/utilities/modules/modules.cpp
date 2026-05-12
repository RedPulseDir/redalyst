// utilities/modules/modules.cpp
#include <stdafx.hpp>

bool modules::initialize( )
{
    this->client = g::memory.get_module( "client.dll" );
    if ( !this->client )
    {
        return false;
    }

    this->engine2 = g::memory.get_module( "engine2.dll" );
    if ( !this->engine2 )
    {
        return false;
    }

    this->tier0 = g::memory.get_module( "tier0.dll" );
    if ( !this->tier0 )
    {
        return false;
    }

    this->schemasystem = g::memory.get_module( "schemasystem.dll" );
    if ( !this->schemasystem )
    {
        return false;
    }

    this->vphysics2 = g::memory.get_module( "vphysics2.dll" );
    if ( !this->vphysics2 )
    {
        return false;
    }

    // Опционально: другие модули
    this->inputsystem = g::memory.get_module( "inputsystem.dll" );
    this->matchmaking = g::memory.get_module( "matchmaking.dll" );
    this->soundsystem = g::memory.get_module( "soundsystem.dll" );

    g::console.print( "modules initialized." );

    return true;
}

std::uintptr_t modules::get_base( std::string_view name ) const
{
    if ( name == "client.dll" )
        return this->client;
    if ( name == "engine2.dll" )
        return this->engine2;
    if ( name == "tier0.dll" )
        return this->tier0;
    if ( name == "schemasystem.dll" )
        return this->schemasystem;
    if ( name == "vphysics2.dll" )
        return this->vphysics2;
    if ( name == "inputsystem.dll" )
        return this->inputsystem;
    if ( name == "matchmaking.dll" )
        return this->matchmaking;
    if ( name == "soundsystem.dll" )
        return this->soundsystem;
        
    return 0;
}
