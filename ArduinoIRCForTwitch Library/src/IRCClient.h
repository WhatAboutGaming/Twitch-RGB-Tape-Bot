/*
  IRCClient.h - Internet Relay Chat library v0.1.0 - 2016-4-9
  Copyright (C) 2016 Fredi Machado.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.
  Modified by WhatAboutGamingLive to have more Twitch IRC support
*/

#ifndef IRCClient_h
#define IRCClient_h

#include "Arduino.h"
#include "Client.h"
#include "IRCMessage.h"

#ifdef ESP8266
#include <functional>
#define IRC_CALLBACK_SIGNATURE std::function<void(IRCMessage ircMessage)> callback
#define IRC_SENTCALLBACK_SIGNATURE std::function<void(String data)> debugSentCallback
#else
#define IRC_CALLBACK_SIGNATURE void (*callback)(IRCMessage ircMessage)
#define IRC_SENTCALLBACK_SIGNATURE void (*debugSentCallback)(String data)
#endif

class IRCClient
{
  private:
    Client* client;
    IRC_CALLBACK_SIGNATURE;
    IRC_SENTCALLBACK_SIGNATURE;
    const char* host;
    uint16_t port;
    bool isConnected;
    String nickname;
    void sendIRC(String data);
    void parse(String data);
    void executeCallback(IRCMessage ircMessage);

  public:
    IRCClient(const char*, uint16_t, Client& client);
    IRCClient& setCallback(IRC_CALLBACK_SIGNATURE);
    IRCClient& setSentCallback(IRC_SENTCALLBACK_SIGNATURE);
    boolean connect(String nickname, String user, String password = "");
    void disconnect();
    boolean loop();
    boolean connected();
    void sendRaw(String data);
    void sendCapReq(String capReq);
    void joinChannel(String channelName);
    void sendMessage(String channelName, String message);
    void sendAction(String channelName, String message);
    void sendWhisper(String userName, String message);
    void sendNormalReply(String channelName, String message, String messageId);
    void sendActionReply(String channelName, String message, String messageId);
};

#endif /* IRCClient_h */