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
#include <directfb_strings.h>
#include <directfb_util.h>

/**********************************************************************************************************************/

/*
 * Maximum number of layer ids.
 */
#define DFB_DISPLAYLAYER_IDS_MAX 32

static const DirectFBDisplayLayerTypeFlagsNames(layer_types);
static const DirectFBDisplayLayerCapabilitiesNames(layer_caps);

static const DirectFBInputDeviceTypeFlagsNames(input_types);
static const DirectFBInputDeviceCapabilitiesNames(input_caps);

static const DirectFBScreenCapabilitiesNames(screen_caps);
static const DirectFBScreenMixerCapabilitiesNames(mixer_caps);
static const DirectFBScreenEncoderTypeNames(encoder_type);
static const DirectFBScreenEncoderCapabilitiesNames(encoder_caps);
static const DirectFBScreenEncoderTVStandardsNames(tv_standards);
static const DirectFBScreenEncoderPictureFramingNames(framings);
static const DirectFBScreenOutputCapabilitiesNames(output_caps);
static const DirectFBScreenOutputConnectorsNames(connectors);
static const DirectFBScreenOutputResolutionNames(resolutions);
static const DirectFBScreenOutputSignalsNames(signals);

static IDirectFB *dfb;

/**********************************************************************************************************************/

static DFBEnumerationResult input_device_callback( DFBInputDeviceID id, DFBInputDeviceDescription desc, void *arg )
{
     int i;

     /* Name */
     printf( "Input Device (%02x) %-30s", id, desc.name );

     switch (id) {
          case DIDID_JOYSTICK:
               printf( "  (primary joystick)" );
               break;
          case DIDID_KEYBOARD:
               printf( "  (primary keyboard)" );
               break;
          case DIDID_MOUSE:
               printf( "  (primary mouse)" );
               break;
          case DIDID_REMOTE:
               printf( "  (primary remote control)" );
               break;
          default:
               break;
     }

     printf( "\n" );

     printf( "   Vendor  ID: 0x%04x\n", (unsigned int) desc.vendor_id );
     printf( "   Product ID: 0x%04x\n", (unsigned int) desc.product_id );

     /* Type */
     printf( "   Type: " );

     for (i = 0; input_types[i].type; i++) {
          if (desc.type & input_types[i].type)
               printf( "%s ", input_types[i].name );
     }

     printf( "\n" );

     /* Caps */
     printf( "   Caps: " );

     for (i = 0; input_caps[i].capability; i++) {
          if (desc.caps & input_caps[i].capability)
               printf( "%s ", input_caps[i].name );
     }

     printf( "\n" );

     return DFENUM_OK;
}

static void enum_input_devices()
{
     DFBResult ret;

     ret = dfb->EnumInputDevices( dfb, input_device_callback, NULL );
     if (ret)
          DirectFBError( "EnumInputDevices() failed", ret );
}

/**********************************************************************************************************************/

static DFBEnumerationResult display_layer_callback( DFBDisplayLayerID id, DFBDisplayLayerDescription desc, void *arg )
{
     int i;

     /* Name */
     printf( "     Layer (%02x) %-30s", id, desc.name );

     switch (id) {
          case DLID_PRIMARY:
               printf( "  (primary layer)" );
               break;
          default:
               break;
     }

     printf( "\n" );

     /* Type */
     printf( "        Type:    " );

     for (i = 0; layer_types[i].type; i++) {
          if (desc.type & layer_types[i].type)
               printf( "%s ", layer_types[i].name );
     }

     printf( "\n" );

     /* Caps */
     printf( "        Caps:    " );

     for (i = 0; layer_caps[i].capability; i++) {
          if (desc.caps & layer_caps[i].capability)
               printf( "%s ", layer_caps[i].name );
     }

     printf( "\n" );

     /* Sources */
     if (desc.caps & DLCAPS_SOURCES) {
          DFBResult              ret;
          IDirectFBDisplayLayer *layer;

          ret = dfb->GetDisplayLayer( dfb, id, &layer );
          if (ret) {
               DirectFBError( "GetDisplayLayer() failed", ret );
          }
          else {
               DFBDisplayLayerSourceDescription *descs;

               descs = D_CALLOC( desc.sources, sizeof(*descs) );
               if (descs) {
                    ret = layer->GetSourceDescriptions( layer, descs );
                    if (ret) {
                         DirectFBError( "GetSourceDescriptions() failed", ret );
                    }
                    else {
                         printf( "        Sources: " );

                         for (i = 0; i < desc.sources; i++) {
                              if (i > 0)
                                   printf( ", %s", descs[i].name );
                              else
                                   printf( "%s", descs[i].name );
                         }

                         printf( "\n" );
                    }

                    D_FREE( descs );
               }
               else
                    D_OOM();

               layer->Release( layer );
          }
     }

     printf( "\n" );

     return DFENUM_OK;
}

static void enum_display_layers( IDirectFBScreen *screen )
{
     DFBResult ret;

     ret = screen->EnumDisplayLayers( screen, display_layer_callback, NULL );
     if (ret)
          DirectFBError( "EnumDisplayLayers() failed", ret );
}

