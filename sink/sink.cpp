/*
 * This file is part of wysiwidi
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include "sink.h"
#include "sink_media_manager.h"

Sink::Sink(const std::string& host, int rtsp_port)
  : MiracBroker(host.c_str(), std::to_string(rtsp_port)) {
}

Sink::~Sink() {}

void Sink::got_message(const std::string& message) {
  wfd_sink_->RTSPDataReceived(message);
}

void Sink::on_connected() {
  media_manager_.reset(new SinkMediaManager(get_peer_address()));
  wfd_sink_.reset(wfd::Sink::Create(this, media_manager_.get()));
  wfd_sink_->Start();
}

void Sink::Play() {
  wfd_sink_->Play();
}

void Sink::Pause() {
  wfd_sink_->Pause();
}

void Sink::Teardown() {
  wfd_sink_->Teardown();
}

