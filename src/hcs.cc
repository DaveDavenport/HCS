/**
 *    This file is part of HCS.
 *    Written by Qball Cow <qball@gmpclient.org> 2013-2014
 *
 *    HCS is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License.
 *
 *    HCS is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with HCS.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>
#include <string>
#include <errno.h>
#include <readline/readline.h>

#define BAUDRATE         B9600
#define MODEMDEVICE      "/dev/ttyUSB0"

#define _POSIX_SOURCE    1

#define FALSE            0
#define TRUE             1

class PSUError : public std::exception
{
public:
PSUError( const char* errMessage ) : errMessage_ ( errMessage )
{
}
PSUError( const std::string errMessage ) : errMessage_ ( errMessage.c_str () )
{
}

// overriden what() method from exception class
const char* what () const throw( )
{
    return errMessage_;
}

private:
const char* errMessage_;
};

/**
 * Base class a PSU implementation should inherit from.
 */
class PSU
{
private:
struct termios oldtio;
protected:
int            fd = -1;
// Baudrate, set when needed.
int            baudrate = B9600;

PSU( int baudrate ) : baudrate ( baudrate )
{
}

virtual void init ()         = 0;
virtual void uninitialize () = 0;

public:
virtual ~PSU()
{
    if ( fd >= 0 )
    {
        close_device ();
    }
}
/**
 * Open a connection to the device.
 * Based on hostname, or hard coded default.
 */
virtual void open_device ()
{
    if ( getenv ( "HCS_DEVICE" ) == nullptr )
    {
        open_device ( MODEMDEVICE );
    }
    else
    {
        const char *path = getenv ( "HCS_DEVICE" );
        open_device ( path );
    }
}

/**
 * @param dev_node The device node to open.
 *
 * Open a connection to the device.
 */
virtual void open_device ( const char *dev_node ) throw ( PSUError & )
{
    fd = open ( dev_node, O_RDWR | O_NOCTTY );
    if ( fd < 0 )
    {
        printf ( "failed: %s\n", strerror ( errno ) );
        // TODO: THROW error
        std::string name = std::string ( "Failed to open \"" ) + dev_node + "\": '" + strerror ( errno ) + "'";
        throw PSUError ( name );
    }

    // save status port settings.
    tcgetattr ( fd, &oldtio );

    // Setup the serial port.
    struct termios newtio = { 0, };
    newtio.c_cflag     = baudrate | CS8 | CREAD | PARODD;
    newtio.c_iflag     = 0;
    newtio.c_oflag     = 0;
    newtio.c_lflag     = 0;       //ICANON;
    newtio.c_cc[VMIN]  = 1;
    newtio.c_cc[VTIME] = 0;
    tcflush ( fd, TCIFLUSH | TCIOFLUSH );
    tcsetattr ( fd, TCSANOW, &newtio );

    init ();
}
/**
 * Close connection to the device.
 */
virtual void close_device ()
{
    if ( fd < 0 )
    {
        // throw error.
        throw PSUError ( "Close device: Device already closed" );
    }
    // close connection
    tcflush ( fd, TCIFLUSH );
    tcsetattr ( fd, TCSANOW, &oldtio );
    close ( fd );
}

/**
 * Get the set output voltage.
 */
virtual float get_voltage () throw( PSUError & ) = 0;
/**
 * Get the set current limit.
 */
virtual float get_current () throw( PSUError & ) = 0;

/**
 * @param value the output voltage (in Volts)
 * Set the output voltage.
 */
virtual void set_voltage ( const float value ) throw( PSUError & ) = 0;

/**
 * @param value the output current (in Amps)
 * Set the output current.
 */
virtual void set_current ( const float value ) throw( PSUError & ) = 0;

/**
 * Get the actual output voltage.
 *
 * @returns the output voltage in Volts.
 */
virtual float get_voltage_actual () throw( PSUError & ) = 0;

/**
 * Get the actual output current.
 *
 * @returns the output current in Amps.
 */
virtual float get_current_actual () throw( PSUError & ) = 0;

/**
 * Enable output.
 *
 * Enable the output of the power supply.
 */
virtual void state_enable () throw( PSUError & ) = 0;

/**
 * Disable output.
 *
 * Disable the output of the power supply.
 */
virtual void state_disable () throw( PSUError & ) = 0;

/**
 * Get output state
 *
 * Get the output state of the power supply.
 *
 * @returns true when the output is enable, false otherwise.
 */
virtual bool get_state () throw( PSUError & ) = 0;

/**
 * Print device information.
 *
 * Print out information about the device.
 */
virtual void print_device_info () throw( PSUError & ) = 0;
};