/**********************************************************************************************************************/

static void dump_mixers( IDirectFBScreen *screen, int num )
{
     unsigned int               i, n;
     DFBResult                  ret;
     DFBScreenMixerDescription *descs;

     descs = D_CALLOC( num, sizeof(*descs) );
     if (!descs) {
          D_OOM();
          return;
     }

     ret = screen->GetMixerDescriptions( screen, descs );
     if (ret) {
          DirectFBError( "GetMixerDescriptions() failed", ret );
          D_FREE( descs );
          return;
     }

     for (i = 0; i < num; i++) {
          printf( "   Mixer (%u) %s\n", i, descs[i].name );

          /* Caps */
          printf( "     Caps:                    " );

          for (n = 0; mixer_caps[n].capability; n++) {
               if (descs[i].caps & mixer_caps[n].capability)
                    printf( "%s ", mixer_caps[n].name );
          }

          printf( "\n" );

          /* Full mode layers */
          if (descs[i].caps & DSMCAPS_FULL) {
               printf( "     Layers (full mode):      " );

               for (n = 0; n < DFB_DISPLAYLAYER_IDS_MAX; n++) {
                    if (DFB_DISPLAYLAYER_IDS_HAVE( descs[i].layers, n ))
                         printf( "(%02x) ", n );
               }

               printf( "\n" );
          }

          /* Sub mode layers */
          if (descs[i].caps & DSMCAPS_SUB_LAYERS) {
               printf( "     Layers (sub mode): %2d of ", descs[i].sub_num );

               for (n = 0; n < DFB_DISPLAYLAYER_IDS_MAX; n++) {
                    if (DFB_DISPLAYLAYER_IDS_HAVE( descs[i].sub_layers, n ))
                         printf( "(%02x) ", n );
               }

               printf( "\n" );
          }

          printf( "\n" );
     }

     printf( "\n" );

     D_FREE( descs );
}

static void dump_encoders( IDirectFBScreen *screen, int num )
{
     int                          i, n;
     DFBResult                    ret;
     DFBScreenEncoderDescription *descs;
     DFBScreenEncoderConfig       conf;

     descs = D_CALLOC( num, sizeof(*descs) );
     if (!descs) {
          D_OOM();
          return;
     }

     ret = screen->GetEncoderDescriptions( screen, descs );
     if (ret) {
          DirectFBError( "GetEncoderDescriptions() failed", ret );
          D_FREE( descs );
          return;
     }

     for (i = 0; i < num; i++) {
          printf( "   Encoder (%d) %s\n", i, descs[i].name );

          /* Type */
          printf( "     Type:           " );

          for (n = 0; encoder_type[n].type; n++) {
               if (descs[i].type & encoder_type[n].type)
                    printf( "%s ", encoder_type[n].name );
          }

          printf( "\n" );

          /* Caps */
          printf( "     Caps:           " );

          for (n = 0; encoder_caps[n].capability; n++) {
               if (descs[i].caps & encoder_caps[n].capability)
                    printf( "%s ", encoder_caps[n].name );
          }

          printf( "\n" );

          /* TV Standards */
          if (descs[i].caps & DSECAPS_TV_STANDARDS) {
               printf( "     TV Standards:   " );

               for (n = 0; tv_standards[n].standard; n++) {
                    if (descs[i].tv_standards & tv_standards[n].standard)
                         printf( "%s ", tv_standards[n].name );
               }

               printf( "\n" );
          }

          /* Output Signals */
          if (descs[i].caps & DSECAPS_OUT_SIGNALS) {
               printf( "     Output Signals: " );

               for (n = 0; signals[n].signal; n++) {
                    if (descs[i].out_signals & signals[n].signal)
                         printf( "%s ", signals[n].name );
               }

               printf( "\n" );
          }

          /* Output Resolutions */
          if (descs[i].caps & DSECAPS_RESOLUTION) {
               printf( "     Output Resolutions: " );

               for (n = 0; resolutions[n].resolution; n++) {
                    if (descs[i].all_resolutions & resolutions[n].resolution)
                         printf( "%s ", resolutions[n].name );
               }

               printf( "\n" );
          }

          /* Output Connectors */
          if (descs[i].caps & DSECAPS_CONNECTORS) {
               printf( "     Output Connectors: " );

               for (n = 0; connectors[n].connector; n++) {
                    if (descs[i].all_connectors & connectors[n].connector)
                         printf( "%s ", connectors[n].name );
               }

               printf( "\n" );
          }

          /* Picture Framing */
          if (descs[i].caps & DSECAPS_FRAMING) {
               printf( "     Framing:        " );

               for (n = 0; framings[n].framing; n++) {
                    if (descs[i].all_framing & framings[n].framing)
                         printf( "%s ", framings[n].name );
               }

               printf( "\n" );
          }

          ret = screen->GetEncoderConfiguration( screen, i, &conf );
          if (ret) {
               DirectFBError( "GetEncoderConfiguration() failed", ret );
               D_FREE( descs );
               return;
          }

          if (conf.flags & DSECONF_MIXER)
               printf( "     Mixer:          %d\n", conf.mixer );

          printf( "\n" );
     }

     printf( "\n" );

     D_FREE( descs );
}

