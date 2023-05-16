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

#include <dfvff.h>
#include <direct/util.h>
#include <directfb_strings.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

static const DirectFBPixelFormatNames(format_names);
static const DirectFBColorSpaceNames(colorspace_names);

static const char            *filename   = NULL;
static bool                   debug      = false;
static DFBSurfacePixelFormat  format     = DSPF_UNKNOWN;
static int                    width      = 0;
static int                    height     = 0;
static unsigned int           fps_num    = 0;
static unsigned int           fps_den    = 1;
static DFBSurfaceColorSpace   colorspace = DSCS_UNKNOWN;
static unsigned long          nframes    = 0;

#define DEBUG(...)                             \
     do {                                      \
          if (debug)                           \
               fprintf( stderr, __VA_ARGS__ ); \
     } while (0)

/**********************************************************************************************************************/

static void print_usage()
{
     int i = 0;

     fprintf( stderr, "DirectFB Fast Video File Format Tool\n\n" );
     fprintf( stderr, "Usage: mkdfvff [options] <video>\n\n" );
     fprintf( stderr, "Options:\n\n" );
     fprintf( stderr, "  -d, --debug                           Output debug information.\n" );
     fprintf( stderr, "  -f, --format     <pixelformat>        Choose the pixel format.\n" );
     fprintf( stderr, "  -s, --size       <width>x<height>     Set video frame size (for raw input video).\n" );
     fprintf( stderr, "  -r, --rate       <fps_num>/<fps_den>  Choose the frame rate (for raw input video).\n" );
     fprintf( stderr, "  -c, --colorspace <colorspace>         Choose the color space.\n" );
     fprintf( stderr, "  -n, --nframes    <nframes>            Set the number of video frames to output.\n" );
     fprintf( stderr, "  -h, --help                            Show this help message.\n\n" );
     fprintf( stderr, "Supported pixel formats:\n\n" );
     while (format_names[i].format != DSPF_UNKNOWN) {
          if (DFB_BYTES_PER_PIXEL( format_names[i].format ) < 3 &&
              DFB_COLOR_IS_YUV   ( format_names[i].format )) {
               fprintf( stderr, "  %-10s %2d byte(s)",
                        format_names[i].name, DFB_BYTES_PER_PIXEL( format_names[i].format ) );
               if (DFB_PLANAR_PIXELFORMAT( format_names[i].format )) {
                    int planes = DFB_PLANE_MULTIPLY( format_names[i].format, 10 );
                    fprintf( stderr, " (x %d.%d)", planes / 10, planes % 10 );
               }
               fprintf( stderr, "\n" );
          }
          ++i;
     }
     fprintf( stderr, "\n" );
     i = 0;
     fprintf( stderr, "Supported color spaces:\n\n" );
     while (colorspace_names[i].colorspace != DSCS_UNKNOWN) {
          DFBSurfaceColorSpace colorspace = colorspace_names[i].colorspace;
          if (colorspace != DSCS_RGB) {
               fprintf( stderr, "  %s\n", colorspace_names[i].name );
          }
          ++i;
     }
     fprintf( stderr, "\n" );
}

static DFBBoolean parse_format( const char *arg )
{
     int i = 0;

     while (format_names[i].format != DSPF_UNKNOWN) {
          if (!strcasecmp( arg, format_names[i].name )          &&
              DFB_BYTES_PER_PIXEL( format_names[i].format ) < 3 &&
              DFB_COLOR_IS_YUV   ( format_names[i].format )) {
               format = format_names[i].format;
               return DFB_TRUE;
          }
          ++i;
     }

     fprintf( stderr, "Invalid pixel format specified!\n" );

     return DFB_FALSE;
}

static DFBBoolean parse_size( const char *arg )
{
     if (sscanf( arg, "%dx%d", &width, &height ) == 2)
          return DFB_TRUE;

     fprintf( stderr, "Invalid size specified!\n" );

     return DFB_FALSE;
}

