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

#include <directfb.h>

/**********************************************************************************************************************/

static IDirectFB       *dfb;
static IDirectFBScreen *screen;

static DFBDisplayLayerID layer_id;
static int               level;

static DFBBoolean set_layer_id = DFB_FALSE;
static DFBBoolean set_level    = DFB_FALSE;

/**********************************************************************************************************************/

static void print_usage()
{
     fprintf( stderr, "\nDirectFB Layer Level Configuration\n\n" );
     fprintf( stderr, "Usage: dflayerlevel [options]\n\n" );
     fprintf( stderr, "Options:\n" );
     fprintf( stderr, "  -l, --layer <id[@level]>  Show/change the specified display layer level\n" );
     fprintf( stderr, "  -h, --help                Show this help message\n" );
     fprintf( stderr, "\n" );
}

static DFBBoolean parse_command_line( int argc, char *argv[] )
{
     int n;

     for (n = 1; n < argc; n++) {
          const char *arg = argv[n];

          if (strcmp( arg, "-h" ) == 0 || strcmp( arg, "--help" ) == 0) {
               print_usage();
               return DFB_FALSE;
          }

          if (strcmp( arg, "-l" ) == 0 || strcmp( arg, "--layer" ) == 0) {
               char *opt;

               if (++n == argc) {
                    print_usage();
                    return DFB_FALSE;
               }

               if (sscanf( argv[n], "%d", &layer_id ) != 1 || layer_id < 0) {
                    fprintf( stderr, "Invalid layer id specified!\n" );
                    return DFB_FALSE;
               }

               set_layer_id = DFB_TRUE;

               opt = strchr( argv[n], '@' );

               if (opt) {
                    opt++;

                    if (sscanf( opt, "%d", &level ) != 1) {
                         fprintf( stderr, "Invalid layer level specified!\n" );
                         return DFB_FALSE;
                    }

                    set_level = DFB_TRUE;
               }

               continue;
          }

          print_usage();
          return DFB_FALSE;
     }

     return DFB_TRUE;
}

/**********************************************************************************************************************/

static DFBEnumerationResult display_layer_callback( DFBDisplayLayerID id, DFBDisplayLayerDescription desc, void *data )
{
     DFBResult ret;

     if (set_layer_id && id != layer_id)
          return DFENUM_OK;

     printf( "%s", desc.name );

     if (desc.caps & DLCAPS_LEVELS) {
          IDirectFBDisplayLayer *layer;

          ret = dfb->GetDisplayLayer( dfb, id, &layer );
          if (ret) {
               DirectFBError( "GetDisplayLayer() failed", ret );
               return DFENUM_CANCEL;
          }

          /* Set the level of the specified display layer if requested. */
          if (set_level) {
               ret = layer->SetCooperativeLevel( layer, DLSCL_ADMINISTRATIVE );
               if (ret) {
                    DirectFBError( "SetCooperativeLevel() failed", ret );
                    layer->Release( layer );
                    return DFENUM_CANCEL;
               }

               ret = layer->SetLevel( layer, level );
               if (ret) {
                    DirectFBError( "SetLevel() failed", ret );
                    layer->Release( layer );
                    return DFENUM_CANCEL;
               }
          }

          /* Query current level. */
          ret = layer->GetLevel( layer, &level );
          if (ret) {
               DirectFBError( "GetLevel() failed", ret );
               layer->Release( layer );
               return DFENUM_CANCEL;
          }

          printf( " @ level %2d\n", level );

          layer->Release( layer );
     }
     else
          printf( "\n" );

     return DFENUM_OK;
}

/**********************************************************************************************************************/

int main( int argc, char *argv[] )
{
     DFBResult ret;

     /* Initialize DirectFB including command line parsing. */
     ret = DirectFBInit( &argc, &argv );
     if (ret) {
          DirectFBError( "DirectFBInit() failed", ret );
          return -1;
     }

     /* Parse the command line. */
     if (!parse_command_line( argc, argv ))
          return -2;

     /* Create the main interface. */
     ret = DirectFBCreate( &dfb );
     if (ret) {
          DirectFBError( "DirectFBCreate() failed", ret );
          return -3;
     }

     /* Get the primary screen. */
     dfb->GetScreen( dfb, DSCID_PRIMARY, &screen );
     if (ret) {
          DirectFBError( "GetScreen() failed", ret );
          return -4;
     }

     /* Enumerate all display layers. */
     screen->EnumDisplayLayers( screen, display_layer_callback, NULL );

     /* Release the main interface. */
     dfb->Release( dfb );

     return 0;
}
