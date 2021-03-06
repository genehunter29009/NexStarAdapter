/******************************************************************
    Author:     Juan Menendez Blanco    <juanmb@gmail.com>

    This code is part of the NexStarAdapter project:
        https://github.com/juanmb/NexStarAdapter

********************************************************************/

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include "serial_command.h"
#include "nexstar_aux.h"


#define VERSION_MAJOR 4
#define VERSION_MINOR 2
#define MOUNT_MODEL 10  // GT
#define CONTROLLER_VARIANT 0x11  // NexStar
#define BAUDRATE 9600

#define AUX_SELECT 5
#define AUX_RX 6
#define AUX_TX 7
#define RA_POT_PIN A6
#define DEC_POT_PIN A7

// milliseconds to position factor
#define T_FACTOR (0xffffffff/8.616e7)

#define reverse(a) (0xffffffff - (a))


SerialCommand sCmd;
NexStarAux scope(AUX_RX, AUX_TX, AUX_SELECT);

uint8_t tracking_mode = 0;
uint32_t t_sync = 0;


// Obtain the difference in RA from the motor position due to Earth rotation
static uint32_t getRADiff() {
    return T_FACTOR*(millis() - t_sync);
}


void cmdGetEqCoords(char *cmd)
{
    uint32_t ra_pos, dec_pos;
    scope.getPosition(DEV_RA, &ra_pos);
    scope.getPosition(DEV_DEC, &dec_pos);
    ra_pos = reverse(ra_pos) + getRADiff();

    ra_pos = (ra_pos >> 16) & 0xffff;
    dec_pos = (dec_pos >> 16) & 0xffff;

    char tmp[11];
    sprintf(tmp, "%04lX,%04lX#", ra_pos, dec_pos);
    Serial.write(tmp);
}

void cmdGetEqPreciseCoords(char *cmd)
{
    uint32_t ra_pos, dec_pos;
    scope.getPosition(DEV_RA, &ra_pos);
    scope.getPosition(DEV_DEC, &dec_pos);
    ra_pos = reverse(ra_pos) + getRADiff();

    char tmp[19];
    sprintf(tmp, "%08lX,%08lX#", ra_pos, dec_pos);
    Serial.write(tmp);
}

void cmdGetAzCoords(char *cmd)
{
    //TODO
    cmdGetEqCoords(cmd);
}

void cmdGetAzPreciseCoords(char *cmd)
{
    //TODO
    cmdGetEqPreciseCoords(cmd);
}

// Return true if a given position is close to the current axis position
// (approx. 11 degrees)
bool isClose(uint32_t pos, uint8_t axis)
{
    uint32_t curr, diff;

    scope.getPosition(axis, &curr);
    diff = (pos > curr) ? (pos - curr) : (curr - pos);
    return diff < 0x08000000;
}

void cmdGotoEqCoords(char *cmd)
{
    uint32_t ra_pos, dec_pos;
    sscanf(cmd, "R%4lx,%4lx", &ra_pos, &dec_pos);
    ra_pos = reverse(ra_pos << 16) + getRADiff();
    dec_pos <<= 16;

    scope.gotoPosition(DEV_RA, isClose(ra_pos, DEV_RA), ra_pos);
    scope.gotoPosition(DEV_DEC, isClose(dec_pos, DEV_DEC), dec_pos);
    Serial.write('#');
}

void cmdGotoEqPreciseCoords(char *cmd)
{
    uint32_t ra_pos, dec_pos;
    sscanf(cmd, "r%8lx,%8lx", &ra_pos, &dec_pos);
    ra_pos = reverse(ra_pos) + getRADiff();

    scope.gotoPosition(DEV_RA, isClose(ra_pos, DEV_RA), ra_pos);
    scope.gotoPosition(DEV_DEC, isClose(dec_pos, DEV_DEC), dec_pos);
    Serial.write('#');
}

void cmdGotoAzCoords(char *cmd)
{
    //TODO
    cmdGotoEqCoords(cmd);
}

void cmdGotoAzPreciseCoords(char *cmd)
{
    //TODO
    cmdGotoEqPreciseCoords(cmd);
}

void cmdGotoInProgress(char *cmd)
{
    bool ra_done, dec_done;
    scope.slewDone(DEV_RA, &ra_done);
    scope.slewDone(DEV_DEC, &dec_done);
    Serial.write((ra_done && dec_done) ? '0' : '1');
    Serial.write('#');
}

void cmdCancelGoto(char *cmd)
{
    scope.move(DEV_RA, 0, 0);
    scope.move(DEV_DEC, 0, 0);
    Serial.write('#');
}

void cmdSyncEqCoords(char *cmd)
{
    uint32_t ra_pos, dec_pos;
    sscanf(cmd, "S%4lx,%4lx", &ra_pos, &dec_pos);
    scope.setPosition(DEV_RA, reverse(ra_pos << 16));
    scope.setPosition(DEV_DEC, dec_pos << 16);
    Serial.write('#');
    t_sync = millis();
}

void cmdSyncEqPreciseCoords(char *cmd)
{
    uint32_t ra_pos, dec_pos;
    sscanf(cmd, "s%8lx,%8lx", &ra_pos, &dec_pos);
    scope.setPosition(DEV_RA, reverse(ra_pos));
    scope.setPosition(DEV_DEC, dec_pos);
    Serial.write('#');
    t_sync = millis();
}

