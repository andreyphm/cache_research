#include "cache_runner.hpp"
#include "ARC_cache.hpp"
#include "LFU_cache.hpp"
#include "2Q_cache.hpp"
#include "LIRS_cache.hpp"
#include "SlowGetPage.hpp"

#include <deque>
#include <numeric>
#include <stdexcept>
#include <string>
#include <variant>

namespace {

class Level {
public:
    using Storage = SlowGetPage<std::string>;
    using Lfu = LFU::Cache<std::string, Level>;
    using Arc = ARC::Cache<std::string, Level>;
    using TwoQ = TWO_Q::Cache<std::string, Level>;
    using Lirs = LIRS::Cache<std::string, Level>;

    Level(std::size_t& misses) : lower_(nullptr), hit_count_(nullptr) {
        std::get<Storage>(cache_).load = [&misses](const std::string& key) {
            ++misses;
            return key;
        };
    }

    Level(const CachePolicy policy, const std::size_t capacity, Level& lower_level, std::size_t& level_hits)
        : lower_(&lower_level), hit_count_(&level_hits) {
        lower_level.upper_ = this;
        const auto invalidate_upper = [this](const std::string& key) {
            if (upper_) {
                upper_->remove(key);
            }
        };
        switch (policy) {
            case CachePolicy::LFU:
                cache_.emplace<Lfu>(lower_level, capacity, invalidate_upper);
                break;
            case CachePolicy::ARC:
                cache_.emplace<Arc>(lower_level, capacity, invalidate_upper);
                break;
            case CachePolicy::TWO_Q:
                cache_.emplace<TwoQ>(lower_level, capacity, invalidate_upper);
                break;
            case CachePolicy::LIRS:
                cache_.emplace<Lirs>(lower_level, capacity, invalidate_upper);
                break;
        }
    }

    std::string fetch(const std::string& key) {
        ++fetch_count_;
        if (!lower_) {
            return std::visit([&](auto& value) { return value.fetch(key); }, cache_);
        }
        const auto lower_fetches = lower_->fetch_count_;
        const auto data = std::visit([&](auto& value) { return value.fetch(key); }, cache_);
        if (lower_->fetch_count_ == lower_fetches) {
            ++(*hit_count_);
        }
        return data;
    }

    void remove(const std::string& key) {
        std::visit([&](auto& value) { value.remove(key); }, cache_);
    }

private:
    std::variant<Storage, Lfu, Arc, TwoQ, Lirs> cache_;
    Level* upper_ = nullptr;
    Level* const lower_;
    std::size_t* const hit_count_;
    std::size_t fetch_count_ = 0;
};

} // namespace

std::vector<std::string> read_requests(std::istream& input,
                                       const std::size_t request_count) {
    std::vector<std::string> requests(request_count);
    for (auto& key : requests) {
        if (!(input >> key)) {
            throw std::runtime_error("Incomplete request sequence");
        }
    }
    return requests;
}

CacheHitStatistics count_hits_by_level(const Config& config, const std::vector<std::size_t>& capacities,
                                       const std::vector<std::string>& requests) {
    if (capacities.size() != config.levels.size()) {
        throw std::invalid_argument("Capacity count must match level count");
    }
    for (std::size_t i = 0; i < capacities.size(); ++i) {
        if (capacities[i] == 0) {
            throw std::invalid_argument("Cache capacity must be positive");
        }
        if (i > 0 && capacities[i - 1] > capacities[i]) {
            throw std::invalid_argument("Inclusive cache capacities must not decrease");
        }
    }

    CacheHitStatistics statistics;
    statistics.level_hits.resize(config.levels.size());
    std::deque<Level> levels;
    levels.emplace_front(statistics.storage_misses);
    for (std::size_t i = config.levels.size(); i > 0; --i) {
        levels.emplace_front(config.levels[i - 1], capacities[i - 1],
                             levels.front(), statistics.level_hits[i - 1]);
    }

    for (const auto& key : requests) {
        levels.front().fetch(key);
    }
    return statistics;
}

std::size_t count_hits(const Config& config, const std::vector<std::size_t>& capacities,
                       const std::vector<std::string>& requests) {
    return count_hits_by_level(config, capacities, requests).total_hits();
}

std::size_t CacheHitStatistics::total_hits() const {
    return std::accumulate(level_hits.begin(), level_hits.end(), std::size_t{0});
}
