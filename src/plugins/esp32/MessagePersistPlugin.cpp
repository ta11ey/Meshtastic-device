#include "MessagePersistPlugin.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "PowerFSM.h"
#include "RTC.h"
#include "Router.h"
#include "configuration.h"
#include <Arduino.h>
#include <SPIFFS.h>
//#include <assert.h>

/*
    As a sender, I can send packets every n-seonds. These packets include an incramented PacketID.

    As a receiver, I can receive packets from multiple senders. These packets can be saved to the spiffs.
*/

MessagePersistPlugin *messagePersistPlugin;
MessagePersistPluginRadio *messagePersistPluginRadio;

MessagePersistPlugin::MessagePersistPlugin() : concurrency::OSThread("MessagePersistPlugin") {}

// uint32_t packetSequence = 0;

#define SEC_PER_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

int32_t MessagePersistPlugin::runOnce()
{
#ifndef NO_ESP32

    /*
        Uncomment the preferences below if you want to use the plugin
        without having to configure it from the PythonAPI or WebUI.
    */

    // radioConfig.preferences.message_persist_plugin_enabled = 1;

    // Fixed position is useful when testing indoors.
    // radioConfig.preferences.fixed_position = 1;


    if (radioConfig.preferences.message_persist_plugin_enabled) {

        if (firstTime) {

            // Interface with the serial peripheral from in here.

            messagePersistPluginRadio = new MessagePersistPluginRadio();

            firstTime = 0;

            DEBUG_MSG("Initializing Message Persist Plugin\n");
            return (5000); // Sending first message 5 seconds after initilization.

        } else {
                return (500);
        }

    } else {
        DEBUG_MSG("Message Persist Plugin - Disabled\n");
    }

    return (INT32_MAX);
#endif
}

MeshPacket *MessagePersistPluginRadio::allocReply()
{

    auto reply = allocDataPacket(); // Allocate a packet for sending

    return reply;
}

// Jake: here we don't want to send a message but rather listen to message sent events and save to file.
void MessagePersistPluginRadio::sendPayload(NodeNum dest, bool wantReplies)
{

}

bool MessagePersistPluginRadio::handleReceived(const MeshPacket &mp)
{
#ifndef NO_ESP32

    if (radioConfig.preferences.message_persist_plugin_enabled) {

        auto &p = mp.decoded;
        DEBUG_MSG("Received text msg self=0x%0x, from=0x%0x, to=0x%0x, id=%d, msg=%.*s\n",
                nodeDB.getNodeNum(), mp.from, mp.to, mp.id, p.payload.size, p.payload.bytes);

        if (getFrom(&mp) != nodeDB.getNodeNum()) {

            DEBUG_MSG("* * Message came from the mesh\n");
            Serial2.println("* * Message came from the mesh");
            Serial2.printf("%s", p.payload.bytes);
            /*



            */

            NodeInfo *n = nodeDB.getNode(getFrom(&mp));

            if (radioConfig.preferences.message_persist_plugin_enabled) {
                appendFile(mp);
            }

            DEBUG_MSG("-----------------------------------------\n");
            DEBUG_MSG("p.payload.bytes  \"%s\"\n", p.payload.bytes);
            DEBUG_MSG("p.payload.size   %d\n", p.payload.size);
            DEBUG_MSG("---- Received Packet:\n");
            DEBUG_MSG("mp.from          %d\n", mp.from);
            DEBUG_MSG("mp.rx_snr        %f\n", mp.rx_snr);
            DEBUG_MSG("mp.hop_limit     %d\n", mp.hop_limit);
            // DEBUG_MSG("mp.decoded.position.latitude_i     %d\n", mp.decoded.position.latitude_i); // Depricated
            // DEBUG_MSG("mp.decoded.position.longitude_i    %d\n", mp.decoded.position.longitude_i); // Depricated
            DEBUG_MSG("---- Node Information of Received Packet (mp.from):\n");
            DEBUG_MSG("n->user.long_name         %s\n", n->user.long_name);
            DEBUG_MSG("n->user.short_name        %s\n", n->user.short_name);
            // DEBUG_MSG("n->user.macaddr           %X\n", n->user.macaddr);
            DEBUG_MSG("n->has_position           %d\n", n->has_position);
            DEBUG_MSG("n->position.latitude_i    %d\n", n->position.latitude_i);
            DEBUG_MSG("n->position.longitude_i   %d\n", n->position.longitude_i);
            DEBUG_MSG("n->position.battery_level %d\n", n->position.battery_level);
            DEBUG_MSG("---- Current device location information:\n");
            DEBUG_MSG("gpsStatus->getLatitude()     %d\n", gpsStatus->getLatitude());
            DEBUG_MSG("gpsStatus->getLongitude()    %d\n", gpsStatus->getLongitude());
            DEBUG_MSG("gpsStatus->getHasLock()      %d\n", gpsStatus->getHasLock());
            DEBUG_MSG("gpsStatus->getDOP()          %d\n", gpsStatus->getDOP());
            DEBUG_MSG("-----------------------------------------\n");
        }

    } else {
        DEBUG_MSG("Range Test Plugin Disabled\n");
    }

#endif

    return true; // Let others look at this message also if they want
}

