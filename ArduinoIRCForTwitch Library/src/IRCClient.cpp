/*
  IRCClient.cpp - Internet Relay Chat library v0.1.0 - 2016-4-9
  Copyright (C) 2016 Fredi Machado.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.
  Modified by WhatAboutGamingLive to have more Twitch IRC support
*/

#include "IRCClient.h"

IRCClient::IRCClient(const char* host, uint16_t port, Client& client) {
  this->host = host;
  this->port = port;
  this->client = &client;
  this->isConnected = false;
}

IRCClient& IRCClient::setCallback(IRC_CALLBACK_SIGNATURE) {
  this->callback = callback;
  return *this;
}

IRCClient& IRCClient::setSentCallback(IRC_SENTCALLBACK_SIGNATURE) {
  this->debugSentCallback = debugSentCallback;
  return *this;
}

boolean IRCClient::connect(String nickname, String user, String password) {
  if (!connected()) {
    int result = client->connect(this->host, this->port);
    if (result == 1) {
      this->nickname = nickname;
      //sendIRC("HELLO");
      if(password != "")
      {
        sendIRC("PASS " + password);
      }
      sendIRC("NICK " + nickname);
      sendIRC("USER " + user);
      //sendIRC("CAP REQ :twitch.tv/commands");
      //sendIRC("CAP REQ :twitch.tv/tags");
      //sendIRC("CAP REQ :twitch.tv/membership");
      sendCapReq("twitch.tv/commands");
      sendCapReq("twitch.tv/tags");
      sendCapReq("twitch.tv/membership");
      this->isConnected = true;
      return true;
    }
    return false;
  }
  return true;
}

void IRCClient::disconnect() {
  //Serial.println("Attempting to disconnect from IRC");
  client->flush();
  client->stop();
  //Serial.println("Disconnected from IRC, hopefully");
}

boolean IRCClient::loop() {
  if (connected() && client->available()) {
    String message = "";
    while (client->available()) {
      char c = client->read();
      if (c == '\r') {
        client->read(); // discard \n
        parse(message);
        message = "";
      } else {
        message += c;
      }
    }
    return true;
  }
  return false;
}

void IRCClient::parse(String data) {
  IRCMessage ircMessage(data);

  if (data[0] == ':') {
    ircMessage.prefix = data.substring(1, data.indexOf(" ") - 1);
    int index = ircMessage.prefix.indexOf("@");
    if (index != -1) {
      ircMessage.nick = ircMessage.prefix.substring(0, index);
      ircMessage.host = ircMessage.prefix.substring(index);
    }
    index = ircMessage.nick.indexOf("!");
    if (index != -1) {
      String temp = ircMessage.nick;
      ircMessage.nick = temp.substring(0, index);
      ircMessage.user = temp.substring(index);
    }

    data = data.substring(data.indexOf(" ") + 1);
  }

  int index = data.indexOf(" ");
  ircMessage.command = data.substring(0, index);
  ircMessage.command.toUpperCase();

  data = data.substring(index + 1);

  if (data != "") {
    if (data[0] == ':') {
      ircMessage.text = data.substring(1);
    } else {
      int pos1 = 0, pos2;
      while ((pos2 = data.indexOf(" ", pos1)) != -1) {
        pos1 = pos2 + 1;
        if (data[pos1] == ':') {
          ircMessage.parameters = data.substring(0, pos2);
          ircMessage.text = data.substring(pos1 + 1);
          break;
        }
      }
      if (ircMessage.text == "") {
        ircMessage.text = data;
      }
    }
  }
  
  if (ircMessage.command == "PING") {
    sendIRC("PONG " + data);
    /*
    Serial.println("FROM LIBRARY PING");
    Serial.print("PONG ");
    Serial.println(data);
    Serial.println("HELL YEAH");
    */
    //return;
  }

  /*
  if (ircMessage.command == "PRIVMSG") {
    String to = ircMessage.parameters;
    String text = ircMessage.text;
    
    if (text[0] == '\001') { // CTCP
      text = text.substring(1, text.length() - 1);
      if (to == this->nickname) {
        if (text == "VERSION") {
          sendIRC("NOTICE " + ircMessage.nick + " :\001VERSION Open source Arduino IRC client by Fredi Machado - https://github.com/Fredi/ArduinoIRC \001");
          return;
        }
        // CTCP not implemented
        sendIRC("NOTICE " + ircMessage.nick + " :\001ERRMSG " + text + " :Not implemented\001");
        return;
      }
    }
  }
  */
  executeCallback(ircMessage);
}

boolean IRCClient::connected() {
  if (client == NULL) {
    return false;
  }
  boolean rc = (int)client->connected();
  if (!rc && this->isConnected) {
    this->isConnected = false;
    client->flush();
    client->stop();
  }
  return rc;
}

void IRCClient::sendIRC(String data) {
  client->print(data + "\r\n");
  if (debugSentCallback) {
    debugSentCallback("SENT: " + data);
  }
}

void IRCClient::executeCallback(IRCMessage ircMessage) {
  if (callback) {
    callback(ircMessage);
  }
}

void IRCClient::sendRaw(String rawData) {
  sendIRC(rawData);
}

void IRCClient::sendCapReq(String capReq) {
  String rawData("CAP REQ :" + capReq);
  //Serial.print("Sending capReq ");
  //Serial.println(rawData);
  sendIRC(rawData);
}

void IRCClient::joinChannel(String channelName) {
  String rawData("JOIN #" + channelName);
  sendIRC(rawData);
}

void IRCClient::sendMessage(String channelName, String message) {
  String rawData("PRIVMSG #" + channelName + " :" + message);
  sendIRC(rawData);
}

void IRCClient::sendAction(String channelName, String message) {
  String rawData("PRIVMSG #" + channelName + " :ACTION " + message + "");
  sendIRC(rawData);
}

void IRCClient::sendWhisper(String userName, String message) {
  String rawData("PRIVMSG #twitch :.w " + userName + " " + message);
  sendIRC(rawData);
}

void IRCClient::sendNormalReply(String channelName, String message, String messageId) {
  String rawData("@reply-parent-msg-id=" + messageId + " PRIVMSG #" + channelName + " :" + message);
  sendIRC(rawData);
}

void IRCClient::sendActionReply(String channelName, String message, String messageId) {
  String rawData("@reply-parent-msg-id=" + messageId + " PRIVMSG #" + channelName + " :ACTION " + message + "");
  sendIRC(rawData);
}