class EAPS2K : public PSU
{
private:
const int baudrate = B115200;
enum ErrorTypes
{
    NO_ERROR              = 0x0,
    CRC_INVALID           = 0x3,
    DELIMITER_INVALID     = 0x4,
    OUTPUT_ADDR_INVALID   = 0x5,
    OBJECT_INVALID        = 0x7,
    OBJECT_LENGTH_INVALID = 0x8,
    ACCESS_VIOLATION      = 0x9,
    DEVICE_LOCKED         = 0x15,
    OBJECT_OVERFLOW       = 0x30,
    OBJECT_UNDERFLOW      = 0x31
};
struct
{
    ErrorTypes type;
    const char *name;
} ErrorTypeStr[10] =
{
    { NO_ERROR,              "No error"                                   },
    { CRC_INVALID,           "Check sum incorrect"                        },
    { DELIMITER_INVALID,     "Start delimiter incorrect"                  },
    { OUTPUT_ADDR_INVALID,   "Wrong address for output"                   },
    { OBJECT_INVALID,        "Object not defined"                         },
    { OBJECT_LENGTH_INVALID, "Object length incorrect"                    },
    { ACCESS_VIOLATION,      "Read/Write permissions violated, no access" },
    { DEVICE_LOCKED,         "Device is in \"Lock\" state"                },
    { OBJECT_OVERFLOW,       "Upper limit of object exceeded"             },
};
// See object table manual.
enum ObjectTypes
{
    DEVICE_TYPE          = 0,
    DEVICE_SERIAL_NO     = 1,
    NOMINAL_VOLTAGE      = 2,
    NOMINAL_CURRENT      = 3,
    NOMINAL_POWER        = 4,
    DEVICE_ARTICLE_NO    = 6,
    MANUFACTURER         = 8,
    SOFTWARE_VERSION     = 9,
    DEVICE_CLASS         = 19,
    OVP_THRESHOLD        = 38,
    OCP_THRESHOLD        = 39,
    SET_VOLTAGE          = 50,
    SET_CURRENT          = 51,
    POWER_SUPPLY_CONTROL = 54,
    STATUS_ACTUAL        = 71,
    STATUS_SET           = 72,
};
// These values are needed to convert the result.
float nominal_voltage = 1;
float nominal_current = 1;
float nominal_power   = 1;

/** Telegram functions */

// Max length is SD (1) + DN (1) + OBJ (2) + CS(2) + DATA (0-16)
uint8_t _telegram[22]  = { 0, };
uint8_t _telegram_size = 0;

void telegram_validate ()
{
    if ( _telegram[0] == 0 )
    {
        // Throw error.
    }
}
enum SendType
{
    SEND   =0xC0,
    RECEIVE=0x40
};
const int cast_type = 0x20;
const int direction = 0x10;
void telegram_start ( SendType dir, int size )
{
    // SD
    _telegram[0] = cast_type + dir + direction + ( ( size - 1 ) & 0x0F );
    // DN
    _telegram[1]   = 0x00;
    _telegram_size = 2;
}
void telegram_set_object ( ObjectTypes object )
{
    _telegram[_telegram_size] = object;
    _telegram_size           += 1;
}
void telegram_push ( uint8_t val )
{
    _telegram[_telegram_size] = val;
    _telegram_size           += 1;
}
const char *telegram_get_error ( ErrorTypes type ) const
{
    for ( int i = 0; i < 10; i++ )
    {
        if ( ErrorTypeStr[i].type == type )
        {
            return ErrorTypeStr[i].name;
        }
    }
    return ErrorTypeStr[0].name;
}
void telegram_send ()
{
    if ( _telegram[0] == 0 )
    {
        // Throw error.
    }
    telegram_crc_set ();
    write ( fd, _telegram, _telegram_size );
    syncfs ( fd );
    // check send error.

    // clear telegram.
    _telegram[0] = 0;
    usleep ( 50000 );
    // Receive answer
    telegram_receive ();
    // Check error
    if ( _telegram[2] == 0xFF && _telegram[3] != 0 )
    {
        ErrorTypes  type = (ErrorTypes) _telegram[3];
        std::string name = std::string ( "PSU reported error: " );
        name += telegram_get_error ( type );
        throw PSUError ( name );
    }
    // Check error
    usleep ( 50000 );
}
void telegram_receive ()
{
    if ( _telegram[0] != 0 )
    {
        // Throw error.
    }
    // Read header first.
    size_t ss = 3;
    do
    {
        ss -= read ( fd, &_telegram[3 - ss], ss );
    } while ( ss > 0 );

    // Calculate remainder of size.
    _telegram_size = 3 + ( ( _telegram[0] ) & 0x0F ) + 1 + 2;
    ss             = _telegram_size - 3;
    do
    {
        ss -= read ( fd, &_telegram[_telegram_size - ss], ss );
    } while ( ss > 0 );

    if ( !telegram_crc_check () )
    {
        throw PSUError ( "Message Invalid, CRC failure" );
    }
}

/**
 * Set the crc of the telegram.
 */
void telegram_crc_set ()
{
    int16_t crc = crc16 ( _telegram, _telegram_size );
    _telegram[_telegram_size++] = ( crc >> 8 ) & 0xFF;
    _telegram[_telegram_size++] = ( crc ) & 0xFF;
}

static int crc16 ( uint8_t *ba, int size )
{
    int work = 0;
    for ( int i = 0; i < size; i++ )
    {
        work += ba[i];
    }
    return work & 0xFFFF;
}

/**
 * Check the crc of the telegram.
 */
bool telegram_crc_check ()
{
    int16_t crc = crc16 ( _telegram, _telegram_size - 2 );
    if ( ( _telegram[_telegram_size - 2] == ( ( crc >> 8 ) & 0xFF ) ) &&
         ( _telegram[_telegram_size - 1] == ( ( crc ) & 0xFF ) ) )
    {
        return true;
    }
    return false;
}
static inline float  to_float ( uint8_t val[4] )
{
    union
    {
        float    fv;
        uint32_t value;
        uint8_t  array[4];
    } b;
    b.array[0] = val[0];
    b.array[1] = val[1];
    b.array[2] = val[2];
    b.array[3] = val[3];
    b.value    = be32toh ( b.value );
    return b.fv;
}
static inline uint32_t to_uint16 ( uint8_t val[2] )
{
    union
    {
        uint32_t value;
        uint8_t  array[2];
    } b;
    b.array[0] = val[0];
    b.array[1] = val[1];
    b.value    = be16toh ( b.value );
    return b.value;
}

void init ()
{
    // Retrieve nominal voltage
    telegram_start ( RECEIVE, 4 );
    telegram_set_object ( NOMINAL_VOLTAGE );
    telegram_send ();
    this->nominal_voltage = to_float ( &_telegram[3] );
    // Retrieve nominal current
    telegram_start ( RECEIVE, 4 );
    telegram_set_object ( NOMINAL_CURRENT );
    telegram_send ();
    this->nominal_current = to_float ( &_telegram[3] );
    // Retrieve nominal power
    telegram_start ( RECEIVE, 4 );
    telegram_set_object ( NOMINAL_POWER );
    telegram_send ();
    this->nominal_power = to_float ( &_telegram[3] );

    // Take control over the PSU.
    // TODO: do it when only needed.
    this->enable_remote ();
}

void uninitialize ()
{
    // Release control over the PSU.
    this->disable_remote ();
}

public:
void enable_remote () throw( PSUError & )
{
    telegram_start ( SEND, 2 );
    telegram_set_object ( POWER_SUPPLY_CONTROL );
    telegram_push ( 0x10 );
    telegram_push ( 0x10 );
    telegram_send ();
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_ACTUAL );
    telegram_send ();
    // Check state
}
void disable_remote () throw( PSUError & )
{
    telegram_start ( SEND, 2 );
    telegram_set_object ( POWER_SUPPLY_CONTROL );
    telegram_push ( 0x10 );
    telegram_push ( 0x00 );
    telegram_send ();
}
void state_enable () throw( PSUError & )
{
    telegram_start ( SEND, 2 );
    telegram_set_object ( POWER_SUPPLY_CONTROL );
    telegram_push ( 0x01 );
    telegram_push ( 0x01 );
    telegram_send ();
}
void state_disable () throw( PSUError & )
{
    telegram_start ( SEND, 2 );
    telegram_set_object ( POWER_SUPPLY_CONTROL );
    telegram_push ( 0x01 );
    telegram_push ( 0x00 );
    telegram_send ();
}

