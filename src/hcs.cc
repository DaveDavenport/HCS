/**
 *    This file is part of HCS.
 *    Written by Qball Cow <qball@gmpclient.org> 2013-2015
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
#include <sstream>
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

#include <vector>
#include <config.h>


#ifdef HAVE_LIBUDEV_H
#include <libudev.h>
#endif

#include <hcs.h>
#include <hcs-ea.h>

void PSU::open_device ()
{
    if ( getenv ( "HCS_DEVICE" ) == nullptr ) {
        open_device ( MODEMDEVICE );
    }
    else{
        const char *path = getenv ( "HCS_DEVICE" );
        open_device ( path );
    }
}
void PSU::open_device ( const char *dev_node ) throw ( PSUError & )
{
    fd = open ( dev_node, O_RDWR | O_NOCTTY );
    if ( fd < 0 ) {
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
    newtio.c_lflag     = 0;                   //ICANON;
    newtio.c_cc[VMIN]  = 1;
    newtio.c_cc[VTIME] = 0;
    tcflush ( fd, TCIFLUSH | TCIOFLUSH );
    tcsetattr ( fd, TCSANOW, &newtio );

    init ();
}
void PSU::close_device ()
{
    if ( fd < 0 ) {
        // throw error.
        throw PSUError ( "Close device: Device already closed" );
    }
    // close connection
    tcflush ( fd, TCIFLUSH );
    tcsetattr ( fd, TCSANOW, &oldtio );
    close ( fd );
}

/**
 * Voltcraft Power supply
 */
