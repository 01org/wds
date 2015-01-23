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

#include <iostream>

#include "desktop_media_manager.h"
#include "mirac_broker_source.h"

#include "wfd/public/source.h"

MiracBrokerSource::MiracBrokerSource(int rtsp_port)
  : MiracBroker(std::to_string(rtsp_port)) {
}

MiracBrokerSource::~MiracBrokerSource() {}

void MiracBrokerSource::got_message(const std::string& message) {
  wfd_source_->RTSPDataReceived(message);
}

void MiracBrokerSource::on_connected() {
  media_manager_.reset(new DesktopMediaManager(get_peer_address()));
  wfd_source_.reset(wfd::Source::Create(this, media_manager_.get()));
  wfd_source_->Start();
}

void MiracBrokerSource::on_connect_timeout() {
  std::cout << "* RTSP connection failed: timeout" << std::endl;
}

wfd::Peer* MiracBrokerSource::Peer() const {
  return wfd_source_.get();
}
