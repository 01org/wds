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

#ifndef SINK_H
#define SINK_H

#include <memory>

#include "wfd/public/media_manager.h"
#include "wfd/public/sink.h"

#include "mirac-broker.hpp"

class Sink : public MiracBroker {
 public:
  explicit Sink(const std::string& host, int rtsp_port);
  ~Sink();

  void Play();
  void Pause();
  void Teardown();

 protected:
  virtual wfd::Peer* Peer() const override;

 private:
  virtual void got_message(const std::string& message) override;
  virtual void on_connected() override;

  std::unique_ptr<wfd::SinkMediaManager> media_manager_;
  std::unique_ptr<wfd::Sink> wfd_sink_;
};

#endif // SINK_H
