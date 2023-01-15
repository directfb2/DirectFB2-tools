/*
   This file is part of DirectFB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include <fusionsound.h>

/**********************************************************************************************************************/

int main( int argc, char *argv[] )
{
     DirectResult  ret;
     IFusionSound *sound;
     float         volume = 0.0f;

     if (argc > 1) {
          if (!strcmp( argv[1], "-h" ) || !strcmp( argv[1], "--help" )) {
               fprintf( stderr, "\nFusionSound Master Volume Configuration\n\n" );
               fprintf( stderr, "Usage: fsvolume [value]\n\n" );
               fprintf( stderr, "  If no value is given\n" );
               fprintf( stderr, "      Print current volume level\n\n" );
               fprintf( stderr, "  [0.0 ... 1.0]\n" );
               fprintf( stderr, "      Set volume level to given value\n\n" );
               fprintf( stderr, "  +/-[0.0 ... 1.0]\n" );
               fprintf( stderr, "      Adjust volume level by given value\n\n" );
               return 1;
          }
     }

     ret = FusionSoundInit( &argc, &argv );
     if (ret)
          FusionSoundErrorFatal( "FusionSoundInit() failed", ret );

     ret = FusionSoundCreate( &sound );
     if (ret)
          FusionSoundErrorFatal( "FusionSoundCreate() failed", ret );

     if (argc > 1) {
          if (argv[1][0] == '+' || argv[1][0] == '-') {
               ret = sound->GetMasterVolume( sound, &volume );
               if (ret)
                    FusionSoundError( "GetMasterVolume() failed", ret );
          }

          ret = sound->SetMasterVolume( sound, volume + strtof( argv[1], NULL ) );
          if (ret)
               FusionSoundError( "SetMasterVolume() failed", ret );
     }
     else {
          ret = sound->GetMasterVolume( sound, &volume );
          if (ret)
               FusionSoundError( "GetMasterVolume() failed", ret );

          printf( "%.3f\n", volume );
     }

     sound->Release( sound );

     return ret;
}