class PPS11360 : public PSU
{
public:
    static bool check_supported_type ( const char *vendor_id, const char *product_id )
    {
        if ( strcmp ( vendor_id, "10c4" ) == 0 ) {
            if ( strcmp ( product_id, "ea60" ) == 0 ) {
                return true;
            }
        }
        return false;
    }

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
        if ( this->read_cmd ( buffer, 1024 ) > 0 ) {
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
        if ( this->read_cmd ( buffer, 1024 ) > 0 ) {
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
        if ( this->read_cmd ( buffer, 1024 ) > 0 ) {
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
        this->send_cmd ( "GETS", NULL );
        voltage = current = -1.0;

        if ( this->read_cmd ( buffer, 1024 ) > 5 ) {
            std::string b = buffer;
            voltage = strtol ( b.substr ( 0, 3 ).c_str (), 0, 10 ) / 10.0;
            current = strtol ( b.substr ( 3, 6 ).c_str (), 0, 10 ) / 100.0;
        }
    }


    void send_cmd ( const char *command, const char *arg )
    {
        // Check command.
        if ( command == nullptr ) {
            return;
        }

        // Write command to str.
        ssize_t result = write ( fd, command, strlen ( command ) );
        if ( result != strlen ( command ) ) {
            std::stringstream ss;
            ss << "Failed to send sufficient bytes: " << result << " out of " << strlen ( command );
            throw PSUError ( ss.str () );
        }

        if ( arg != nullptr ) {
            // Send argument.
            result = write ( fd, arg, strlen ( arg ) );
            if ( result != strlen ( arg ) ) {
                std::stringstream ss;
                ss << "Failed to send sufficient bytes: " << result << " out of " << strlen ( arg );
                throw PSUError ( ss.str () );
            }
        }

        // End line
        result = write ( fd, "\r", 1 );
        if ( result != 1 ) {
            std::stringstream ss;
            ss << "Failed to send sufficient bytes: " << result << " out of 1";
            throw PSUError ( ss.str () );
        }
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
                ) {
            ssize_t v = read ( fd, &buffer[size], max_length - size );
            buffer[size + 1] = '\0';

            if ( buffer[size] == '\r' ) {
                buffer[size] = '\n';
            }

            if ( v > 0 ) {
                size += v;

                if ( size + 1 >= max_length ) {
                    return -1;
                }
            }
            else{
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
        if ( power_supply != nullptr ) {
            delete power_supply;
            power_supply = nullptr;
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
        while ( TRUE ) {
            char *message = readline ( "> " );

            if ( message == NULL ) {
                break;
            }

            if ( strcasecmp ( message, "q" ) == 0 ||
                 strcasecmp ( message, "quit" ) == 0 ) {
                printf ( "Quit\n" );
                free ( message );
                break;
            }

            if ( message[0] != '\0' ) {
                //parse into arguments
                int  argc   = 0;
                char **argv = NULL;
                char *p;

                for ( p = strtok ( message, " " ); p != NULL; p = strtok ( NULL, " " ) ) {
                    argv           = ( char * * ) realloc ( argv, ( argc + 2 ) * sizeof ( *argv ) );
                    argv[argc]     = p;
                    argv[argc + 1] = NULL;
                    argc++;
                }

                for ( int i = 0; i < argc; i++ ) {
                    int retv = this->parse_command ( argc - i, &argv[i] );
                    if ( retv < 0 ) {
                        return EXIT_FAILURE;
                    }
                    i += retv;
                }

                if ( argv ) {
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
            if ( strncmp ( command, "auto", 4 ) == 0 ) {
                if ( power_supply != nullptr ) {
                    delete power_supply;
                    power_supply = nullptr;
                }
                // Get list of connected devices.
                detect_devices ();
                int dev_num = 0;
                // Autoconnect.
                if ( argc > ( index + 1 ) ) {
                    char *p;
                    int  val = strtol ( argv[1], &p, 10 );
                    if ( p != argv[1] ) {
                        dev_num = val;
                        index++;
                    }
                }
                if ( psu_list.size () > dev_num ) {
                    auto &psu = psu_list[dev_num];
                    if ( power_supply != nullptr ) {
                        delete power_supply;
                        power_supply = nullptr;
                    }
                    power_supply = psu.connect ();
                }
                else {
                    fprintf ( stderr, "No device available to open.\n" );
                }
            }
            else if ( strncmp ( command, "pps", 3 ) == 0 ) {
                if ( power_supply != nullptr ) {
                    delete power_supply;
                }
                power_supply = new PPS11360 ();
                power_supply->open_device ();
            }
            else if ( strncmp ( command, "eaps", 4 ) == 0 ) {
                if ( power_supply != nullptr ) {
                    delete power_supply;
                }
                power_supply = new EAPS2K ();
                power_supply->open_device ();
            }
            else if ( strncmp ( command, "list", 4 ) == 0 ) {
                // Find devices.
                detect_devices ();
                printf ( "Found %zd power suppl%s:\n", psu_list.size (), ( psu_list.size () == 1 ) ? "y" : "ies" );
                int index = 0;
                for ( auto psu : psu_list ) {
                    printf ( " [%2d] %s at '%s'\n",
                             index,
                             psu.type == PSU::PSUTypes::EAPS2K ? "Elektro-Automatik" : "Voltcraft",
                             psu.device_name );
                    index++;
                }
            }
            else if ( power_supply != nullptr ) {
                if ( strncmp ( command, "status", 6 ) == 0 ) {
                    power_supply->print_device_info ();
                }
                else if ( strncmp ( command, "on", 2 ) == 0 ) {
                    power_supply->state_enable ();
                }
                else if ( strncmp ( command, "off", 3 ) == 0 ) {
                    power_supply->state_disable ();
                }
                else if ( strncmp ( command, "ovp", 3 ) == 0 ) {
                    if ( argc > ( index + 1 ) ) {
                        // write
                        const char *value = argv[++index];
                        float      volt   = strtof ( value, nullptr );
                        power_supply->set_over_voltage ( volt );
                    }
                    else{
                        float voltage = power_supply->get_over_voltage ();
                        printf ( "%.2f\n", voltage );
                    }
                }
                else if ( strncmp ( command, "ocp", 3 ) == 0 ) {
                    if ( argc > ( index + 1 ) ) {
                        // write
                        const char *value = argv[++index];
                        float      curr   = strtof ( value, nullptr );
                        power_supply->set_over_current ( curr );
                    }
                    else{
                        float current = power_supply->get_over_current ();
                        printf ( "%.2f\n", current );
                    }
                }
                else if ( strncmp ( command, "mode", 4 ) == 0 ) {
                    printf ( "%s", power_supply->get_mode_str ( power_supply->get_operating_mode () ) );
                }
                else if ( strncmp ( command, "voltage", 7 ) == 0 ) {
                    if ( argc > ( index + 1 ) ) {
                        // write
                        const char *value = argv[++index];
                        float      volt   = strtof ( value, nullptr );
                        power_supply->set_voltage ( volt );
                    }
                    else{
                        float voltage;
                        // read
                        voltage = power_supply->get_voltage_actual ();
                        printf ( "%.2f\n", voltage );
                    }
                }
                else if ( strncmp ( command, "current", 7 ) == 0 ) {
                    if ( argc > ( index + 1 ) ) {
                        // write
                        const char *value  = argv[++index];
                        float      current = strtof ( value, nullptr );
                        power_supply->set_current ( current );
                    }
                    else{
                        float current;
                        // read
                        current = power_supply->get_current_actual ();
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
        for ( int i = 0; i < argc; i++ ) {
            const char *command = argv[i];

            if ( strncmp ( command, "interactive", 11 ) == 0 ) {
                return this->interactive ();
            }
            else{
                int retv = this->parse_command ( argc - i, &argv[i] );
                if ( retv < 0 ) {
                    std::cerr << "Failed to parse command" << std::endl;
                    return EXIT_FAILURE;
                }
                i += retv;
            }
        }

        return EXIT_SUCCESS;
    }
private:
    struct PSU_dev
    {
        PSU::PSUTypes type;
        char          device_name[256];

        PSU_dev ( PSU::PSUTypes type, const char *dn )
        {
            this->type = type;
            strncpy ( device_name, dn, 256 );
        }

        PSU *connect ()
        {
            PSU *power_supply = nullptr;
            switch ( type )
            {
            case PSU::PSUTypes::EAPS2K :
                power_supply = new EAPS2K ();
                break;
            case PSU::PSUTypes::PPS11360:
                power_supply = new PPS11360 ();
                break;
            }
            if ( power_supply != nullptr ) {
                power_supply->open_device ( device_name );
            }
            return power_supply;
        }
    };
    void detect_devices ()
    {
        psu_list.clear ();
#ifdef HAVE_LIBUDEV_H
        struct udev           *ud = udev_new ();

        struct udev_enumerate *enumerate = udev_enumerate_new ( ud );
        udev_enumerate_add_match_subsystem ( enumerate, "tty" );
        udev_enumerate_scan_devices ( enumerate );
        struct udev_list_entry *list = udev_enumerate_get_list_entry ( enumerate );
        struct udev_list_entry *entry;
        udev_list_entry_foreach ( entry, list )
        {
            const char         *path;
            struct udev_device *dev;


            // Have to grab the actual udev device here...
            path = udev_list_entry_get_name ( entry );
            dev  = udev_device_new_from_syspath ( ud, path );
            const char *vendor_id  = udev_device_get_property_value ( dev, "ID_VENDOR_ID" );
            const char *product_id = udev_device_get_property_value ( dev, "ID_MODEL_ID" );
            const char *dev_name   = udev_device_get_property_value ( dev, "DEVNAME" );
            if ( vendor_id == nullptr || product_id == nullptr || dev_name == nullptr ) {
                udev_device_unref ( dev );
                continue;
            }
            // Types
            if ( EAPS2K::check_supported_type ( vendor_id, product_id ) ) {
                psu_list.push_back ( PSU_dev ( PSU::PSUTypes::EAPS2K, dev_name ) );
            }
            else if ( PPS11360::check_supported_type ( vendor_id, product_id ) ) {
                psu_list.push_back ( PSU_dev ( PSU::PSUTypes::PPS11360, dev_name ) );
            }
            // Done with this device
            udev_device_unref ( dev );
        }
        // Done with the list and this udev
        udev_enumerate_unref ( enumerate );
        udev_unref ( ud );
#endif
    }
private:
    std::vector<struct PSU_dev> psu_list;
};

int main ( int argc, char **argv )
{
    HCS hcs;
    return hcs.run ( argc, argv );
}
