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
#include <hcs-ea.h>

#include <config.h>
/**
 * Helpers.
 */
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
/**
 * Interface API
 */
void EAPS2K::print_device_info () throw( PSUError & )
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

    printf ( " Set OVP:          %20.02f\n", this->get_over_voltage () );
    printf ( " Set OCP:          %20.02f\n", this->get_over_current () );
    printf ( " Set voltage:      %20.02f\n", this->get_voltage () );
    printf ( " Set current:      %20.02f\n", this->get_current () );
    auto volt = this->get_voltage_actual ();
    auto curr = this->get_current_actual ();
    printf ( " Current voltage:  %20.02f\n", volt );
    printf ( " Current current:  %20.02f\n", curr );
    printf ( " Current power:    %20.02f\n", volt * curr );
    printf ( " Current mode:     %20s\n", get_mode_str ( this->get_operating_mode () ) );
}
bool EAPS2K::check_supported_type ( const char *vendor_id, const char *product_id )
{
    if ( strcasecmp ( vendor_id, "232e" ) == 0 ) {
        if ( strcasecmp ( product_id, "0010" ) == 0 ) {
            return true;
        }
    }
    return false;
}
void EAPS2K::enable_remote () throw( PSUError & )
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
void EAPS2K::disable_remote () throw( PSUError & )
{
    telegram_start ( SEND, 2 );
    telegram_set_object ( POWER_SUPPLY_CONTROL );
    telegram_push ( 0x10 );
    telegram_push ( 0x00 );
    telegram_send ();
}
void EAPS2K::state_enable () throw( PSUError & )
{
    telegram_start ( SEND, 2 );
    telegram_set_object ( POWER_SUPPLY_CONTROL );
    telegram_push ( 0x01 );
    telegram_push ( 0x01 );
    telegram_send ();
}
void EAPS2K::state_disable () throw( PSUError & )
{
    telegram_start ( SEND, 2 );
    telegram_set_object ( POWER_SUPPLY_CONTROL );
    telegram_push ( 0x01 );
    telegram_push ( 0x00 );
    telegram_send ();
}

bool EAPS2K::get_state () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_ACTUAL );
    telegram_send ();
    return ( _telegram[4] & 1 ) == 1;
}

float EAPS2K::get_current () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_SET );
    telegram_send ();
    float current = to_uint16 ( &_telegram[7] );
    return ( nominal_current * current ) / 256.0e2;
}
float EAPS2K::get_voltage () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_SET );
    telegram_send ();
    float voltage = to_uint16 ( &_telegram[5] );
    return ( nominal_voltage * voltage ) / 256.0e2;
}
float EAPS2K::get_current_actual () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_ACTUAL );
    telegram_send ();
    float current = to_uint16 ( &_telegram[7] );
    return ( nominal_current * current ) / 256.0e2;
}
float EAPS2K::get_voltage_actual () throw( PSUError & )
{
    telegram_start ( RECEIVE, 2 );
    telegram_set_object ( STATUS_ACTUAL );
    telegram_send ();
    float voltage = to_uint16 ( &_telegram[5] );
    return ( nominal_voltage * voltage ) / 256.0e2;
}

float EAPS2K::get_over_voltage () throw ( PSUError & )
{
    telegram_start ( RECEIVE, 2 );
    telegram_set_object ( OVP_THRESHOLD );
    telegram_send ();
    float voltage = to_uint16 ( &_telegram[3] );
    return ( nominal_voltage * voltage ) / 256.0e2;
}
float EAPS2K::get_over_current () throw ( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( OCP_THRESHOLD );
    telegram_send ();
    float current = to_uint16 ( &_telegram[3] );
    return ( nominal_current * current ) / 256.0e2;
}

PSU::OperatingMode EAPS2K::get_operating_mode () throw( PSUError & )
{
    telegram_start ( RECEIVE, 6 );
    telegram_set_object ( STATUS_ACTUAL );
    telegram_send ();
    if ( ( _telegram[4] & 1 ) == 0 ) {
        return OperatingMode::OFF;
    }
    // bits 2+1: 10->CC, 00->CV
    return ( ( _telegram[4] & 6 ) >> 2 ) ? OperatingMode::CC : OperatingMode::CV;
}
void EAPS2K::set_voltage ( float value ) throw( PSUError & )
{
    uint32_t val = ( value * 25600 ) / nominal_voltage;
    telegram_start ( SEND, 2 );
    telegram_set_object ( SET_VOLTAGE );
    telegram_push ( ( val >> 8 ) & 0xFF );
    telegram_push ( val & 0xFF );
    telegram_send ();
}
void EAPS2K::set_current ( float value ) throw( PSUError & )
{
    uint32_t val = ( value * 25600 ) / nominal_current;
    telegram_start ( SEND, 2 );
    telegram_set_object ( SET_CURRENT );
    telegram_push ( ( val >> 8 ) & 0xFF );
    telegram_push ( val & 0xFF );
    telegram_send ();
}

