#include "vast/segmentizer.h"

#include <caf/all.hpp>
#include "vast/event.h"
#include "vast/logger.h"

namespace vast {

using namespace caf;

segmentizer::segmentizer(actor upstream,
                         size_t max_events_per_chunk, size_t max_segment_size)
  : upstream_{upstream},
    stats_{std::chrono::seconds(1)},
    segment_{uuid::random(), max_segment_size},
    writer_{&segment_, max_events_per_chunk}
{
}

message_handler segmentizer::act()
{
  trap_exit(true);

  attach_functor([=](uint32_t) { upstream_ = invalid_actor; });

  return
  {
    [=](exit_msg const& e)
    {
      if (! writer_.flush())
      {
        segment_ = segment{uuid::random()};
        writer_.attach_to(&segment_);
        if (! writer_.flush())
          VAST_LOG_ACTOR_ERROR("failed to flush a fresh segment");

        assert(segment_.events() > 0);
      }

      if (segment_.events() > 0)
      {
        VAST_LOG_ACTOR_DEBUG(
            "sends final segment " << segment_.id() << " with " <<
            segment_.events() << " events to " << upstream_);

        send(upstream_, std::move(segment_));
      }

      if (total_events_ > 0)
        VAST_LOG_ACTOR_VERBOSE("processed " << total_events_ << " events");

      quit(e.reason);
    },
    [=](std::vector<event> const& v)
    {
      total_events_ += v.size();

      for (auto& e : v)
      {
        if (writer_.write(e))
        {
          if (stats_.increment())
          {
            send(upstream_, atom("statistics"), stats_.last());
            VAST_LOG_ACTOR_VERBOSE(
                "ingests at rate " << stats_.last() << " events/sec" <<
                " (mean " << stats_.mean() <<
                ", median " << stats_.median() <<
                ", standard deviation " << std::round(stats_.sd()) << ")");
          }
        }
        else
        {
          VAST_LOG_ACTOR_DEBUG("sends segment " << segment_.id() <<
                               " with " << segment_.events() <<
                               " events to " << upstream_);

          auto max_segment_size = segment_.max_bytes();
          send(upstream_, std::move(segment_));

          segment_ = segment{uuid::random(), max_segment_size};
          writer_.attach_to(&segment_);

          if (! writer_.flush())
          {
            VAST_LOG_ACTOR_ERROR("failed to flush chunk to fresh segment");
            quit(exit::error);
            return;
          }

          if (! writer_.write(e))
          {
            VAST_LOG_ACTOR_ERROR("failed to write event to fresh segment");
            quit(exit::error);
            return;
          }
        }
      }
    }
  };
}

std::string segmentizer::describe() const
{
  return "segmentizer";
}


} // namespace vast