static void dump_outputs( IDirectFBScreen *screen, int num )
{
     int                         i, n;
     DFBResult                   ret;
     DFBScreenOutputDescription *descs;
     DFBScreenOutputConfig       conf;

     descs = D_CALLOC( num, sizeof(*descs) );
     if (!descs) {
          D_OOM();
          return;
     }

     ret = screen->GetOutputDescriptions( screen, descs );
     if (ret) {
          DirectFBError( "GetOutputDescriptions() failed", ret );
          D_FREE( descs );
          return;
     }

     for (i = 0; i < num; i++) {
          printf( "   Output (%d) %s\n", i, descs[i].name );

          /* Caps */
          printf( "     Caps:       " );

          for (n = 0; output_caps[n].capability; n++) {
               if (descs[i].caps & output_caps[n].capability)
                    printf( "%s ", output_caps[n].name );
          }

          printf( "\n" );

          /* Connectors */
          if (descs[i].caps & DSOCAPS_CONNECTORS) {
              printf( "     Connectors: " );

              for (n = 0; connectors[n].connector; n++) {
                   if (descs[i].all_connectors & connectors[n].connector)
                        printf( "%s ", connectors[n].name );
              }

              printf( "\n" );
          }

          /* Resolutions */
          if (descs[i].caps & DSOCAPS_RESOLUTION) {
               printf( "     Resolutions: " );

               for (n = 0; resolutions[n].resolution; n++) {
                    if (descs[i].all_resolutions & resolutions[n].resolution)
                         printf( "%s ", resolutions[n].name );
               }

               printf( "\n" );
          }

          /* Signals */
          printf( "     Signals:    " );

          for (n = 0; signals[n].signal; n++) {
               if (descs[i].all_signals & signals[n].signal)
                    printf( "%s ", signals[n].name );
          }

          printf( "\n" );

          ret = screen->GetOutputConfiguration( screen, i, &conf );
          if (ret) {
               DirectFBError( "GetOutputConfiguration() failed", ret );
               D_FREE( descs );
               return;
          }

          if (conf.flags & DSOCONF_ENCODER)
               printf( "     Encoder:    %d\n", conf.encoder );

          printf( "\n" );
     }

     printf( "\n" );

     D_FREE( descs );
}

static DFBEnumerationResult screen_callback( DFBScreenID id, DFBScreenDescription desc, void *arg )
{
     int              i;
     DFBResult        ret;
     IDirectFBScreen *screen;

     ret = dfb->GetScreen( dfb, id, &screen );
     if (ret)
          DirectFBErrorFatal( "GetScreen() failed", ret );

     /* Name */
     printf( "Screen (%02x) %-30s", id, desc.name );

     switch (id) {
          case DSCID_PRIMARY:
               printf( "  (primary screen)" );
               break;
          default:
               break;
     }

     printf( "\n" );

     /* Caps */
     printf( "   Caps: " );

     for (i = 0; screen_caps[i].capability; i++) {
          if (desc.caps & screen_caps[i].capability)
               printf( "%s ", screen_caps[i].name );
     }

     printf( "\n" );

     /* Mixers */
     if (desc.mixers)
          dump_mixers( screen, desc.mixers );

     /* Encoders */
     if (desc.encoders)
          dump_encoders( screen, desc.encoders );

     /* Outputs */
     if (desc.outputs)
          dump_outputs( screen, desc.outputs );

     /* Display layers */
     enum_display_layers( screen );

     screen->Release( screen );

     return DFENUM_OK;
}

static void enum_screens()
{
     DFBResult ret;

     ret = dfb->EnumScreens( dfb, screen_callback, NULL );
     if (ret)
          DirectFBError( "EnumScreens() failed", ret );
}

/**********************************************************************************************************************/

static DFBEnumerationResult video_mode_callback( int width, int height, int bpp, void *arg )
{
     printf( "Video Mode %dx%d-%d\n", width, height, bpp );

     printf( "\n" );

     return DFENUM_OK;
}

static void enum_video_modes()
{
     DFBResult ret;

     ret = dfb->EnumVideoModes( dfb, video_mode_callback, NULL );
     if (ret)
          DirectFBError( "EnumVideoModes() failed", ret );
}

/**********************************************************************************************************************/

int main( int argc, char *argv[] )
{
     DFBResult ret;

     /* Initialize DirectFB including command line parsing. */
     ret = DirectFBInit( &argc, &argv );
     if (ret) {
          DirectFBError( "DirectFBInit() failed", ret );
          return 1;
     }

     DirectFBSetOption( "bg-none", NULL );
     DirectFBSetOption( "no-cursor", NULL );

     /* Create the main interface. */
     ret = DirectFBCreate( &dfb );
     if (ret) {
          DirectFBError( "DirectFBCreate() failed", ret );
          return 1;
     }

     enum_video_modes();
     enum_screens();
     enum_input_devices();

     /* Release the main interface. */
     dfb->Release( dfb );

     return 0;
}
