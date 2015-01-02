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

#include "gst_sink_media_manager.h"

GstSinkMediaManager::GstSinkMediaManager(const std::string& hostname)
  : gst_pipeline_(new MiracGstSink(hostname, 0)) {
}

void GstSinkMediaManager::Play() {
  gst_pipeline_->Play();
}

void GstSinkMediaManager::Pause() {
  gst_pipeline_->Pause();
}

void GstSinkMediaManager::Teardown() {
  gst_pipeline_->Teardown();
}

bool GstSinkMediaManager::IsPaused() const {
  return gst_pipeline_->IsPaused();
}

void GstSinkMediaManager::SetSinkRtpPorts(int port1, int port2) {
}

std::pair<int,int> GstSinkMediaManager::SinkRtpPorts() const {
  return std::pair<int,int>(gst_pipeline_->sink_udp_port(), 0);
}

int GstSinkMediaManager::SourceRtpPort() const {
  return 0;
}


void GstSinkMediaManager::SetPresentationUrl(const std::string& url) {
  presentation_url_ = url;
}

std::string GstSinkMediaManager::PresentationUrl() const {
  return presentation_url_;
}

void GstSinkMediaManager::SetSession(const std::string& session) {
  session_ = session;
}

std::string GstSinkMediaManager::Session() const {
  return session_;
}
