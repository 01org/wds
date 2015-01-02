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

#include "wfd/public/source.h"


#include "cap_negotiation_state.h"
#include "init_state.h"
#include "streaming_state.h"
#include "wfd_session_state.h"
#include "wfd/common/message_handler.h"
#include "wfd/common/rtsp_input_handler.h"
#include "wfd/common/wfd_export.h"
#include "wfd/parser/setparameter.h"

namespace wfd {

namespace {

bool InitializeRequestId(Request* request) {
  Request::ID id;
  switch(request->method()) {
  case Request::MethodOptions:
    id = Request::M2;
    break;
  case Request::MethodSetup:
    id = Request::M6;
    break;
  case Request::MethodPlay:
    id = Request::M7;
    break;
  case Request::MethodTeardown:
    id = Request::M8;
    break;
  case Request::MethodPause:
    id = Request::M9;
    break;
  case Request::MethodSetParameter:
    if (request->payload().has_property(WFD_ROUTE))
      id = Request::M10;
    else if (request->payload().has_property(WFD_CONNECTOR_TYPE))
      id = Request::M11;
    else if (request->payload().has_property(WFD_STANDBY))
      id = Request::M12;
    else if (request->payload().has_property(WFD_IDR_REQUEST))
      id = Request::M13;
    else if (request->payload().has_property(WFD_UIBC_CAPABILITY))
      id = Request::M14;
    else if (request->payload().has_property(WFD_UIBC_SETTING))
      id = Request::M15;
    break;
  default:
    // TODO: warning.
    return false;
  }
  request->set_id(id);
  return true;
}

}

class SourceStateMachine : public MessageSequenceHandler {
 public:
   SourceStateMachine(const InitParams& init_params)
     : MessageSequenceHandler(init_params) {
     AddSequencedHandler(new source::InitState(init_params));
     AddSequencedHandler(new source::CapNegotiationState(init_params));
     AddSequencedHandler(new source::WfdSessionState(init_params));
     AddSequencedHandler(new source::StreamingState(init_params));
   }

   int GetNextCSeq() { return send_cseq_++; }
};

class SourceImpl final : public Source, public RTSPInputHandler, public MessageHandler::Observer {
 public:
  SourceImpl(Delegate* delegate, MediaManager* mng);

 private:
  // Source implementation.
  virtual void Start() override;
  virtual void RTSPDataReceived(const std::string& message) override;
  virtual bool Teardown() override;
  virtual bool Play() override;
  virtual bool Pause() override;

  // public MessageHandler::Observer
  virtual void OnCompleted(MessageHandler* handler) override;
  virtual void OnError(MessageHandler* handler) override;

  virtual void OnTimerEvent(uint timer_id) override;

  // RTSPInputHandler
  virtual void MessageParsed(std::unique_ptr<Message> message) override;

  std::unique_ptr<SourceStateMachine> state_machine_;
};

SourceImpl::SourceImpl(Delegate* delegate, MediaManager* mng)
  : state_machine_(new SourceStateMachine({delegate, mng, this})) {
}

void SourceImpl::Start() {
  state_machine_->Start();
}

void SourceImpl::RTSPDataReceived(const std::string& message) {
  InputReceived(message);
}

void SourceImpl::OnTimerEvent(uint timer_id) {
  if (state_machine_->HandleTimeoutEvent(timer_id))
    state_machine_->Reset();
}

namespace  {

std::unique_ptr<Message> CreateM5(int send_cseq, wfd::TriggerMethod::Method method) {
  auto set_param = std::unique_ptr<Request>(
      new SetParameter("rtsp://localhost/wfd1.0"));
  set_param->header().set_cseq(send_cseq);
  set_param->payload().add_property(
      std::shared_ptr<wfd::Property>(new TriggerMethod(method)));
  set_param->set_id(Request::M5);
  return std::move(set_param);
}

}

bool SourceImpl::Teardown() {
  auto m5 = CreateM5(state_machine_->GetNextCSeq(),
                     wfd::TriggerMethod::TEARDOWN);

  if (!state_machine_->CanSend(m5.get()))
    return false;
  state_machine_->Send(std::move(m5));
  return true;
}

bool SourceImpl::Play() {
  auto m5 = CreateM5(state_machine_->GetNextCSeq(),
                     wfd::TriggerMethod::PLAY);

  if (!state_machine_->CanSend(m5.get()))
    return false;
  state_machine_->Send(std::move(m5));
  return true;
}

bool SourceImpl::Pause() {
  auto m5 = CreateM5(state_machine_->GetNextCSeq(),
                     wfd::TriggerMethod::PAUSE);

  if (!state_machine_->CanSend(m5.get()))
    return false;
  state_machine_->Send(std::move(m5));
  return true;
}

void SourceImpl::OnCompleted(MessageHandler* handler) {}

void SourceImpl::OnError(MessageHandler* handler) {}

void SourceImpl::MessageParsed(std::unique_ptr<Message> message) {
  if (message->is_request() && !InitializeRequestId(ToRequest(message.get())))
    return; // FIXME : Report error.
  state_machine_->Handle(std::move(message));
}

WFD_EXPORT Source* Source::Create(Delegate* delegate, MediaManager* mng) {
  return new SourceImpl(delegate, mng);
}

}  // namespace wfd
