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

#include <directfb_keynames.h>
#include <divine.h>
#include <math.h>

/**********************************************************************************************************************/

static const DirectFBKeySymbolNames(symnames);

static IDiVine *divine;

FILE *stream;

static int  sin_table[360 * 16];
static int  axisabs_x     = 0;
static int  axisabs_y     = 0;
static long loop_position = 0;

/**********************************************************************************************************************/

static void print_usage()
{
     fprintf( stderr, "\nDiVine event generator\n\n"
                      "Usage: spooky [filename]\n\n"
                      "If no filename is given, spooky reads from standard input\n\n"
                      "Commands:\n"
                      "  Code  Description               Arguments & Options                 Examples\n"
                      "  b     Press/release button      {+|-}<button>                       b+0 b-0\n"
                      "  m     Motion event (absolute)   {x|y}<position>                     mx160 my120\n"
                      "  M     Special motion generator  <ms> {c<count>|l<x,y>|o<radius>     M20 c40l4,2 c90o50\n"
                      "  p     Pause spooky              <ms>                                p1000\n"
                      "  t     Entering text             <ms> <text>                         t200 directfb\n"
                      "  T     Special characters/keys   <ms> {b|d|r|Cl|Cr|Cu|Cd|S<symbol>}  T200 bdr CuCl Sprint Sf7\n"
                      "  l     Loop between here and eof\n"
                      "\n" );
}

static DFBBoolean parse_command_line( int argc, char *argv[] )
{
     if (argc > 1) {
          if (!strcmp( argv[1], "-h" ) || !strcmp( argv[1], "--help" ) || argc > 2) {
               print_usage();
               return DFB_FALSE;
          }

          stream = fopen( argv[1], "r" );
          if (!stream) {
               fprintf( stderr, "Could not open '%s' for reading!\n", argv[1] );
               return DFB_FALSE;
          }
     }
     else
          stream = stdin;

     return DFB_TRUE;
}

/**********************************************************************************************************************/

static void button_handler( FILE *stream )
{
     DFBInputEvent event;

     memset( &event, 0, sizeof(event) );

     if (fgetc( stream ) == '+')
          event.type = DIET_BUTTONPRESS;
     else
          event.type = DIET_BUTTONRELEASE;

     event.button = fgetc( stream ) - '0';

     divine->SendEvent( divine, &event );
}

static void motion_handler( FILE *stream )
{
     DFBInputEvent event;

     memset( &event, 0, sizeof(event) );
     event.flags = DIEF_AXISABS;
     event.type  = DIET_AXISMOTION;
     event.axis  = fgetc( stream ) - 'x';

     if (fscanf( stream, "%d", &event.axisabs ) != 1)
          return;

     switch (event.axis) {
          case DIAI_X:
               axisabs_x = event.axisabs;
               break;

          case DIAI_Y:
               axisabs_y = event.axisabs;
               break;

          default:
               break;
     }

     divine->SendEvent( divine, &event );
}

static void motion_special_handler( FILE *stream )
{
     int           i, current_x = axisabs_x, current_y = axisabs_y;
     int ms = 0;
     int           r, x, y;
     int           count = 1;
     DFBInputEvent event;

     fscanf( stream, "%d", &ms );

     while (!feof( stream )) {
          switch (fgetc( stream )) {
               case '\n':
                    return;

               case 'c':
                    if (fscanf( stream, "%d", &count ) != 1)
                         return;
                    break;

               case 'o':
                    if (fscanf( stream, "%d", &r ) != 1)
                         return;

                    for (i = 0; i < count; i++) {
                         memset( &event, 0, sizeof(event) );
                         event.flags   = DIEF_AXISABS | DIEF_FOLLOW;
                         event.type    = DIET_AXISMOTION;
                         event.axis    = DIAI_X;
                         event.axisabs = axisabs_x + ((r * sin_table[(((i % 360) + 90) * 16) % (360 * 16)]) >> 16) - r;

                         divine->SendEvent( divine, &event );

                         current_x = event.axisabs;

                         event.axis    = DIAI_Y;
                         event.axisabs = axisabs_y + ((r * sin_table[((i % 360) * 16) % (360 * 16)]) >> 16);

                         divine->SendEvent( divine, &event );

                         current_y = event.axisabs;

                         usleep( ms * 1000 );
                    }

                    axisabs_x = current_x;
                    axisabs_y = current_y;
                    break;

               case 'l':
                    if (fscanf( stream, "%d,%d", &x, &y ) != 2)
                         return;

                    for (i = 0; i < count; i++) {
                         memset( &event, 0, sizeof(event) );
                         event.flags   = DIEF_AXISABS | DIEF_FOLLOW;
                         event.type    = DIET_AXISMOTION;
                         event.axis    = DIAI_X;
                         event.axisabs = axisabs_x + x * i;

                         divine->SendEvent( divine, &event );

                         current_x = event.axisabs;

                         event.axis    = DIAI_Y;
                         event.axisabs = axisabs_y + y * i;

                         divine->SendEvent( divine, &event );

                         current_y = event.axisabs;

                         usleep( ms * 1000 );
                    }

                    axisabs_x = current_x;
                    axisabs_y = current_y;
                    break;

               default:
                    break;
          }
     }
}