bool get_state () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_ACTUAL );
    telegram_send ();
    return ( ( _telegram[4] & 1 ) == 1 );
}

float get_current () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_SET );
    telegram_send ();
    float current = to_uint16 ( &_telegram[7] );
    return ( nominal_current * current ) / 256.0e2;
}
float get_voltage () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_SET );
    telegram_send ();
    float voltage = to_uint16 ( &_telegram[5] );
    return ( nominal_voltage * voltage ) / 256.0e2;
}
float get_current_actual () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_ACTUAL );
    telegram_send ();
    float current = to_uint16 ( &_telegram[7] );
    return ( nominal_current * current ) / 256.0e2;
}
float get_voltage_actual () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_ACTUAL );
    telegram_send ();
    float voltage = to_uint16 ( &_telegram[5] );
    return ( nominal_voltage * voltage ) / 256.0e2;
}

int get_operating_mode () throw( PSUError &)
{
    telegram_start ( RECEIVE, 6);
    telegram_set_object ( STATUS_ACTUAL );
    telegram_send ();
    // bits 2+1: 10->CC, 00->CV
    return ( _telegram[4] & 6);
}
void set_voltage ( float value ) throw( PSUError & )
{
    uint32_t val = ( value * 25600 ) / nominal_voltage;
    telegram_start ( SEND, 2 );
    telegram_set_object ( SET_VOLTAGE );
    telegram_push ( ( val >> 8 ) & 0xFF );
    telegram_push ( val & 0xFF );
    telegram_send ();
}
void set_current ( float value ) throw( PSUError & )
{
    uint32_t val = ( value * 25600 ) / nominal_current;
    telegram_start ( SEND, 2 );
    telegram_set_object ( SET_CURRENT );
    telegram_push ( ( val >> 8 ) & 0xFF );
    telegram_push ( val & 0xFF );
    telegram_send ();
}