/// Ported from my old java code, returns distance in meters along the globe
/// surface (by magic?)
float MessagePersistPluginRadio::latLongToMeter(double lat_a, double lng_a, double lat_b, double lng_b)
{
    double pk = (180 / 3.14169);
    double a1 = lat_a / pk;
    double a2 = lng_a / pk;
    double b1 = lat_b / pk;
    double b2 = lng_b / pk;
    double cos_b1 = cos(b1);
    double cos_a1 = cos(a1);
    double t1 = cos_a1 * cos(a2) * cos_b1 * cos(b2);
    double t2 = cos_a1 * sin(a2) * cos_b1 * sin(b2);
    double t3 = sin(a1) * sin(b1);
    double tt = acos(t1 + t2 + t3);
    if (isnan(tt))
        tt = 0.0; // Must have been the same point?

    return (float)(6366000 * tt);
}

bool MessagePersistPluginRadio::appendFile(const MeshPacket &mp)
{
    auto &p = mp.decoded;

    NodeInfo *n = nodeDB.getNode(getFrom(&mp));
    /*
        DEBUG_MSG("-----------------------------------------\n");
        DEBUG_MSG("p.payload.bytes  \"%s\"\n", p.payload.bytes);
        DEBUG_MSG("p.payload.size   %d\n", p.payload.size);
        DEBUG_MSG("---- Received Packet:\n");
        DEBUG_MSG("mp.from          %d\n", mp.from);
        DEBUG_MSG("mp.rx_snr        %f\n", mp.rx_snr);
        DEBUG_MSG("mp.hop_limit     %d\n", mp.hop_limit);
        // DEBUG_MSG("mp.decoded.position.latitude_i     %d\n", mp.decoded.position.latitude_i);  // Depricated
        // DEBUG_MSG("mp.decoded.position.longitude_i    %d\n", mp.decoded.position.longitude_i); // Depricated
        DEBUG_MSG("---- Node Information of Received Packet (mp.from):\n");
        DEBUG_MSG("n->user.long_name         %s\n", n->user.long_name);
        DEBUG_MSG("n->user.short_name        %s\n", n->user.short_name);
        DEBUG_MSG("n->user.macaddr           %X\n", n->user.macaddr);
        DEBUG_MSG("n->has_position           %d\n", n->has_position);
        DEBUG_MSG("n->position.latitude_i    %d\n", n->position.latitude_i);
        DEBUG_MSG("n->position.longitude_i   %d\n", n->position.longitude_i);
        DEBUG_MSG("n->position.battery_level %d\n", n->position.battery_level);
        DEBUG_MSG("---- Current device location information:\n");
        DEBUG_MSG("gpsStatus->getLatitude()     %d\n", gpsStatus->getLatitude());
        DEBUG_MSG("gpsStatus->getLongitude()    %d\n", gpsStatus->getLongitude());
        DEBUG_MSG("gpsStatus->getHasLock()      %d\n", gpsStatus->getHasLock());
        DEBUG_MSG("gpsStatus->getDOP()          %d\n", gpsStatus->getDOP());
        DEBUG_MSG("-----------------------------------------\n");
    */
    if (!SPIFFS.begin(true)) {
        DEBUG_MSG("An Error has occurred while mounting SPIFFS\n");
        return 0;
    }

    if (SPIFFS.totalBytes() - SPIFFS.usedBytes() < 51200) {
        DEBUG_MSG("SPIFFS doesn't have enough free space. Abourting write.\n");
        return 0;
    }

    // If the file doesn't exist, write the header.
    if (!SPIFFS.exists("/static/messages.csv")) {
        //--------- Write to file
        File fileToWrite = SPIFFS.open("/static/messages.csv", FILE_WRITE);

        if (!fileToWrite) {
            DEBUG_MSG("There was an error opening the file for writing\n");
            return 0;
        }

        // Print the CSV header
        if (fileToWrite.println(
                "time,from,sender name,sender lat,sender long,rx lat,rx long,rx snr,rx elevation,distance,payload")) {
            DEBUG_MSG("File was written\n");
        } else {
            DEBUG_MSG("File write failed\n");
        }

        fileToWrite.close();
    }

    //--------- Apend content to file
    File fileToAppend = SPIFFS.open("/static/messages.csv", FILE_APPEND);

    if (!fileToAppend) {
        DEBUG_MSG("There was an error opening the file for appending\n");
        return 0;
    }

    struct timeval tv;
    if (!gettimeofday(&tv, NULL)) {
        long hms = tv.tv_sec % SEC_PER_DAY;
        // hms += tz.tz_dsttime * SEC_PER_HOUR;
        // hms -= tz.tz_minuteswest * SEC_PER_MIN;
        // mod `hms` to ensure in positive range of [0...SEC_PER_DAY)
        hms = (hms + SEC_PER_DAY) % SEC_PER_DAY;

        // Tear apart hms into h:m:s
        int hour = hms / SEC_PER_HOUR;
        int min = (hms % SEC_PER_HOUR) / SEC_PER_MIN;
        int sec = (hms % SEC_PER_HOUR) % SEC_PER_MIN; // or hms % SEC_PER_MIN

        fileToAppend.printf("%02d:%02d:%02d,", hour, min, sec); // Time
    } else {
        fileToAppend.printf("??:??:??,"); // Time
    }

    fileToAppend.printf("%d,", getFrom(&mp));                     // From
    fileToAppend.printf("%s,", n->user.long_name);                // Long Name
    fileToAppend.printf("%f,", n->position.latitude_i * 1e-7);    // Sender Lat
    fileToAppend.printf("%f,", n->position.longitude_i * 1e-7);   // Sender Long
    fileToAppend.printf("%f,", gpsStatus->getLatitude() * 1e-7);  // RX Lat
    fileToAppend.printf("%f,", gpsStatus->getLongitude() * 1e-7); // RX Long
    fileToAppend.printf("%d,", gpsStatus->getAltitude());         // RX Altitude

    fileToAppend.printf("%f,", mp.rx_snr); // RX SNR

    if (n->position.latitude_i && n->position.longitude_i && gpsStatus->getLatitude() && gpsStatus->getLongitude()) {
        float distance = latLongToMeter(n->position.latitude_i * 1e-7, n->position.longitude_i * 1e-7,
                                        gpsStatus->getLatitude() * 1e-7, gpsStatus->getLongitude() * 1e-7);
        fileToAppend.printf("%f,", distance); // Distance in meters
    } else {
        fileToAppend.printf("0,");
    }

    // TODO: If quotes are found in the payload, it has to be escaped.
    fileToAppend.printf("\"%s\"\n", p.payload.bytes);

    fileToAppend.close();

    return 1;
}
