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

#include <algorithm>

#include "message_handler.h"
#include "wfd/public/media_manager.h"

namespace wfd {

int MessageHandler::send_cseq_ = 1;

MessageSequenceHandler::MessageSequenceHandler(const InitParams& init_params)
  : MessageHandler(init_params),
    current_handler_(nullptr) {
}

MessageSequenceHandler::~MessageSequenceHandler() {
  for (MessageHandler* handler : handlers_)
    delete handler;
}

void MessageSequenceHandler::Start() {
  if (current_handler_) {
    return;
  }
  current_handler_ = handlers_.front();
  current_handler_->Start();
}

void MessageSequenceHandler::Reset() {
  if (current_handler_) {
    current_handler_->Reset();
    current_handler_ = nullptr;
  }
}

bool MessageSequenceHandler::CanSend(Message* message) const {
  return current_handler_->CanSend(message);
}

void MessageSequenceHandler::Send(std::unique_ptr<Message> message) {
  current_handler_->Send(std::move(message));
}

bool MessageSequenceHandler::CanHandle(Message* message) const {
  return current_handler_->CanHandle(message);
}

void MessageSequenceHandler::Handle(std::unique_ptr<Message> message) {
  current_handler_->Handle(std::move(message));
}

void MessageSequenceHandler::AddSequencedHandler(MessageHandler* handler) {
  assert(!current_handler_); // We are not started
  assert(handler);
  assert(handlers_.end() == std::find(
      handlers_.begin(), handlers_.end(), handler));
  handlers_.push_back(handler);
  handler->set_observer(this);
}

void MessageSequenceHandler::OnCompleted(MessageHandler* handler) {
  assert(handler == current_handler_);
  current_handler_->Reset();

  auto it = std::find(handlers_.begin(), handlers_.end(), handler);
  assert(handlers_.end() != it);
  if (++it == handlers_.end()) {
    observer_->OnCompleted(this);
    return;
  }

  current_handler_ = *it;
  current_handler_->Start();
}

void MessageSequenceHandler::OnError(MessageHandler* handler) {
  assert(handler == current_handler_);
  handler->Reset();
  observer_->OnError(this);
}

MessageSequenceWithOptionalSetHandler::MessageSequenceWithOptionalSetHandler(
    const InitParams& init_params)
  : MessageSequenceHandler(init_params) {
}

MessageSequenceWithOptionalSetHandler::~MessageSequenceWithOptionalSetHandler() {
  for (MessageHandler* handler : optional_handlers_)
    delete handler;
}

void MessageSequenceWithOptionalSetHandler::Start() {
  MessageSequenceHandler::Start();
  for (MessageHandler* handler : optional_handlers_)
    handler->Start();
}

void MessageSequenceWithOptionalSetHandler::Reset() {
  MessageSequenceHandler::Reset();
  for (MessageHandler* handler : optional_handlers_)
    handler->Reset();
}

bool MessageSequenceWithOptionalSetHandler::CanSend(Message* message) const {
  for (MessageHandler* handler : optional_handlers_)
    if (handler->CanSend(message))
      return true;

  if (MessageSequenceHandler::CanSend(message))
    return true;

  return false;
}

void MessageSequenceWithOptionalSetHandler::Send(std::unique_ptr<Message> message) {
  for (MessageHandler* handler : optional_handlers_) {
    if (handler->CanSend(message.get())) {
      handler->Send(std::move(message));
      return;
    }
  }

  if (MessageSequenceHandler::CanSend(message.get())) {
    MessageSequenceHandler::Send(std::move(message));
    return;
  }

  observer_->OnError(this);
}

bool MessageSequenceWithOptionalSetHandler::CanHandle(Message* message) const {
  if (MessageSequenceHandler::CanHandle(message))
    return true;

  for (MessageHandler* handler : optional_handlers_)
    if (handler->CanHandle(message))
      return true;

  return false;
}

void MessageSequenceWithOptionalSetHandler::Handle(std::unique_ptr<Message> message) {
  if (MessageSequenceHandler::CanHandle(message.get())) {
     MessageSequenceHandler::Handle(std::move(message));
     return;
  }

  for (MessageHandler* handler : optional_handlers_) {
    if (handler->CanHandle(message.get())) {
      handler->Handle(std::move(message));
      return;
    }
  }

  observer_->OnError(this);
}

void MessageSequenceWithOptionalSetHandler::AddOptionalHandler(
    MessageHandler* handler) {
  assert(handler);
  assert(optional_handlers_.end() == std::find(
      optional_handlers_.begin(), optional_handlers_.end(), handler));
  optional_handlers_.push_back(handler);
  handler->set_observer(this);
}

void MessageSequenceWithOptionalSetHandler::OnCompleted(MessageHandler* handler) {
  auto it = std::find(
      optional_handlers_.begin(), optional_handlers_.end(), handler);
  if (it != optional_handlers_.end()) {
    handler->Reset();
    handler->Start();
    return;
  }
  MessageSequenceHandler::OnCompleted(handler);
}

void MessageSequenceWithOptionalSetHandler::OnError(MessageHandler* handler) {
  handler->Reset();
  observer_->OnError(this);
}

// MessageReceiverBase
MessageReceiverBase::MessageReceiverBase(const InitParams& init_params)
  : MessageHandler(init_params),
    wait_for_message_(false) {}

MessageReceiverBase::~MessageReceiverBase() {}

bool MessageReceiverBase::CanHandle(Message* message) const {
  assert(message);
  return wait_for_message_;
}

void MessageReceiverBase::Start() { wait_for_message_ = true; }
void MessageReceiverBase::Reset() { wait_for_message_ = false; }
bool MessageReceiverBase::CanSend(Message* message) const { return false; }
void MessageReceiverBase::Send(std::unique_ptr<Message> message) {}
void MessageReceiverBase::Handle(std::unique_ptr<Message> message) {
  assert(message);
  if (!CanHandle(message.get())) {
    observer_->OnError(this);
    return;
  }
  wait_for_message_ = false;
  std::unique_ptr<Reply> reply = HandleMessage(message.get());
  if (!reply) {
    observer_->OnError(this);
    return;
  }
  reply->header().set_cseq(message->cseq());
  sender_->SendRTSPData(reply->to_string());
  observer_->OnCompleted(this);
}

MessageSenderBase::MessageSenderBase(const InitParams& init_params)
  : MessageHandler(init_params) {
}

MessageSenderBase::~MessageSenderBase() {
}

void MessageSenderBase::Reset() {
  while (!cseq_queue_.empty())
    cseq_queue_.pop();
}

void MessageSenderBase::Send(std::unique_ptr<Message> message) {
  assert(message);
  if (!CanSend(message.get())) {
    observer_->OnError(this);
    return;
  }
  cseq_queue_.push(message->cseq()); // TODO : Add timeout check for reply.
  sender_->SendRTSPData(message->to_string());
}

bool MessageSenderBase::CanHandle(Message* message) const {
  assert(message);
  return message->is_reply() && !cseq_queue_.empty() &&
         (message->cseq() == cseq_queue_.front());
}

void MessageSenderBase::Handle(std::unique_ptr<Message> message) {
  assert(message);
  if (!CanHandle(message.get())) {
    observer_->OnError(this);
    return;
  }
  cseq_queue_.pop();

  if (!HandleReply(static_cast<Reply*>(message.get())))
    observer_->OnError(this);

  if (cseq_queue_.empty()) {
    observer_->OnCompleted(this);
  }
}

SequencedMessageSender::SequencedMessageSender(const InitParams& init_params)
  : MessageSenderBase(init_params),
    to_be_send_(nullptr) {
}

SequencedMessageSender::~SequencedMessageSender() {
}

void SequencedMessageSender::Start() {
  auto message = CreateMessage();
  to_be_send_ = message.get();
  Send(std::move(message));
}

void SequencedMessageSender::Reset() {
  to_be_send_ = nullptr;
  MessageSenderBase::Reset();
}

bool SequencedMessageSender::CanSend(Message* message) const {
  return message && (message == to_be_send_);
}

}
