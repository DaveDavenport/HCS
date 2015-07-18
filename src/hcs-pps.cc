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
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <hcs.h>
#include <hcs-pps.h>

#include <config.h>

bool PPS11360::check_supported_type ( const char *vendor_id, const char *product_id )
{
    if ( strcmp ( vendor_id, "10c4" ) == 0 ) {
        if ( strcmp ( product_id, "ea60" ) == 0 ) {
            return true;
        }
    }
    return false;
}

PPS11360::PPS11360() : PSU ( B9600 )
{
}
/**
 * Private functions
 */
size_t PPS11360::read_cmd ( char *buffer, size_t max_length )
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
}

bool PPS11360::get_state () throw ( PSUError & )
{
    return false;
}

void PPS11360::state_enable ( void ) throw ( PSUError & )
{
    char buffer[128];
    this->send_cmd ( "SOUT", "0" );
    this->read_cmd ( buffer, 128 );
}
void PPS11360::state_disable ( void ) throw ( PSUError & )
{
    char buffer[128];
    this->send_cmd ( "SOUT", "1" );
    this->read_cmd ( buffer, 128 );
}
float PPS11360::get_voltage_actual () throw( PSUError & )
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
float PPS11360::get_current_actual () throw( PSUError & )
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

float PPS11360::get_over_voltage() throw ( PSUError & )
{
    return -1.0;
}
float PPS11360::get_over_current() throw ( PSUError & )
{
    return -1.0;
}

void PPS11360::print_device_info ( void ) throw ( PSUError & )
{
    printf ( "\nDevice specifications:\n" );
    PSU::print_device_info ();
}
PSU::OperatingMode PPS11360::get_operating_mode () throw ( PSUError & )
{    char buffer[1024];
    // Send getd mesg.
    this->send_cmd ( "GETD", NULL );
    if ( this->read_cmd ( buffer, 1024 ) > 0 ) {
        std::string b       = buffer;
        int         limited = strtol ( b.substr ( 8, 8 ).c_str (), 0, 10 );
        return (limited == 0) ? PSU::OperatingMode::CV: PSU::OperatingMode::CC;
    }
    return OperatingMode::CV;
}

void PPS11360::set_voltage ( float value )  throw ( PSUError & )
{
    char buffer[1024];
    snprintf ( buffer, 1024, "%03d", ( int ) ( value * 10 ) );
    this->send_cmd ( "VOLT", buffer );
    this->read_cmd ( buffer, 1024 );
}
void PPS11360::set_current ( float value )  throw ( PSUError & )
{
    char buffer[1024];
    snprintf ( buffer, 1024, "%03d", ( int ) ( value * 100 ) );
    this->send_cmd ( "CURR", buffer );
    this->read_cmd ( buffer, 1024 );
}
float PPS11360::get_voltage () throw ( PSUError & )
{
    float voltage, current;
    get_voltage_current ( voltage, current );
    return voltage;
}
float PPS11360::get_current () throw ( PSUError & )
{
    float voltage, current;
    get_voltage_current ( voltage, current );
    return current;
}

void PPS11360::init ()
{
}
void PPS11360::uninitialize ()
{
}
void PPS11360::get_voltage_current ( float &voltage, float &current )
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


void PPS11360::send_cmd ( const char *command, const char *arg )
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