void EAPS2K::set_over_current ( float value ) throw( PSUError & )
{
    uint32_t val = ( value * 25600 ) / nominal_current;
    telegram_start ( SEND, 2 );
    telegram_set_object ( OCP_THRESHOLD );
    telegram_push ( ( val >> 8 ) & 0xFF );
    telegram_push ( val & 0xFF );
    telegram_send ();
}


void EAPS2K::set_over_voltage ( float value ) throw( PSUError & )
{
    uint32_t val = ( value * 25600 ) / nominal_voltage;
    telegram_start ( SEND, 2 );
    telegram_set_object ( OVP_THRESHOLD );
    telegram_push ( ( val >> 8 ) & 0xFF );
    telegram_push ( val & 0xFF );
    telegram_send ();
}

/**
 * The telegram interface to communication with PSU
 */
static int crc16 ( uint8_t *ba, int size )
{
    int work = 0;
    for ( int i = 0; i < size; i++ ) {
        work += ba[i];
    }
    return work & 0xFFFF;
}

void EAPS2K::telegram_validate ()
{
    if ( _telegram[0] == 0 ) {
        // Throw error.
    }
}
void EAPS2K::telegram_start ( SendType dir, int size )
{
    // SD
    _telegram[0] = cast_type + dir + direction + ( ( size - 1 ) & 0x0F );
    // DN
    _telegram[1]   = 0x00;
    _telegram_size = 2;
}
void EAPS2K::telegram_set_object ( ObjectTypes object )
{
    _telegram[_telegram_size] = object;
    _telegram_size           += 1;
}
void EAPS2K::telegram_push ( uint8_t val )
{
    _telegram[_telegram_size] = val;
    _telegram_size           += 1;
}
const char *EAPS2K::telegram_get_error ( ErrorTypes type ) const
{
    for ( int i = 0; i < 10; i++ ) {
        if ( ErrorTypeStr[i].type == type ) {
            return ErrorTypeStr[i].name;
        }
    }
    return ErrorTypeStr[0].name;
}
void EAPS2K::telegram_send ()
{
    if ( _telegram[0] == 0 ) {
        // Throw error.
    }
    telegram_crc_set ();
    ssize_t result = write ( fd, _telegram, _telegram_size );
    if ( result != _telegram_size ) {
        std::stringstream ss;
        ss << "Failed to send sufficient bytes: " << result << " out of " << _telegram_size;
        throw PSUError ( ss.str () );
    }
    fsync ( fd );
    // check send error.

    // clear telegram.
    _telegram[0] = 0;
    usleep ( 50000 );
    // Receive answer
    telegram_receive ();
    // Check error
    if ( _telegram[2] == 0xFF && _telegram[3] != 0 ) {
        ErrorTypes  type = (ErrorTypes) _telegram[3];
        std::string name = std::string ( "PSU reported error: " );
        name += telegram_get_error ( type );
        throw PSUError ( name );
    }
    // Check error
    usleep ( 50000 );
}
void EAPS2K::telegram_receive ()
{
    if ( _telegram[0] != 0 ) {
        // Throw error.
    }
    // Read header first.
    size_t ss = 3;
    do {
        ss -= read ( fd, &_telegram[3 - ss], ss );
    } while ( ss > 0 );

    // Calculate remainder of size.
    _telegram_size = 3 + ( ( _telegram[0] ) & 0x0F ) + 1 + 2;
    ss             = _telegram_size - 3;
    do {
        ss -= read ( fd, &_telegram[_telegram_size - ss], ss );
    } while ( ss > 0 );

    if ( !telegram_crc_check () ) {
        throw PSUError ( "Message Invalid, CRC failure" );
    }
}
/**
 * Set the crc of the telegram.
 */
void EAPS2K::telegram_crc_set ()
{
    int16_t crc = crc16 ( _telegram, _telegram_size );
    _telegram[_telegram_size++] = ( crc >> 8 ) & 0xFF;
    _telegram[_telegram_size++] = ( crc ) & 0xFF;
}


/**
 * Check the crc of the telegram.
 */
bool EAPS2K::telegram_crc_check ()
{
    int16_t crc = crc16 ( _telegram, _telegram_size - 2 );
    if ( ( _telegram[_telegram_size - 2] == ( ( crc >> 8 ) & 0xFF ) ) &&
         ( _telegram[_telegram_size - 1] == ( ( crc ) & 0xFF ) ) ) {
        return true;
    }
    return false;
}

/**
 * Others
 */
void EAPS2K::init ()
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
void EAPS2K::uninitialize ()
{
    // Release control over the PSU.
    this->disable_remote ();
}
EAPS2K::EAPS2K() : PSU ( B115200 )
{
}

EAPS2K::~EAPS2K()
{
    if ( this->fd >= 0 ) {
        this->disable_remote ();
    }
}
