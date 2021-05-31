#pragma once

#include "SinglePortPlugin.h"
#include "concurrency/OSThread.h"
#include "configuration.h"
#include <Arduino.h>
#include <functional>

class MessagePersistPlugin : private concurrency::OSThread
{
    bool firstTime = 1;

  public:
    MessagePersistPlugin();

  protected:
    virtual int32_t runOnce();
};

extern MessagePersistPlugin *messagePersistPlugin;

/*
 * Radio interface for MessagePersistPlugin
 *
 */
class MessagePersistPluginRadio : public SinglePortPlugin
{
    uint32_t lastRxID;

  public:
    MessagePersistPluginRadio() : SinglePortPlugin("MessagePersistPluginRadio", PortNum_TEXT_MESSAGE_APP) {}

    /**
     * Send our payload into the mesh
     */
    void sendPayload(NodeNum dest = NODENUM_BROADCAST, bool wantReplies = false);

    /**
     * Append range test data to the file on the spiffs
     */
    bool appendFile(const MeshPacket &mp);

    /**
     * Kevin's magical calculation of two points to meters.
     */
    float latLongToMeter(double lat_a, double lng_a, double lat_b, double lng_b);

  protected:
    virtual MeshPacket *allocReply();

    /** Called to handle a particular incoming message

    @return true if you've guaranteed you've handled this message and no other handlers should be considered for it
    */
    virtual bool handleReceived(const MeshPacket &mp);
};

extern MessagePersistPluginRadio *messagePersistPluginRadio;