static DFBBoolean parse_rate( const char *arg )
{
     if (sscanf( arg, "%u/%u", &fps_num, &fps_den ) == 2)
          return DFB_TRUE;

     fprintf( stderr, "Invalid frame rate specified!\n" );

     return DFB_FALSE;
}

static DFBBoolean parse_colorspace( const char *arg )
{
     int i = 0;

     while (colorspace_names[i].colorspace != DSCS_UNKNOWN) {
          if (!strcasecmp( arg, colorspace_names[i].name ) &&
              colorspace_names[i].colorspace != DSCS_RGB) {
               colorspace = colorspace_names[i].colorspace;
               return DFB_TRUE;
          }

          ++i;
     }

     fprintf( stderr, "Invalid color space specified!\n" );

     return DFB_FALSE;
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

          if (strcmp( arg, "-d" ) == 0 || strcmp( arg, "--debug" ) == 0) {
               debug = true;
               continue;
          }

          if (strcmp( arg, "-f" ) == 0 || strcmp( arg, "--format" ) == 0) {
               if (++n == argc) {
                    print_usage();
                    return DFB_FALSE;
               }

               if (!parse_format( argv[n] ))
                    return DFB_FALSE;

               continue;
          }

          if (strcmp( arg, "-s" ) == 0 || strcmp( arg, "--size" ) == 0) {
               if (++n == argc) {
                    print_usage();
                    return DFB_FALSE;
               }

               if (!parse_size( argv[n] ))
                    return DFB_FALSE;

               continue;
          }

          if (strcmp( arg, "-r" ) == 0 || strcmp( arg, "--rate" ) == 0) {
               if (++n == argc) {
                    print_usage();
                    return DFB_FALSE;
               }

               if (!parse_rate( argv[n] ))
                    return DFB_FALSE;

               continue;
          }

          if (strcmp( arg, "-c" ) == 0 || strcmp( arg, "--colorspace" ) == 0) {
               if (++n == argc) {
                    print_usage();
                    return DFB_FALSE;
               }

               if (!parse_colorspace( argv[n] ))
                    return DFB_FALSE;

               continue;
          }

          if (strcmp( arg, "-n" ) == 0 || strcmp( arg, "--nframes" ) == 0) {
               if (++n == argc) {
                    print_usage();
                    return DFB_FALSE;
               }

               nframes = strtoul( argv[n], NULL, 10 );

               continue;
          }

          if (filename || access( arg, R_OK )) {
               print_usage();
               return DFB_FALSE;
          }

          filename = arg;
     }

     if (!filename) {
          print_usage();
          return DFB_FALSE;
     }

     return DFB_TRUE;
}

/**********************************************************************************************************************/

