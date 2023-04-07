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

#include <dfiff.h>

/**********************************************************************************************************************/

int main( int argc, char *argv[] )
{
     int            bpp;
     DFBColor       color;
     DFBColorKey    colorkey;
     DFIFFHeader    header;
     unsigned long  i;
     char          *error;
     FILE          *fp   = NULL;
     unsigned char *data = NULL;
     int            err  = 1;

     /* Parse the command line. */
     if (argc != 4) {
          fprintf( stderr, "\nDirectFB Fast Image File Format Color Keying\n\n" );
          fprintf( stderr, "Usage: %s <color> <colorkey> <image>\n", argv[0] );
          fprintf( stderr, "\n" );
          goto out;
     }

     /* Parse color string. */
     i = strtoul( argv[1], &error, 16 );
     if (*error) {
          fprintf( stderr, "Invalid characters in color string!\n" );
          goto out;
     }

     color.r = (i & 0xff0000) >> 16;
     color.g = (i & 0xff00)   >>  8;
     color.b = (i & 0xff)     >>  0;

     /* Parse color key string. */
     i = strtoul( argv[2], &error, 16 );
     if (*error) {
          fprintf( stderr, "Invalid characters in color key string!\n" );
          goto out;
     }

     colorkey.r = (i & 0xff0000) >> 16;
     colorkey.g = (i & 0xff00)   >>  8;
     colorkey.b = (i & 0xff)     >>  0;

     /* Open the file. */
     fp = fopen( argv[3], "rb" );
     if (!fp) {
          fprintf( stderr, "Failed to open '%s'!\n", argv[3] );
          goto out;
     }

     /* Read the file. */
     fread( &header, sizeof(header), 1, fp );

     data = calloc( header.height, header.pitch );
     if (!data) {
          fprintf( stderr, "Failed to allocate %u bytes!\n", header.height * header.pitch );
          goto out;
     }

     fread( data, header.height, header.pitch, fp );

     /* Write file with color key. */
     fwrite( &header, sizeof(header), 1, stdout );

     bpp = DFB_BYTES_PER_PIXEL( header.format );

     switch (header.format) {
          case DSPF_RGB24:
          case DSPF_RGB32:
               for (i = 0; i < header.height * header.pitch; i += bpp) {
                    if (data[i] == color.r && data[i+1] == color.g && data[i+2] == color.b) {
                         data[i+2] = colorkey.r;
                         data[i+1] = colorkey.g;
                         data[i+0] = colorkey.b;
                    }
               }
               break;

          default:
               fprintf( stderr, "Unsupported format for color keying!\n" );
               goto out;
     }

     fwrite( data, header.height, header.pitch, stdout );

     err = 0;

out:
     if (data)
          free( data );

     if (fp)
          fclose( fp );

     return err;
}
