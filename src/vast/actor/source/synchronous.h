#ifndef VAST_ACTOR_SOURCE_SYNCHRONOUS_H
#define VAST_ACTOR_SOURCE_SYNCHRONOUS_H

#include <caf/all.hpp>
#include "vast/event.h"
#include "vast/actor/actor.h"
#include "vast/util/result.h"

namespace vast {
namespace source {

/// A synchronous source that extracts events one by one.
template <typename Derived>
struct synchronous : public default_actor
{
public:
  void at(caf::exit_msg const& msg) override
  {
    send_events();
    this->quit(msg.reason);
  }

  caf::message_handler make_handler() override
  {
    using namespace caf;
    this->attach_functor([=](uint32_t) { sinks_.clear(); });

    return
    {
      on(atom("batch size"), arg_match) >> [=](uint64_t batch_size)
      {
        VAST_DEBUG(this, "sets batch size to", batch_size);
        batch_size_ = batch_size;
      },
      on(atom("sink"), arg_match) >> [=](actor const& sink)
      {
        VAST_DEBUG(this, "adds sink to", sink);
        sinks_.push_back(sink);
      },
      on(atom("start")) >> [=]
      {
        running_ = true;
        this->send(this, atom("run"));
      },
      on(atom("stop")) >> [=]
      {
        running_ = false;
      },
      on(atom("run")) >> [=]
      {
        if (sinks_.empty())
        {
          VAST_ERROR(this, "cannot run without sinks");
          this->quit(exit::error);
          return;
        }

        bool done = false;
        while (events_.size() < batch_size_ && ! done)
        {
          result<event> r = static_cast<Derived*>(this)->extract();
          if (r)
          {
            events_.push_back(std::move(*r));
          }
          else if (r.failed())
          {
            VAST_ERROR(this, r.error());
            done = true;
            break;
          }

          done = static_cast<Derived const*>(this)->done();
        }

        send_events();

        if (done)
          this->quit(exit::done);
        else if (running_)
          this->send(this, this->last_dequeued());
      }
    };
  }

private:
  void send_events()
  {
    if (! events_.empty())
    {
      for (auto& a : sinks_)
        this->send(a, std::move(events_));
      events_ = {};
    }
  }

  bool running_ = true;
  std::vector<caf::actor> sinks_;
  uint64_t batch_size_ = std::numeric_limits<uint16_t>::max();
  std::vector<event> events_;
};

} // namespace source
} // namespace vast

#endif