void print_device_info () throw( PSUError & )
{
    printf ( "---------------------------------------\n" );
    printf ( "\nDevice information:\n" );
    // Get device type.
    telegram_start ( RECEIVE, 16 );
    telegram_set_object ( DEVICE_TYPE );
    telegram_send ();
    printf ( " Device Type:      %20s\n", &_telegram[3] );

    telegram_start ( RECEIVE, 16 );
    telegram_set_object ( MANUFACTURER );
    telegram_send ();
    printf ( " Manufacturer:     %20s\n", &_telegram[3] );

    telegram_start ( RECEIVE, 16 );
    telegram_set_object ( DEVICE_ARTICLE_NO );
    telegram_send ();
    printf ( " Article No. :     %20s\n", &_telegram[3] );

    telegram_start ( RECEIVE, 16 );
    telegram_set_object ( DEVICE_SERIAL_NO );
    telegram_send ();
    printf ( " Serial Num.:      %20s\n", &_telegram[3] );

    telegram_start ( RECEIVE, 16 );
    telegram_set_object ( SOFTWARE_VERSION );
    telegram_send ();
    printf ( " Software Version: %20s\n", &_telegram[3] );

    printf ( "\nDevice specifications:\n" );
    printf ( " Nominal voltage:  %20.02f\n", nominal_voltage );
    printf ( " Nominal current:  %20.02f\n", nominal_current );
    printf ( " Nominal power:    %20.02f\n", nominal_power );

    printf ( " Set voltage:      %20.02f\n", this->get_voltage () );
    printf ( " Set current:      %20.02f\n", this->get_current () );
    printf ( " Current voltage:  %20.02f\n", this->get_voltage_actual () );
    printf ( " Current current:  %20.02f\n", this->get_current_actual () );
    printf ( " Current mode:     %20s\n",    (this->get_operating_mode () == 4) ? "CC" : "CV" );
}


EAPS2K() : PSU ( B115200 )
{
}

~EAPS2K()
{
    if ( this->fd >= 0 )
    {
        this->disable_remote ();
    }
}
};

