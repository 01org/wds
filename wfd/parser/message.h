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


#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <memory>

#include "header.h"
#include "payload.h"

namespace wfd {

class Message {
 public:
  explicit Message(bool is_reply);
  virtual ~Message();

  bool is_reply() const { return is_reply_; }
  bool is_request() const { return !is_reply_; }

  int cseq() const;

  void set_header(std::unique_ptr<Header> header) {
    header_ = std::move(header);
  }

  Header& header();

  void set_payload(std::unique_ptr<Payload> payload) {
    payload_ = std::move(payload);
  }

  Payload& payload();

  virtual std::string to_string() const;

 protected:
  std::unique_ptr<Header> header_;
  std::unique_ptr<Payload> payload_;

 private:
  bool is_reply_;
};

class Request : public Message {
 public:
  enum ID {
      UNKNOWN, M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12, M13, M14,
      M15, M16
  };

  enum RTSPMethod {
    MethodOptions = 1,
    MethodSetParameter,
    MethodGetParameter,
    MethodSetup,
    MethodPlay,
    MethodTeardown,
    MethodPause
  };

  Request(RTSPMethod method, const std::string& request_uri = std::string());
  virtual ~Request();

  const std::string& request_uri() const { return request_uri_; }
  void set_request_uri(const std::string& request_uri) {
    request_uri_ = request_uri;
  }

  ID id() const { return id_; }
  void set_id(ID id) { id_ = id; }

  RTSPMethod method() const { return method_; }
  void set_method(RTSPMethod method) { method_ = method; }

 private:
  ID id_;
  RTSPMethod method_;
  std::string request_uri_;
};

inline Request* ToRequest(Message* message) {
  return static_cast<Request*>(message);
}

}  // namespace wfd

#endif  // MESSAGE_H_
