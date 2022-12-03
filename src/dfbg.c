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

static IDirectFB             *dfb   = NULL;
static IDirectFBDisplayLayer *layer = NULL;

static const char *background    = NULL;
static DFBBoolean  tiled         = DFB_FALSE;
static DFBBoolean  premultiplied = DFB_FALSE;

static DFBBoolean parse_command_line  ( int argc, char *argv[] );
static void       set_background_color( u32 argb );
static void       set_background_image( const char *filename );

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

     DirectFBSetOption( "bg-none", NULL );
     DirectFBSetOption( "no-cursor", NULL );

     /* Create the main interface. */
     ret = DirectFBCreate( &dfb );
     if (ret) {
          DirectFBError( "DirectFBCreate() failed", ret );
          return -3;
     }

     /* Get the primary display layer. */
     ret = dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer );
     if (ret) {
          DirectFBError( "GetDisplayLayer() failed", ret );
          dfb->Release( dfb );
          return -4;
     }

     /* Acquire administrative cooperative level. */
     ret = layer->SetCooperativeLevel( layer, DLSCL_ADMINISTRATIVE );
     if (ret) {
          DirectFBError( "SetCooperativeLevel() failed", ret );
          layer->Release( layer );
          dfb->Release( dfb );
          return -5;
     }

     /* Set the background as desired by the user. */
     if (access( background, R_OK ) == DR_OK) {
          set_background_image( background );
     }
     else {
          u32   argb;
          char *error;

          argb = strtoul( background, &error, 16 );

          if (!*error)
               set_background_color( argb );
          else
               fprintf( stderr, "Invalid image file or characters in color string!\n" );
     }

     /* Release the primary display layer. */
     layer->Release( layer );

     /* Release the main interface. */
     dfb->Release( dfb );

     return 0;
}

/**********************************************************************************************************************/

static void print_usage()
{
     fprintf( stderr, "\nDirectFB Desktop Background Configuration\n\n" );
     fprintf( stderr, "Usage: dfbg [options] <imagefile>|<color>\n\n" );
     fprintf( stderr, "Options:\n" );
     fprintf( stderr, "  -t, --tile           Set tiled mode\n" );
     fprintf( stderr, "  -p, --premultiplied  Create the surface with DSCAPS_PREMULTIPLIED\n" );
     fprintf( stderr, "  -h, --help           Show this help message\n" );
     fprintf( stderr, "\n" );
}

static DFBBoolean parse_command_line( int argc, char *argv[] )
{
     int n;

     for (n = 1; n < argc; n++) {
          if (strcmp( argv[n], "-h" ) == 0 || strcmp( argv[n], "--help" ) == 0) {
               print_usage();
               return DFB_FALSE;
          }

          if (strcmp( argv[n], "-t" ) == 0 || strcmp( argv[n], "--tile" ) == 0) {
               tiled = DFB_TRUE;
               continue;
          }

          if (strcmp( argv[n], "-p" ) == 0 || strcmp( argv[n], "--premultiplied" ) == 0) {
               premultiplied = DFB_TRUE;
               continue;
          }

          if (*argv[n] != '-' && !background) {
               background = argv[n];
               continue;
          }

          print_usage();
          return DFB_FALSE;
     }

     if (!background) {
          print_usage();
          return DFB_FALSE;
     }

     return DFB_TRUE;
}

/**********************************************************************************************************************/

static void set_background_color( u32 argb )
{
     DFBResult  ret;

     ret = layer->SetBackgroundColor( layer,
                                      (argb & 0xff0000)   >> 16,
                                      (argb & 0xff00)     >>  8,
                                      (argb & 0xff)       >>  0,
                                      (argb & 0xff000000) >> 24 );
     if (ret) {
          DirectFBError( "SetBackgroundColor() failed", ret );
          return;
     }

     ret = layer->SetBackgroundMode( layer, DLBM_COLOR );
     if (ret)
          DirectFBError( "SetBackgroundMode() failed", ret );
}

static void set_background_image( const char *filename )
{
     DFBResult               ret;
     DFBSurfaceDescription   desc;
     IDirectFBSurface       *surface;
     IDirectFBImageProvider *provider;

     ret = dfb->CreateImageProvider( dfb, filename, &provider );
     if (ret) {
          DirectFBError( "CreateImageProvider() failed", ret );
          return;
     }

     ret = provider->GetSurfaceDescription( provider, &desc );
     if (ret) {
          DirectFBError( "GetSurfaceDescription() failed", ret );
          provider->Release( provider );
          return;
     }

     desc.flags |= DSDESC_CAPS;
     desc.caps   = DSCAPS_SHARED;

     if (premultiplied)
          desc.caps |= DSCAPS_PREMULTIPLIED;

     if (!tiled) {
          DFBDisplayLayerConfig   config;

          ret = layer->GetConfiguration( layer, &config );
          if (ret) {
               DirectFBError( "IDirectFBDisplayLayer::GetConfiguration() failed", ret );
               provider->Release( provider );
               return;
          }

          desc.width  = config.width;
          desc.height = config.height;
     }

     ret = dfb->CreateSurface( dfb, &desc, &surface );
     if (ret) {
          DirectFBError( "CreateSurface() failed", ret );
          provider->Release( provider );
          return;
     }

     ret = provider->RenderTo( provider, surface, NULL );
     if (ret) {
          DirectFBError( "RenderTo() failed", ret );
          surface->Release( surface );
          provider->Release( provider );
          return;
     }

     ret = layer->SetBackgroundImage( layer, surface );
     if (ret) {
          DirectFBError( "SetBackgroundImage() failed", ret );
          surface->Release( surface );
          provider->Release( provider );
          return;
     }

     ret = layer->SetBackgroundMode( layer, tiled ? DLBM_TILE : DLBM_IMAGE );
     if (ret)
          DirectFBError( "SetBackgroundMode() failed", ret );

     surface->Release( surface );
     provider->Release( provider );
}
