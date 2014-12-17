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

#include "init_state.h"

#include "wfd/parser/options.h"
#include "wfd/parser/reply.h"

namespace wfd {
namespace source {

class M1Handler final : public SequencedMessageSender {
 public:
  using SequencedMessageSender::SequencedMessageSender;
 private:
  virtual std::unique_ptr<Message> CreateMessage() override {
    Options* options = new Options("*");
    options->header().set_cseq(send_cseq_++);
    options->header().set_require_wfd_support(true);
    return std::unique_ptr<Message>(options);
  }

  virtual bool HandleReply(Reply* reply) override {
    return (reply->response_code() == 200);
  }

};

class M2Handler final : public MessageReceiver<Request::M2> {
 public:
  M2Handler(const InitParams& init_params)
    : MessageReceiver<Request::M2>(init_params) {
  }
  virtual std::unique_ptr<Reply> HandleMessage(
      Message* message) override {
    auto reply = std::unique_ptr<Reply>(new Reply(200));
    std::vector<wfd::Method> supported_methods;
    supported_methods.push_back(ORG_WFA_WFD_1_0);
    supported_methods.push_back(GET_PARAMETER);
    supported_methods.push_back(SET_PARAMETER);
    supported_methods.push_back(PLAY);
    supported_methods.push_back(PAUSE);
    supported_methods.push_back(SETUP);
    supported_methods.push_back(TEARDOWN);
    reply->header().set_supported_methods(supported_methods);
    return std::move(reply);
  }
};

InitState::InitState(const InitParams& init_params)
  : MessageSequenceHandler(init_params) {
  AddSequencedHandler(new M1Handler(init_params));
  AddSequencedHandler(new M2Handler(init_params));
}

InitState::~InitState() {
}

}  // source
}  // wfd
