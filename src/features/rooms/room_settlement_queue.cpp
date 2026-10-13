#include "features/rooms/room_settlement_queue.h"

#include <iostream>
#include <stdexcept>

namespace bubble_fighter::server::features::rooms {
RoomSettlementQueue::RoomSettlementQueue(RoomMatchSettler settler)
    : settler_(std::move(settler)) {
  if (!settler_) {
    throw std::invalid_argument("Missing match settlement provider.");
  }
}

std::optional<RoomMatchResult> RoomSettlementQueue::Job::Result() const {
  std::lock_guard lock(mutex_);
  return result_;
}

std::shared_ptr<RoomSettlementQueue::Job> RoomSettlementQueue::Enqueue(
    RoomMatchRequest request) {
  const auto encoded = request.Encode();
  std::lock_guard lock(mutex_);
  const auto found = jobs_.find(request.match_id);
  if (found != jobs_.end()) {
    if (found->second->kRequest.Encode() != encoded) {
      throw std::invalid_argument("Changed frozen match request.");
    }
    return found->second;
  }
  auto job = std::shared_ptr<Job>(new Job(std::move(request)));
  jobs_.emplace(job->kRequest.match_id, job);
  return job;
}

void RoomSettlementQueue::TrySettle(const std::shared_ptr<Job>& job, Time now) {
  {
    std::lock_guard lock(job->mutex_);
    if (job->running_ || job->result_ || now < job->retry_at_) {
      return;
    }
    job->running_ = true;
  }
  try {
    auto result = settler_(job->kRequest);
    result.Validate(job->kRequest);
    {
      std::lock_guard lock(job->mutex_);
      job->result_ = std::move(result);
      job->running_ = false;
      if (job->delayed_) {
        std::cerr << "Local match settlement recovered.\n";
      }
    }
    // The room may still hold this completed job for delivery. A
    // departed room holds nothing: its reward remains in MySQL only.
    std::lock_guard lock(mutex_);
    const auto entry = jobs_.find(job->kRequest.match_id);
    if (entry != jobs_.end() && entry->second == job) {
      jobs_.erase(entry);
    }
  } catch (const std::exception&) {
    std::lock_guard lock(job->mutex_);
    job->running_ = false;
    job->retry_at_ = now + kRetryDelay;
    if (!job->delayed_) {
      std::cerr << "Local match settlement delayed; retrying in two seconds.\n";
    }
    job->delayed_ = true;
  }
}

void RoomSettlementQueue::ProcessPending(Time now) {
  std::vector<std::shared_ptr<Job>> pending;
  {
    std::lock_guard lock(mutex_);
    for (const auto& [id, job] : jobs_) {
      (void)id;
      pending.push_back(job);
    }
  }
  for (const auto& job : pending) {
    TrySettle(job, now);
  }
}

std::size_t RoomSettlementQueue::PendingCount() const {
  std::lock_guard lock(mutex_);
  return jobs_.size();
}
}  // namespace bubble_fighter::server::features::rooms