static void *load_video( DFBSurfaceDescription *desc )
{
     DFBSurfacePixelFormat  dest_format;
     int                    frame_size;
     void                  *handle;
     FILE                  *fp      = NULL;
     AVFormatContext       *fmt_ctx = NULL;
     unsigned char         *data    = NULL;

     desc->flags = DSDESC_NONE;

     if (width && height) {
          if (!fps_num) {
               fprintf( stderr, "Specifying the frame rate is also required!\n" );
               goto out;
          }

          if (!format) {
               fprintf( stderr, "No format specified!\n" );
               goto out;
          }

          fp = fopen( filename, "rb" );
          if (!fp) {
               fprintf( stderr, "Failed to open '%s'!\n", filename );
               goto out;
          }

          if (!colorspace)
               colorspace = width >= 1280 || height > 576 ? DSCS_BT709 : DSCS_BT601;

          dest_format = format;
          frame_size  = DFB_BYTES_PER_LINE( format, width ) * DFB_PLANE_MULTIPLY( format, height );

          data = malloc( frame_size );
          if (!data) {
               fprintf( stderr, "Failed to allocate %d bytes!\n", frame_size );
               goto out;
          }
          else {
               if (!nframes) {
                    struct stat st;

                    if (stat( filename, &st ) < 0) {
                         fprintf( stderr, "Failed to get file status!\n" );
                         goto out;
                    }

                    nframes = st.st_size / frame_size;
               }
          }

          handle = fp;
     }
     else {
          DFBSurfacePixelFormat src_format;

          av_register_all();

          if (avformat_open_input( &fmt_ctx, filename, NULL, NULL ) < 0) {
               fprintf( stderr, "Failed to open '%s'!\n", filename );
               goto out;
          }

          if (avformat_find_stream_info( fmt_ctx, NULL ) < 0) {
               fprintf( stderr, "Couldn't find stream info!\n" );
               goto out;
          }

          width   = fmt_ctx->streams[0]->codec->width;
          height  = fmt_ctx->streams[0]->codec->height;
          fps_num = fmt_ctx->streams[0]->avg_frame_rate.num;
          fps_den = fmt_ctx->streams[0]->avg_frame_rate.den;

          switch (fmt_ctx->streams[0]->codec->colorspace) {
               case AVCOL_SPC_BT709:
                    colorspace = DSCS_BT709;
                    break;
               case AVCOL_SPC_BT470BG:
               case AVCOL_SPC_SMPTE170M:
                    colorspace = DSCS_BT601;
                    break;
               case AVCOL_SPC_BT2020_NCL:
                    colorspace = DSCS_BT2020;
                    break;
               default:
                    colorspace = width >= 1280 || height > 576 ? DSCS_BT709 : DSCS_BT601;
                    break;
          }

          switch (fmt_ctx->streams[0]->codec->pix_fmt) {
               case AV_PIX_FMT_YUV420P:
                    src_format = DSPF_I420;
                    break;
               case AV_PIX_FMT_YUV422P:
                    src_format = DSPF_Y42B;
                    break;
               case AV_PIX_FMT_YUV444P:
                    src_format = DSPF_Y444;
                    break;
               default:
                    fprintf( stderr, "Invalid pixel format!\n" );
                    goto out;
          }

          dest_format = format ?: src_format;
          frame_size = DFB_BYTES_PER_LINE( dest_format, width ) * DFB_PLANE_MULTIPLY( dest_format, height );

          data = malloc( frame_size );
          if (!data) {
               fprintf( stderr, "Failed to allocate %d bytes!\n", frame_size );
               goto out;
          }
          else {
               if (avcodec_open2( fmt_ctx->streams[0]->codec,
                                  avcodec_find_decoder( fmt_ctx->streams[0]->codec->codec_id ), NULL)) {
                    fprintf( stderr, "Failed to open video codec!\n" );
                    goto out;
               }
          }

          handle = fmt_ctx;
     }

     desc->flags                 = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_PREALLOCATED |
                                   DSDESC_COLORSPACE;
     desc->width                 = width;
     desc->height                = height;
     desc->pixelformat           = dest_format;
     desc->preallocated[0].data  = data;
     desc->preallocated[0].pitch = DFB_BYTES_PER_LINE( format, width );
     desc->colorspace            = colorspace;

     fp      = NULL;
     fmt_ctx = NULL;
     data    = NULL;

out:
     if (data)
          free( data );

     if (fp)
          fclose( fp );

     if (fmt_ctx)
          avformat_close_input( &fmt_ctx );

     return desc->flags ? handle : NULL;
}