static void pause_handler( FILE *stream )
{
     int ms = 0;

     fscanf( stream, "%d", &ms );

     usleep( ms * 1000 );
}

static void text_handler( FILE *stream )
{
     int  c;
     int  ms      = 0;
     bool leading = true;

     fscanf( stream, "%d", &ms );

     while (!feof( stream )) {
          c = fgetc( stream );

          if (c == ' ' && leading)
               continue;

          if (c == '\n')
               break;

          divine->SendSymbol( divine, c );

          leading = false;

          usleep( ms * 1000 );
     }
}

static void text_special_handler( FILE *stream )
{
     int  i;
     char name[31];
     int  ms = 0;

     fscanf( stream, "%d", &ms );

     while (!feof( stream )) {
          switch (fgetc( stream )) {
               case '\n':
                    return;

               case 'b':
                    divine->SendSymbol( divine, DIKS_BACKSPACE );
                    break;

               case 'd':
                    divine->SendSymbol( divine, DIKS_DELETE );
                    break;

               case 'r':
                    divine->SendSymbol( divine, DIKS_RETURN );
                    break;

               case 'C':
                    switch (fgetc( stream )) {
                         case 'l':
                              divine->SendSymbol( divine, DIKS_CURSOR_LEFT );
                              break;

                         case 'r':
                              divine->SendSymbol( divine, DIKS_CURSOR_RIGHT );
                              break;

                         case 'u':
                              divine->SendSymbol( divine, DIKS_CURSOR_UP );
                              break;

                         case 'd':
                              divine->SendSymbol( divine, DIKS_CURSOR_DOWN );
                              break;
                    }
                    break;

               case 'S':
                    fscanf( stream, "%30s", name );
                    for (i = 0; symnames[i].symbol != DIKS_NULL; i++) {
                         if (!strcasecmp( name, symnames[i].name )) {
                              divine->SendSymbol( divine, symnames[i].symbol );
                              break;
                         }
                    }
                    break;

               default:
                    break;
          }

          usleep( ms * 1000 );
     }
}

static void loop_handler( FILE *stream )
{
     loop_position = ftell( stream );
}

static void comment_handler( FILE *stream )
{
     while (!feof( stream )) {
          switch (fgetc( stream )) {
               case '\n':
                    return;

               default:
                    break;
          }
     }
}

typedef void (*Handler)( FILE *stream );

static Handler handlers[] = {
     ['b'] = &button_handler,
     ['m'] = &motion_handler,
     ['M'] = &motion_special_handler,
     ['p'] = &pause_handler,
     ['t'] = &text_handler,
     ['T'] = &text_special_handler,
     ['l'] = &loop_handler,
     ['#'] = &comment_handler
};

/**********************************************************************************************************************/

int main( int argc, char *argv[] )
{
     DFBResult ret;
     int       c;

     /* Initialize DiVine including command line parsing. */
     ret = DiVineInit( &argc, &argv );
     if (ret) {
          DirectFBError( "DiVineInit() failed", ret );
          return -1;
     }

     /* Parse the command line. */
     if (!parse_command_line( argc, argv ))
          return -2;

     /* Open the connection to the DiVine input driver. */
     ret = DiVineCreate( &divine );
     if (ret) {
          DirectFBError( "DiVineCreate() failed", ret );
          return -3;
     }

     /* Compute sine table. */
     for (c = 0; c < 360 * 16; c++)
          sin_table[c] = sin( c / 16.0 * M_PI / 180.0 ) * 65536;

     /* Main loop. */
     do {
          while (!feof( stream ) && (c = fgetc( stream )) != '$') {
               if (c < sizeof(handlers) / sizeof(handlers[0]) && handlers[c])
                    handlers[c]( stream );
          }

          if (loop_position)
               fseek( stream, loop_position, SEEK_SET );
     } while (loop_position);

     /* Close the connection. */
     divine->Release( divine );

     if (stream != stdin)
          fclose( stream );

     return 0;
}