class PPS11360 : public PSU
{
public:
PPS11360() : PSU ( B9600 )
{
}

bool get_state () throw ( PSUError & )
{
    return false;
}

void state_enable ( void ) throw ( PSUError & )
{
    char buffer[128];
    this->send_cmd ( "SOUT", "0" );
    this->read_cmd ( buffer, 128 );
}
void state_disable ( void ) throw ( PSUError & )
{
    char buffer[128];
    this->send_cmd ( "SOUT", "1" );
    this->read_cmd ( buffer, 128 );
}
float get_voltage_actual () throw( PSUError & )
{
    char buffer[1024];
    // Send getd mesg.
    this->send_cmd ( "GETD", NULL );
    if ( this->read_cmd ( buffer, 1024 ) > 0 )
    {
        std::string b       = buffer;
        float       voltage = strtol ( b.substr ( 0, 3 ).c_str (), 0, 10 ) / 10.0f;
        return voltage;
    }
    throw PSUError ( "Invalid reply" );
}
float get_current_actual () throw( PSUError & )
{
    char buffer[1024];
    // Send getd mesg.
    this->send_cmd ( "GETD", NULL );
    if ( this->read_cmd ( buffer, 1024 ) > 0 )
    {
        std::string b       = buffer;
        float       current = strtol ( b.substr ( 4, 7 ).c_str (), 0, 10 ) / 1000.0f;
        return current;
    }
    throw PSUError ( "Invalid reply" );
}
void print_device_info ( void ) throw ( PSUError & )
{
    char buffer[1024];
    // Send getd mesg.
    this->send_cmd ( "GETD", NULL );

    // Get
    if ( this->read_cmd ( buffer, 1024 ) > 0 )
    {
        std::string b       = buffer;
        float       voltage = strtol ( b.substr ( 0, 3 ).c_str (), 0, 10 ) / 10.0;
        float       current = strtol ( b.substr ( 4, 7 ).c_str (), 0, 10 ) / 1000.0;
        int         limited = strtol ( b.substr ( 8, 8 ).c_str (), 0, 10 );
        float       set_voltage, set_current;
        // read
        this->get_voltage_current ( set_voltage, set_current );

        printf ( "Volt: %.2f V (Set %.2f V)\n", voltage, set_voltage );
        printf ( "Curr: %.2f A (Set %.2f A)\n", current, set_current );
        printf ( "V*C:  %.2f VA\n", voltage * current );
        printf ( "Lim:  %s\n", ( limited == 0 ) ? "Voltage" : "Current" );
    }
}
void set_voltage ( float value )  throw ( PSUError & )
{
    char buffer[1024];
    snprintf ( buffer, 1024, "%03d", ( int ) ( value * 10 ) );
    this->send_cmd ( "VOLT", buffer );
    this->read_cmd ( buffer, 1024 );
}
void set_current ( float value )  throw ( PSUError & )
{
    char buffer[1024];
    snprintf ( buffer, 1024, "%03d", ( int ) ( value * 100 ) );
    this->send_cmd ( "CURR", buffer );
    this->read_cmd ( buffer, 1024 );
}
float get_voltage () throw ( PSUError & )
{
    float voltage, current;
    get_voltage_current ( voltage, current );
    return voltage;
}
float get_current () throw ( PSUError & )
{
    float voltage, current;
    get_voltage_current ( voltage, current );
    return current;
}

private:
void init ()
{
}
void uninitialize ()
{
}
void get_voltage_current ( float &voltage, float &current )
{
    char buffer[1024];
    this->send_cmd ( "GETS", buffer );
    voltage = current = -1.0;

    if ( this->read_cmd ( buffer, 1024 ) > 5 )
    {
        std::string b = buffer;
        voltage = strtol ( b.substr ( 0, 3 ).c_str (), 0, 10 ) / 10.0;
        current = strtol ( b.substr ( 3, 6 ).c_str (), 0, 10 ) / 100.0;
    }
}


void send_cmd ( const char *command, const char *arg )
{
    // Check command.
    if ( command == nullptr )
    {
        return;
    }

    // Write command to str.
    write ( fd, command, strlen ( command ) );

    if ( arg != nullptr )
    {
        // Send argument.
        write ( fd, arg, strlen ( arg ) );
    }

    // End line
    write ( fd, "\r", 1 );
}

size_t read_cmd ( char *buffer, size_t max_length )
{
    size_t size = 0;

    while ( size < 3 ||
            !(
                buffer[size - 3] == 'O' &&
                buffer[size - 2] == 'K' &&
                buffer[size - 1] == '\n'
                )
            )
    {
        ssize_t v = read ( fd, &buffer[size], max_length - size );
        buffer[size + 1] = '\0';

        if ( buffer[size] == '\r' )
        {
            buffer[size] = '\n';
        }

        if ( v > 0 )
        {
            size += v;

            if ( size + 1 >= max_length )
            {
                return -1;
            }
        }
        else
        {
            printf ( "%i\n", errno );
            printf ( "%s\n", strerror ( errno ) );
            return -1;
        }
    }

    return size;
}
};
class HCS
{
private:
int            fd = 0;
struct termios oldtio;
PSU            *power_supply = nullptr;

public:
~HCS()
{
    if ( this->power_supply != nullptr )
    {
        delete this->power_supply;
        this->power_supply = nullptr;
    }
}

HCS()
{
}

/**
 * Small interactive GUI for controlling power supply
 */
int interactive ()
{
    while ( TRUE )
    {
        char *message = readline ( "> " );

        if ( message == NULL )
        {
            break;
        }

        if ( strcasecmp ( message, "q" ) == 0 ||
             strcasecmp ( message, "quit" ) == 0 )
        {
            printf ( "Quit\n" );
            free ( message );
            break;
        }

        if ( message[0] != '\0' )
        {
            //parse into arguments
            int  argc   = 0;
            char **argv = NULL;
            char *p;

            for ( p = strtok ( message, " " ); p != NULL; p = strtok ( NULL, " " ) )
            {
                argv           = ( char * * ) realloc ( argv, ( argc + 2 ) * sizeof ( *argv ) );
                argv[argc]     = p;
                argv[argc + 1] = NULL;
                argc++;
            }

            for ( int i = 0; i < argc; i++ )
            {
                int retv = this->parse_command ( argc - i, &argv[i] );
                if ( retv < 0 )
                {
                    return EXIT_FAILURE;
                }
                i += retv;
            }

            if ( argv )
            {
                free ( argv );
            }
        }

        free ( message );
    }

    return EXIT_SUCCESS;
}
int parse_command ( int argc, char **argv )
{
    int        index    = 0;
    const char *command = argv[0];
    try {
        if ( strncmp ( command, "pps", 3 ) == 0 )
        {
            if ( this->power_supply != nullptr )
            {
                delete this->power_supply;
            }
            this->power_supply = new PPS11360 ();
            this->power_supply->open_device ();
        }
        else if ( strncmp ( command, "eaps", 4 ) == 0 )
        {
            if ( this->power_supply != nullptr )
            {
                delete this->power_supply;
            }
            this->power_supply = new EAPS2K ();
            this->power_supply->open_device ();
        }
        else if ( this->power_supply != nullptr )
        {
            if ( strncmp ( command, "status", 6 ) == 0 )
            {
                this->power_supply->print_device_info ();
            }
            else if ( strncmp ( command, "on", 2 ) == 0 )
            {
                this->power_supply->state_enable ();
            }
            else if ( strncmp ( command, "off", 3 ) == 0 )
            {
                this->power_supply->state_disable ();
            }
            else if ( strncmp ( command, "voltage", 7 ) == 0 )
            {
                if ( argc > ( index + 1 ) )
                {
                    // write
                    const char *value = argv[++index];
                    float      volt   = strtof ( value, nullptr );
                    this->power_supply->set_voltage ( volt );
                }
                else
                {
                    float voltage;
                    // read
                    voltage = this->power_supply->get_voltage_actual ();
                    printf ( "%.2f\n", voltage );
                }
            }
            else if ( strncmp ( command, "current", 7 ) == 0 )
            {
                if ( argc > ( index + 1 ) )
                {
                    // write
                    const char *value  = argv[++index];
                    float      current = strtof ( value, nullptr );
                    this->power_supply->set_current ( current );
                }
                else
                {
                    float current;
                    // read
                    current = this->power_supply->get_current_actual ();
                    printf ( "%.2f\n", current );
                }
            }
        }
    }catch ( PSUError error ) {
        std::cerr << "Parse command failed: " << error.what () << std::endl;
        return -1;
    }

    return index;
}


int run ( int argc, char **argv )
{
    for ( int i = 0; i < argc; i++ )
    {
        const char *command = argv[i];

        if ( strncmp ( command, "interactive", 11 ) == 0 )
        {
            return this->interactive ();
        }
        else
        {
            int retv = this->parse_command ( argc - i, &argv[i] );
            if ( retv < 0 )
            {
                std::cerr << "Failed to parse command" << std::endl;
                return EXIT_FAILURE;
            }
            i += retv;
        }
    }

    return EXIT_FAILURE;
}
};

int main ( int argc, char **argv )
{
    HCS hcs;
    return hcs.run ( argc, argv );
}