static void write_frames( void *handle, DFBSurfaceDescription *desc )
{
     int                frame_size;
     unsigned long      frames_decoded;
     FILE              *fp      = NULL;
     AVFormatContext   *fmt_ctx = NULL;
     struct SwsContext *sws_ctx = NULL;
     AVFrame           *frame   = NULL;

     frame_size = DFB_BYTES_PER_LINE( desc->pixelformat, width ) * DFB_PLANE_MULTIPLY( desc->pixelformat, height );

     if (!avcodec_find_decoder_by_name( "h264" )) {
          fp = handle;

          for (frames_decoded = 0; frames_decoded < nframes; frames_decoded++) {
               if (fread( desc->preallocated[0].data, 1, frame_size, fp ) != frame_size) {
                   fprintf( stderr, "Failed to read raw file!\n" );
                   goto out;
               }

               fwrite( desc->preallocated[0].data, 1, frame_size, stdout );
          }
     }
     else {
          enum AVPixelFormat  pix_fmt;
          AVPicture           picture;
          AVPacket            pkt;
          int                 got_frame = 0;

          fmt_ctx = handle;

          switch (desc->pixelformat) {
               case DSPF_I420:
                    pix_fmt = AV_PIX_FMT_YUV420P;
                    break;
               case DSPF_Y42B:
                    pix_fmt = AV_PIX_FMT_YUV422P;
                    break;
               case DSPF_Y444:
                    pix_fmt = AV_PIX_FMT_YUV444P;
                    break;
               default:
                    fprintf( stderr, "Unsupported format conversion!\n" );
                    return;
          }

          sws_ctx = sws_getContext( width, height, fmt_ctx->streams[0]->codec->pix_fmt, width, height, pix_fmt,
                                    SWS_FAST_BILINEAR, NULL, NULL, NULL );

          avpicture_fill( &picture, desc->preallocated[0].data, pix_fmt, width, height );

          frame = av_frame_alloc();
          if (!frame)
               goto out;
          else
               frames_decoded = 0;

          while (av_read_frame( fmt_ctx, &pkt ) >= 0) {
               avcodec_decode_video2( fmt_ctx->streams[0]->codec, frame, &got_frame, &pkt );

               av_free_packet( &pkt );

               if (got_frame) {
                    sws_scale( sws_ctx, (void*) frame->data, frame->linesize, 0, height, picture.data, picture.linesize );

                    fwrite( desc->preallocated[0].data, 1, frame_size, stdout );

                    frames_decoded++;
                    if (frames_decoded == nframes)
                         break;
               }
          }
     }

out:
     if (fp)
          fclose( fp );

     if (frame)
          av_free( frame );

     if (sws_ctx)
          sws_freeContext( sws_ctx );

     if (fmt_ctx) {
          avcodec_close( fmt_ctx->streams[0]->codec );
          avformat_close_input( &fmt_ctx );
     }
}

/**********************************************************************************************************************/

static DFVFFHeader header = {
     magic: { 'D', 'F', 'V', 'F', 'F' },
     major: 0,
     minor: 0,
     flags: 0x01
};

int main( int argc, char *argv[] )
{
     int                    i, j;
     DFBSurfaceDescription  desc;
     void                  *handle;

     /* Parse the command line. */
     if (!parse_command_line( argc, argv ))
          return -1;

     handle = load_video( &desc );
     if (!handle)
          return -2;

     for (i = 0; i < D_ARRAY_SIZE(format_names); i++) {
          if (format_names[i].format == desc.pixelformat) {
               for (j = 0; j < D_ARRAY_SIZE(colorspace_names); j++) {
                    if (colorspace_names[j].colorspace == desc.colorspace) {
                         DEBUG( "Writing video (%lu frames): %dx%d, %s (%s), %u/%u fps\n", nframes,
                                desc.width, desc.height, format_names[i].name, colorspace_names[j].name,
                                fps_num, fps_den );
                         break;
                    }
               }
          }
     }

     header.width      = desc.width;
     header.height     = desc.height;
     header.format     = desc.pixelformat;
     header.colorspace = desc.colorspace;

     header.framerate_num = fps_num;
     header.framerate_den = fps_den;

     fwrite( &header, sizeof(header), 1, stdout );

     write_frames( handle, &desc );

     free( desc.preallocated[0].data );

     return 0;
}