void cmdGetTrackingMode(char *cmd)
{
    Serial.write(tracking_mode);
    Serial.write('#');
}

void cmdSetTrackingMode(char *cmd)
{
    tracking_mode = cmd[1];
    scope.setGuiderate(DEV_RA, GUIDERATE_POS, true, 0);    // stop RA motor
    scope.setGuiderate(DEV_DEC, GUIDERATE_POS, true, 0);   // stop DEC motor

    switch(tracking_mode) {
        case TRACKING_EQ_NORTH:
            scope.setGuiderate(DEV_RA, GUIDERATE_POS, 0, GUIDERATE_SIDEREAL);
            break;
        case TRACKING_EQ_SOUTH:
            scope.setGuiderate(DEV_RA, GUIDERATE_NEG, 0, GUIDERATE_SIDEREAL);
            break;
    }

    Serial.write('#');
}

void cmdGetLocation(char *cmd)
{
    // Read the location from EEPROM
    for (int i = 0; i < 8; i++) {
        Serial.write(EEPROM.read(i));
    }
    Serial.write('#');
}

void cmdSetLocation(char *cmd)
{
    // Store the location in EEPROM
    for (int i = 0; i < 8; i++) {
        EEPROM.write(i, cmd[i + 1]);
    }
    Serial.write('#');
}

void cmdGetTime(char *cmd)
{
    //TODO
    Serial.write(17);   // hours
    Serial.write(30);   // minutes
    Serial.write(10);   // seconds
    Serial.write(4);    // month
    Serial.write(1);    // day
    Serial.write(15);   // year
    Serial.write(3);    // offset from GMT
    Serial.write(0);    // Daylight saving
    Serial.write("#");
}

void cmdSetTime(char *cmd)
{
    //TODO
    Serial.write('#');
}

void cmdPassThrough(char *cmd)
{
    NexStarMessage resp;
    uint8_t size = cmd[1] - 1;

    // pass the command to the mount
    int ret = scope.sendMessage(cmd[2], cmd[3], size, &cmd[4], &resp);
    if (ret != 0) {
        // TODO: return a response with size = normal_return_size + 1
        Serial.print(ret);
        Serial.write('#');
        return;
    }

    for (int i = 0; i < resp.header.length - 3; i++) {
        Serial.write(resp.payload[i]);
    }
    Serial.write('#');
}

void cmdGetVersion(char *cmd)
{
    Serial.write(VERSION_MAJOR);
    Serial.write(VERSION_MINOR);
    Serial.write('#');
}

void cmdGetVariant(char *cmd)
{
    Serial.write(CONTROLLER_VARIANT);
    Serial.write('#');
}

void cmdGetModel(char *cmd)
{
    Serial.write(MOUNT_MODEL);
    Serial.write('#');
}

void cmdEcho(char *cmd)
{
    Serial.write(cmd[1]);
    Serial.write('#');
}

// Read the absolute position of the two axes using two potentiometers.
// The returned position is the raw 10-bits value reading from the ADC.
void cmdGetAbsPosition(char *cmd)
{
    int axis1_pos = analogRead(RA_POT_PIN);
    int axis2_pos = analogRead(DEC_POT_PIN);

    char tmp[11];
    sprintf(tmp, "%04X,%04X#", axis1_pos, axis2_pos);
    Serial.write(tmp);
}

void setup()
{
    // Map serial commands to functions
    sCmd.addCommand('E', 1, cmdGetEqCoords);
    sCmd.addCommand('e', 1, cmdGetEqPreciseCoords);
    sCmd.addCommand('Z', 1, cmdGetAzCoords);
    sCmd.addCommand('z', 1, cmdGetAzPreciseCoords);

    sCmd.addCommand('R', 10, cmdGotoEqCoords);
    sCmd.addCommand('r', 18, cmdGotoEqPreciseCoords);
    sCmd.addCommand('B', 10, cmdGotoAzCoords);
    sCmd.addCommand('b', 18, cmdGotoAzPreciseCoords);
    sCmd.addCommand('L', 1, cmdGotoInProgress);
    sCmd.addCommand('M', 1, cmdCancelGoto);

    sCmd.addCommand('S', 10, cmdSyncEqCoords);
    sCmd.addCommand('s', 18, cmdSyncEqPreciseCoords);

    sCmd.addCommand('T', 2, cmdSetTrackingMode);
    sCmd.addCommand('t', 1, cmdGetTrackingMode);

    sCmd.addCommand('W', 9, cmdSetLocation);
    sCmd.addCommand('w', 1, cmdGetLocation);
    sCmd.addCommand('H', 9, cmdSetTime);
    sCmd.addCommand('h', 1, cmdGetTime);

    sCmd.addCommand('P', 8, cmdPassThrough);
    sCmd.addCommand('V', 1, cmdGetVersion);
    sCmd.addCommand('v', 1, cmdGetVariant);
    sCmd.addCommand('m', 1, cmdGetModel);
    sCmd.addCommand('K', 2, cmdEcho);

    // custom command
    sCmd.addCommand('U', 1, cmdGetAbsPosition);

    //sCmd.addCommand('J', 1, cmdAlignmentComplete);
    //sCmd.addCommand('x', 1, cmdHibernate);
    //sCmd.addCommand('y', 1, cmdWakeup);

    pinMode(AUX_SELECT, OUTPUT);

    Serial.begin(BAUDRATE);
    scope.begin();
}

void loop()
{
    sCmd.readSerial();
}
